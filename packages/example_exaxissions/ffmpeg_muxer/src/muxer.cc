//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "muxer.h"

#include <cassert>
#include <cstdint>
#include <memory>

#include "libavcodec/packet.h"
#include "libavutil/channel_layout.h"
#include "aptima_runtime/binding/cpp/aptima.h"
#include "aptima_utils/lang/cpp/lib/buf.h"
#include "aptima_utils/log/log.h"
#include "aptima_utils/macro/check.h"

#define ms2pts(pts, stream)                \
  av_rescale(pts, (stream)->time_base.den, \
             static_cast<int64_t>((stream)->time_base.num) * 1000)
#define pts2ms(ms, stream)                                             \
  av_rescale(ms, static_cast<int64_t>((stream)->time_base.num) * 1000, \
             (stream)->time_base.den)

#define GET_FFMPEG_ERROR_MESSAGE(err_msg, errnum)                            \
  /* NOLINTNEXTLINE */                                                       \
  for (char err_msg[AV_ERROR_MAX_STRING_SIZE], times = 0;                    \
       get_ffmpeg_error_message_(err_msg, AV_ERROR_MAX_STRING_SIZE, errnum), \
                                               times == 0;                   \
       ++times)

#define aptima_VIDEO_FRAME_PIXEL_FMT AV_PIX_FMT_RGB24

// @{
// Output video settings.
#define OUTPUT_VIDEO_CODEC AV_CODEC_ID_H264
#define OUTPUT_VIDEO_PIXEL_FORMAT AV_PIX_FMT_YUV420P
#define OUTPUT_VIDEO_GOP_SIZE 10  // 1 I-frame for every 10 frames at most
// output will be delayed by kVideoMaxBFrame + 1
#define OUTPUT_VIDEO_MAX_B_FRAMES 10
// @}

// @{
// Output audio settings.
#define OUTPUT_AUDIO_CODEC AV_CODEC_ID_AAC
#define OUTPUT_AUDIO_FORMAT AV_SAMPLE_FMT_FLTP
#define OUTPUT_AUDIO_SAMPLE_RATE 48000
#define OUTPUT_AUDIO_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO
// @}

namespace aptima {
namespace ffmpeg_extension {

void get_ffmpeg_error_message_(char *buf, size_t buf_length, int errnum) {
  aptima_ASSERT(buf && buf_length, "Invalid argument.");

  // Get error from ffmpeg.
  if (av_strerror(errnum, buf, buf_length) != 0) {
    int written =
        snprintf(buf, buf_length, "Unknown ffmpeg error code: %d", errnum);
    aptima_ASSERT(written > 0, "Should not happen.");
  }
}

AVFrame *yuv_frame_create_(int width, int height) {
  AVFrame *av_frame = av_frame_alloc();
  aptima_ASSERT(av_frame, "Failed to create AVframe.");

  av_frame->width = width;
  av_frame->height = height;
  av_frame->format = OUTPUT_VIDEO_PIXEL_FORMAT;

  av_image_alloc(av_frame->data, av_frame->linesize, width, height,
                 OUTPUT_VIDEO_PIXEL_FORMAT, 32);

  return av_frame;
}

void yuv_frame_destroy(AVFrame *av_frame) {
  aptima_ASSERT(av_frame, "Invalid argument.");

  av_freep(&(av_frame->data[0]));
  av_frame_free(&av_frame);
}

muxer_t::muxer_t()
    : src_video_width_(0),
      src_video_height_(0),
      src_video_bit_rate_(0),
      src_video_number_of_frames_(0),
      src_video_frame_rate_({}),
      src_video_time_base_({}),
      src_audio_sample_rate_(0),
      src_audio_time_base_({}),
      src_audio_channel_layout_(0),
      output_format_ctx_(nullptr),
      video_stream_idx_(-1),
      audio_stream_idx_(-1),
      next_video_idx_(0),
      next_audio_idx_(0),
      video_encoder_ctx_(nullptr),
      audio_encoder_ctx_(nullptr),
      video_encoder_(nullptr),
      audio_encoder_(nullptr),
      video_stream_(nullptr),
      audio_stream_(nullptr),
      video_converter_ctx_(nullptr),
      audio_converter_ctx_(nullptr),
      packet_(av_packet_alloc()),
      audio_fifo_(nullptr),
      audio_frame_(nullptr),
      audio_prepend_pts_(0) {}

muxer_t::~muxer_t() {
  if (audio_frame_ != nullptr) {
    av_frame_free(&audio_frame_);
  }
  if (packet_ != nullptr) {
    av_packet_free(&packet_);
  }

  if (output_format_ctx_ != nullptr && output_format_ctx_->pb != nullptr) {
    // Write trailer information.
    av_write_trailer(output_format_ctx_);
  }

  if (video_encoder_ctx_ != nullptr) {
    avcodec_close(video_encoder_ctx_);
    avcodec_free_context(&video_encoder_ctx_);
  }

  if (audio_encoder_ctx_ != nullptr) {
    avcodec_close(audio_encoder_ctx_);
    avcodec_free_context(&audio_encoder_ctx_);
  }

  if (video_converter_ctx_ != nullptr) {
    sws_freeContext(video_converter_ctx_);
  }
  if (audio_converter_ctx_ != nullptr) {
    swr_free(&audio_converter_ctx_);
  }
  if (audio_fifo_ != nullptr) {
    av_audio_fifo_free(audio_fifo_);
  }

  if (output_format_ctx_ != nullptr) {
    if (output_format_ctx_->pb != nullptr) {
      // Close the output file.
      avio_close(output_format_ctx_->pb);
    }

    // Free output context.
    avformat_free_context(output_format_ctx_);
  }
}

bool muxer_t::is_av_encoder_opened_() {
  return video_encoder_ != nullptr || audio_encoder_ != nullptr;
}

// Convert the video index in the video encoder context to the video index in
// the video output stream.
int64_t muxer_t::next_video_pts_() {
  // (next_video_idx_ / framerate) = x seconds totally
  // x seconds / time_base = pts according to the time_base of the output stream
  return av_rescale_q(next_video_idx_, av_inv_q(video_encoder_ctx_->framerate),
                      video_stream_->time_base);
}

// Convert the audio index in the audio encoder context to the audio index in
// the audio output stream.
int64_t muxer_t::next_audio_pts_() {
  // (next_audio_idx_ / sample_rate) = x seconds totally
  // x seconds / time_base
  // = x seconds * (1/time_base)
  // = x seconds * time_base.den / time_base.num
  // = pts according to the time_base of the output stream
  return av_rescale(next_audio_idx_, audio_stream_->time_base.den,
                    static_cast<int64_t>(audio_stream_->time_base.num) *
                        audio_encoder_ctx_->sample_rate);
}

void muxer_t::flush_remaining_audio_frames() {
  int ffmpeg_rc = avcodec_send_frame(audio_encoder_ctx_, nullptr);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGE("Failed to flush audio frame: %s", err_msg);
    }
    return;
  }

  while (ffmpeg_rc >= 0) {
    av_packet_unref(packet_);

    ffmpeg_rc = avcodec_receive_packet(audio_encoder_ctx_, packet_);
    if (ffmpeg_rc < 0) {
      if (ffmpeg_rc == AVERROR(EAGAIN)) {
        ffmpeg_rc = 0;
        continue;
      } else {
        GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
          aptima_LOGE("Failed to flush audio frame: %s", err_msg);
        }
        return;
      }
    }

    packet_->stream_index = static_cast<int>(audio_stream_idx_);

    aptima_LOGD("Encoded an audio packet after flushing, pts=%" PRId64
             ", dts=%" PRId64 ", size=%d",
             packet_->pts, packet_->dts, packet_->size);

    ffmpeg_rc = av_interleaved_write_frame(output_format_ctx_, packet_);
    if (ffmpeg_rc < 0) {
      GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
        aptima_LOGD("Error writing audio packet: %s", err_msg);
      }
    }
  }
}

bool muxer_t::encode_audio_frame_(AVFrame *av_frame) {
  aptima_ASSERT(audio_encoder_ctx_ && output_format_ctx_, "Invalid argument.");

  // Supply a raw audio frame to the audio encoder.
  int ffmpeg_rc = avcodec_send_frame(audio_encoder_ctx_, av_frame);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to encode a audio frame: %s", err_msg);
    }
    if (ffmpeg_rc == AVERROR_EOF) {
      aptima_LOGD("encode an EOF audio packet.");
      return true;
    }
    return false;
  }

  // Discard the previous received audio packet.
  av_packet_unref(packet_);

  // Read encoded data from the encoder.
  ffmpeg_rc = avcodec_receive_packet(audio_encoder_ctx_, packet_);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGE("Failed to encode an audio packet: %s", err_msg);
    }
    if (ffmpeg_rc == AVERROR(EAGAIN)) {
      aptima_LOGE(
          "Failed to encode an audio packet: \
      need more frame to produce output packet (audio case)");
      return true;
    }
    if (ffmpeg_rc == AVERROR_EOF) {
      aptima_LOGD("encode an EOF audio packet.");
      return true;
    }
    return false;
  }

  packet_->stream_index = static_cast<int>(audio_stream_idx_);

  // Write a packet to a output media file ensuring correct interleaving.
  ffmpeg_rc = av_interleaved_write_frame(output_format_ctx_, packet_);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to write a audio packet to the output stream: %s",
               err_msg);
    }
    return false;
  }

  return true;
}

bool muxer_t::encode_audio_silent_frame_() {
  if (audio_encoder_ == nullptr) {
    aptima_LOGE("Must open audio stream first");
    return false;
  }

  AVCodecParameters *encoded_stream_params = audio_stream_->codecpar;

  if (audio_frame_ == nullptr) {
    audio_frame_ = av_frame_alloc();
    if (audio_frame_ == nullptr) {
      aptima_LOGE("Failed to allocate audio frame.");
      return false;
    }

    audio_frame_->nb_samples = encoded_stream_params->frame_size;
    audio_frame_->format = encoded_stream_params->format;
    audio_frame_->channel_layout = encoded_stream_params->channel_layout;
    av_channel_layout_copy(&audio_frame_->ch_layout,
                           &encoded_stream_params->ch_layout);
    if (av_frame_get_buffer(audio_frame_, 32) < 0) {
      aptima_LOGE("Failed to allocate audio frame");
      return false;
    }
  }

  av_frame_make_writable(audio_frame_);
  av_samples_set_silence(
      audio_frame_->data, 0, audio_frame_->nb_samples,
      audio_frame_->ch_layout.nb_channels,
      static_cast<enum AVSampleFormat>(audio_frame_->format));

  audio_frame_->pts = next_audio_pts_();
  next_audio_idx_ += audio_frame_->nb_samples;

  aptima_LOGD("Encode a silent audio frame, pts=%" PRId64, audio_frame_->pts);

  return encode_audio_frame_(audio_frame_);
}

// The parameters of the stream we push back should be equal to the parameters
// of the original stream we pull.
bool muxer_t::open(const std::string &dest_name, const bool realtime) {
  if (is_av_encoder_opened_()) {
    aptima_LOGD("Muxer already opened");
    return true;
  }

  aptima_LOGD("Preparing to open output stream [%s]", dest_name.c_str());

  // Create encoders.
  const char *format_str = realtime ? "flv" : "mp4";

  dest_name_ = dest_name;

  int ffmpeg_rc = avformat_alloc_output_context2(
      &output_format_ctx_, nullptr, format_str, dest_name_.c_str());
  if (ffmpeg_rc < 0 || (output_format_ctx_ == nullptr)) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to open output stream: cannot alloc output context: %s",
               err_msg);
    }
    return false;
  }

  open_video_encoder_(realtime);
  open_audio_encoder_();

  if (!is_av_encoder_opened_()) {
    aptima_LOGD("Failed to open encoders");
    return false;
  }

  // Open output stream
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  if ((output_format_ctx_->oformat->flags & AVFMT_NOFILE) == 0) {
    ffmpeg_rc =
        avio_open2(&output_format_ctx_->pb, dest_name_.c_str(), AVIO_FLAG_WRITE,
                   &output_format_ctx_->interrupt_callback, nullptr);
    if (ffmpeg_rc < 0) {
      GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
        aptima_LOGD("Failed to open output: %s", err_msg);
      }
      return false;
    }
  }

  ffmpeg_rc = avformat_write_header(output_format_ctx_, nullptr);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to write output header: %s", err_msg);
    }
    return false;
  }

  aptima_LOGD("Output stream [%s] is opened", dest_name_.c_str());

  // Encode 2 silent frames for AAC encoder.
  encode_audio_silent_frame_();
  encode_audio_silent_frame_();
  audio_prepend_pts_ = next_audio_pts_();

  return true;
}

void muxer_t::dump_video_info_() {
  aptima_LOGD("v:width:       %d", video_encoder_ctx_->width);
  aptima_LOGD("v:height:      %d", video_encoder_ctx_->height);
  aptima_LOGD("v:bit_rate:    %" PRId64, video_encoder_ctx_->bit_rate);
  aptima_LOGD("v:rc_min_rate: %" PRId64, video_encoder_ctx_->rc_min_rate);
  aptima_LOGD("v:rc_max_rate: %" PRId64, video_encoder_ctx_->rc_max_rate);
  aptima_LOGD("v:time_base:   %d/%d", video_encoder_ctx_->time_base.num,
           video_encoder_ctx_->time_base.den);
  aptima_LOGD("v:pix_fmt:     %d", video_encoder_ctx_->pix_fmt);
  aptima_LOGD("v:framerate:   %d/%d", video_encoder_ctx_->framerate.num,
           video_encoder_ctx_->framerate.den);
  aptima_LOGD("v:time_base:   %d/%d", video_stream_->time_base.num,
           video_stream_->time_base.den);
}

void muxer_t::dump_audio_info_() {
  aptima_LOGD("a:sample_fmt:     %d", audio_encoder_ctx_->sample_fmt);
  aptima_LOGD("a:sample_rate:    %d", audio_encoder_ctx_->sample_rate);
  aptima_LOGD("a:channels:       %d", audio_encoder_ctx_->ch_layout.nb_channels);
  aptima_LOGD("a:time_base:      %d/%d", audio_encoder_ctx_->time_base.num,
           audio_encoder_ctx_->time_base.den);
  aptima_LOGD("a:bit_rate:       %" PRId64, audio_encoder_ctx_->bit_rate);
  aptima_LOGD("a:time_base:      %d/%d", audio_stream_->time_base.num,
           audio_stream_->time_base.den);
  aptima_LOGD("a:frame_size:     %d", audio_stream_->codecpar->frame_size);
}

bool muxer_t::open_video_encoder_(const bool realtime) {
  aptima_ASSERT(output_format_ctx_, "Invalid argument.");

  // The ownership of 'video_encoder' belongs to ffmpeg, and it's not necessary
  // to delete it when encountering errors.
  video_encoder_ = avcodec_find_encoder(OUTPUT_VIDEO_CODEC);
  if (video_encoder_ == nullptr) {
    aptima_LOGE("Video encoder not supported: %s",
             avcodec_get_name(OUTPUT_VIDEO_CODEC));
    return false;
  }

  // Add video stream to the output.
  video_stream_ = avformat_new_stream(output_format_ctx_, video_encoder_);
  if (video_stream_ == nullptr) {
    aptima_LOGE("Failed to open video output stream: %s", dest_name_.c_str());
    return false;
  }

  video_stream_idx_ = output_format_ctx_->nb_streams - 1;
  video_stream_->id = static_cast<int>(video_stream_idx_);

  // Initialize video codec context.
  video_encoder_ctx_ = avcodec_alloc_context3(video_encoder_);
  if (video_encoder_ctx_ == nullptr) {
    aptima_LOGE("Failed to allocate video encoder context");
    return false;
  }

  AVRational framerate = src_video_frame_rate_;
  AVRational time_base;
  if (realtime) {
    time_base = av_inv_q(framerate);
  } else {
    time_base = src_video_time_base_;
  }

  video_encoder_ctx_->codec_id = OUTPUT_VIDEO_CODEC;
  video_encoder_ctx_->width = src_video_width_;
  video_encoder_ctx_->height = src_video_height_;
  video_encoder_ctx_->bit_rate = src_video_bit_rate_;
  video_encoder_ctx_->time_base = time_base;
  video_encoder_ctx_->gop_size = OUTPUT_VIDEO_GOP_SIZE;
  video_encoder_ctx_->max_b_frames = OUTPUT_VIDEO_MAX_B_FRAMES;
  video_encoder_ctx_->pix_fmt = OUTPUT_VIDEO_PIXEL_FORMAT;
  video_encoder_ctx_->framerate = framerate;
  video_encoder_ctx_->sample_aspect_ratio = (AVRational){0, 1};
  video_encoder_ctx_->profile = FF_PROFILE_H264_HIGH;
  video_encoder_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  video_stream_->time_base = video_encoder_ctx_->time_base;
  video_stream_->avg_frame_rate = video_encoder_ctx_->framerate;
  video_stream_->sample_aspect_ratio = video_encoder_ctx_->sample_aspect_ratio;

  // Enable video codec
  AVDictionary *av_options = nullptr;
  if (realtime) {
    av_dict_set(&av_options, "tune", "zerolatency", 0);

    // The following 'preset' would affect the encoding quality.
    av_dict_set(&av_options, "preset", "veryfast", 0);
    av_dict_set(&av_options, "profile", "high", 0);
    av_dict_set(&av_options, "level", "5.0", 0);

    // The following is the suggestions from youtube:
    // 854*480*30   = 12000k, youtube:  500k (1/24) - 2000k (1/6)
    // 1280*720*30  = 27000k, youtube: 1500k (1/18) - 4000k (1/6.75)
    // 1280*720*60  = 54000k, youtube: 2250k (1/24) - 4000k (1/9)
    // 1920*1080*30 = 60750k, youtube: 3000k (1/20) - 6000k (1/10)
    //
    // So we set the 'compression ratio' to 1/20 ~ 1/15 ~ 1/12.
    int64_t min_compression_ratio = 20;
    int64_t prefer_compression_ratio = 15;
    int64_t max_compression_ratio = 12;

    int64_t min_rate =
        av_rescale(static_cast<int64_t>(video_encoder_ctx_->width) *
                       video_encoder_ctx_->height,
                   framerate.num, framerate.den) /
        min_compression_ratio;
    int64_t prefer_rate =
        av_rescale(static_cast<int64_t>(video_encoder_ctx_->width) *
                       video_encoder_ctx_->height,
                   framerate.num, framerate.den) /
        prefer_compression_ratio;
    int64_t max_rate =
        av_rescale(static_cast<int64_t>(video_encoder_ctx_->width) *
                       video_encoder_ctx_->height,
                   framerate.num, framerate.den) /
        max_compression_ratio;
    video_encoder_ctx_->rc_min_rate = min_rate;
    video_encoder_ctx_->rc_max_rate = max_rate;
    video_encoder_ctx_->rc_buffer_size = static_cast<int>(max_rate * 2);

    if (video_encoder_ctx_->bit_rate < prefer_rate) {
      aptima_LOGD("Raise bitrate from %" PRId64 " to %" PRId64,
               video_encoder_ctx_->bit_rate, prefer_rate);
      video_encoder_ctx_->bit_rate = prefer_rate;
    }
  } else {
    video_encoder_ctx_->gop_size = 250;    // Follow ffmpeg default setting
    video_encoder_ctx_->max_b_frames = 3;  // Follow ffmpeg default setting
    video_stream_->start_time = 0;

    int64_t frame_dur = av_rescale_q(1, av_inv_q(framerate), time_base);
    video_stream_->duration = src_video_number_of_frames_ * frame_dur;
  }

  int ffmpeg_rc =
      avcodec_open2(video_encoder_ctx_, video_encoder_, &av_options);
  av_dict_free(&av_options);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to open video codec: %s", err_msg);
    }
    return false;
  }

  ffmpeg_rc = avcodec_parameters_from_context(video_stream_->codecpar,
                                              video_encoder_ctx_);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to copy video codec parameters: %s", err_msg);
    }
    return false;
  }

  dump_video_info_();

  aptima_LOGD("%s video encoder opened for stream %d", video_encoder_->name,
           video_stream_idx_);

  return true;
}

bool muxer_t::open_audio_encoder_() {
  audio_encoder_ = avcodec_find_encoder(OUTPUT_AUDIO_CODEC);
  if (audio_encoder_ == nullptr) {
    aptima_LOGE("Audio encoder not supported: %s",
             avcodec_get_name(OUTPUT_AUDIO_CODEC));
    return false;
  }

  // Add audio stream to the output.
  audio_stream_ = avformat_new_stream(output_format_ctx_, audio_encoder_);
  if (audio_stream_ == nullptr) {
    aptima_LOGE("Failed to open audio output stream: %s", dest_name_.c_str());
    return false;
  }

  audio_stream_idx_ = output_format_ctx_->nb_streams - 1;
  audio_stream_->id = static_cast<int>(audio_stream_idx_);

  // Initialize audio codec context.
  audio_encoder_ctx_ = avcodec_alloc_context3(audio_encoder_);
  if (audio_encoder_ctx_ == nullptr) {
    aptima_LOGE("Failed to allocate audio encoder context");
    return false;
  }

  // The settings of encoder:
  // - format: Determined by the codec
  // - sample_rate / channel_layout: Determined by the demuxer, or use default
  //                                 values
  AVSampleFormat sample_fmt = audio_encoder_->sample_fmts != nullptr
                                  ? audio_encoder_->sample_fmts[0]
                                  : OUTPUT_AUDIO_FORMAT;

  int sample_rate = OUTPUT_AUDIO_SAMPLE_RATE;
  uint64_t channel_layout = OUTPUT_AUDIO_CHANNEL_LAYOUT;
  AVRational time_base = (AVRational){1, audio_encoder_ctx_->sample_rate};
  if (src_audio_sample_rate_ > 0) {
    sample_rate = src_audio_sample_rate_;
  }

  if (src_audio_channel_layout_ > 0) {
    channel_layout = src_audio_channel_layout_;
  }

  if (src_audio_time_base_.num > 0) {
    time_base = src_audio_time_base_;
  }

  audio_encoder_ctx_->sample_fmt = sample_fmt;
  audio_encoder_ctx_->sample_rate = sample_rate;
  audio_encoder_ctx_->channel_layout = channel_layout;
  audio_encoder_ctx_->channels =
      av_get_channel_layout_nb_channels(audio_encoder_ctx_->channel_layout);
  audio_encoder_ctx_->time_base = time_base;

  audio_stream_->time_base = time_base;

  // Enable audio codec.
  AVDictionary *av_options = nullptr;
  int ffmpeg_rc =
      avcodec_open2(audio_encoder_ctx_, audio_encoder_, &av_options);
  av_dict_free(&av_options);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGE("Failed to open audio codec: %s", err_msg);
    }
    return false;
  }

  ffmpeg_rc = avcodec_parameters_from_context(audio_stream_->codecpar,
                                              audio_encoder_ctx_);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to copy audio codec parameters: %s", err_msg);
    }
    return false;
  }

  dump_audio_info_();

  aptima_LOGD("%s audio encoder opened for stream %d", audio_encoder_->name,
           audio_stream_idx_);

  return true;
}

bool muxer_t::create_video_converter_(aptima::video_frame_t &video_frame) {
  if (video_converter_ctx_ == nullptr) {
    int width = video_frame.get_width();
    int height = video_frame.get_height();

    video_converter_ctx_ = sws_getContext(
        width, height, aptima_VIDEO_FRAME_PIXEL_FMT, width, height,
        OUTPUT_VIDEO_PIXEL_FORMAT, SWS_POINT, nullptr, nullptr, nullptr);
    if (video_converter_ctx_ == nullptr) {
      aptima_LOGD(
          "Failed to create converter context to convert from RGB frame \
      to YUV frame.");
      return false;
    }
  }

  return true;
}

AVFrame *muxer_t::convert_video_frame_(aptima::video_frame_t &video_frame) {
  // This is an EOF frame.
  if (video_frame.is_eof()) {
    return nullptr;
  }

  if (video_frame.get_pixel_fmt() == aptima_PIXEL_FMT_I420) {
    auto frame_width = video_frame.get_width();
    auto frame_height = video_frame.get_height();

    aptima::buf_t locked_buf = video_frame.lock_buf();

    auto *y_data = locked_buf.data();
    auto *u_data = y_data + frame_width * frame_height;
    auto *v_data = u_data + frame_width * frame_height / 4;

    AVFrame *yuv_frame = yuv_frame_create_(frame_width, frame_height);
    aptima_ASSERT(yuv_frame, "Failed to create YUV frame.");

    av_image_get_buffer_size(OUTPUT_VIDEO_PIXEL_FORMAT, frame_width,
                             frame_height, 16);
    av_image_copy_plane(yuv_frame->data[0], yuv_frame->linesize[0], y_data,
                        frame_width, frame_width, frame_height);
    av_image_copy_plane(yuv_frame->data[1], yuv_frame->linesize[1], u_data,
                        frame_width / 2, frame_width / 2, frame_height / 2);
    av_image_copy_plane(yuv_frame->data[2], yuv_frame->linesize[2], v_data,
                        frame_width / 2, frame_width / 2, frame_height / 2);

    video_frame.unlock_buf(locked_buf);

    return yuv_frame;
  }

  if (video_frame.get_pixel_fmt() == aptima_PIXEL_FMT_RGB24) {
    if (!create_video_converter_(video_frame)) {
      return nullptr;
    }

    aptima::buf_t locked_buf = video_frame.lock_buf();

    const uint8_t *rgb_data[1] = {locked_buf.data()};     // NOLINT
    int rgb_linesize[1] = {video_frame.get_width() * 3};  // NOLINT

    AVFrame *yuv_frame =
        yuv_frame_create_(video_frame.get_width(), video_frame.get_height());
    aptima_ASSERT(yuv_frame, "Failed to create YUV frame.");

    sws_scale(video_converter_ctx_, rgb_data, rgb_linesize, 0,
              video_frame.get_height(), yuv_frame->data, yuv_frame->linesize);

    video_frame.unlock_buf(locked_buf);

    return yuv_frame;
  }

  return nullptr;
}

static void destroy_yuv_frame(AVFrame *yuv_frame) {
  if (yuv_frame != nullptr) {
    yuv_frame_destroy(yuv_frame);
  }
}

// Debug purpose only.
aptima_UNUSED static void save_avframe(const AVFrame *avFrame) {
  FILE *fDump = fopen("encode", "ab");

  uint32_t pitchY = avFrame->linesize[0];
  uint32_t pitchU = avFrame->linesize[1];
  uint32_t pitchV = avFrame->linesize[2];

  uint8_t *avY = avFrame->data[0];
  uint8_t *avU = avFrame->data[1];
  uint8_t *avV = avFrame->data[2];

  for (uint32_t i = 0; i < avFrame->height; i++) {
    (void)fwrite(avY, avFrame->width, 1, fDump);
    avY += pitchY;
  }

  for (uint32_t i = 0; i < avFrame->height / 2; i++) {
    (void)fwrite(avU, avFrame->width / 2, 1, fDump);
    avU += pitchU;
  }

  for (uint32_t i = 0; i < avFrame->height / 2; i++) {
    (void)fwrite(avV, avFrame->width / 2, 1, fDump);
    avV += pitchV;
  }

  (void)fclose(fDump);
}

// Debug purpose only.
aptima_UNUSED static void save_img_frame(
    const std::shared_ptr<aptima::video_frame_t> &pFrame, int index) {
  FILE *pFile = nullptr;
  char szFilename[32];
  int y = 0;

  int width = pFrame->get_width();
  int height = pFrame->get_height();

  // Open file
  int written = snprintf(szFilename, 32, "encode_frame%d.ppm", index);
  aptima_ASSERT(written > 0, "Should not happen.");

  pFile = fopen(szFilename, "wb");

  if (pFile == nullptr) {
    return;
  }

  // Write header
  (void)fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  // Write pixel data
  aptima::buf_t locked_buf = pFrame->lock_buf();
  for (y = 0; y < height; y++) {
    (void)fwrite(locked_buf.data() + y * width * 3, 1, width * 3, pFile);
  }
  pFrame->unlock_buf(locked_buf);

  // Close file
  (void)fclose(pFile);
}

void muxer_t::flush_remaining_video_frames() {
  int ffmpeg_rc = avcodec_send_frame(video_encoder_ctx_, nullptr);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGE("Failed to flush video frame: %s", err_msg);
    }
    return;
  }

  while (ffmpeg_rc >= 0) {
    av_packet_unref(packet_);

    ffmpeg_rc = avcodec_receive_packet(video_encoder_ctx_, packet_);
    if (ffmpeg_rc < 0) {
      if (ffmpeg_rc == AVERROR(EAGAIN)) {
        ffmpeg_rc = 0;
        continue;
      } else {
        GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
          aptima_LOGE("Failed to flush video frame: %s", err_msg);
        }
        return;
      }
    }

    packet_->stream_index = static_cast<int>(video_stream_idx_);
    packet_->duration =
        av_rescale_q(packet_->duration, video_encoder_ctx_->time_base,
                     video_stream_->time_base);
    packet_->pts = av_rescale_q(packet_->pts, video_encoder_ctx_->time_base,
                                video_stream_->time_base);
    packet_->dts = av_rescale_q(packet_->dts, video_encoder_ctx_->time_base,
                                video_stream_->time_base);

    ffmpeg_rc = av_interleaved_write_frame(output_format_ctx_, packet_);
    if (ffmpeg_rc < 0) {
      GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
        aptima_LOGD("Error writing video packet: %s", err_msg);
      }
    }
  }
}

ENCODE_STATUS muxer_t::encode_video_frame(
    std::unique_ptr<aptima::video_frame_t> video_frame) {
  if (video_encoder_ == nullptr) {
    aptima_LOGE("Must open video stream first");
    return ENCODE_STATUS_ERROR;
  }

  AVFrame *yuv_frame = convert_video_frame_(*video_frame);
  if (yuv_frame != nullptr) {
    yuv_frame->pts = next_video_pts_();
    next_video_idx_++;

    // DEBUG
    // save_avframe(yuv_frame);
    // save_img_frame(video_frame, yuv_frame->pts);
  } else {
    // EOF frame.
    flush_remaining_video_frames();
    return ENCODE_STATUS_EOF;
  }

  int ffmpeg_rc = avcodec_send_frame(video_encoder_ctx_, yuv_frame);
  if (ffmpeg_rc < 0) {
    destroy_yuv_frame(yuv_frame);
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGE("Failed to encode a video frame: %s", err_msg);
    }
    if (ffmpeg_rc == AVERROR_EOF) {
      aptima_LOGD("encode an EOF video packet.");
      return ENCODE_STATUS_ERROR;
    }
    return ENCODE_STATUS_ERROR;
  }

  // Discard the previous received video.
  av_packet_unref(packet_);

  ffmpeg_rc = avcodec_receive_packet(video_encoder_ctx_, packet_);
  if (ffmpeg_rc < 0) {
    destroy_yuv_frame(yuv_frame);
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGE("Failed to encode a video packet: %s", err_msg);
    }
    if (ffmpeg_rc == AVERROR(EAGAIN)) {
      aptima_LOGE(
          "Failed to encode a video packet: \
      need more frame to produce output packet (b-frame's case)");
      return ENCODE_STATUS_SUCCESS;
    }

    if (ffmpeg_rc == AVERROR_EOF) {
      aptima_LOGD("encode an EOF video packet.");
      return ENCODE_STATUS_ERROR;
    }
    return ENCODE_STATUS_ERROR;
  }

  packet_->stream_index = static_cast<int>(video_stream_idx_);
  packet_->duration =
      av_rescale_q(packet_->duration, video_encoder_ctx_->time_base,
                   video_stream_->time_base);
  packet_->pts = av_rescale_q(packet_->pts, video_encoder_ctx_->time_base,
                              video_stream_->time_base);
  packet_->dts = av_rescale_q(packet_->dts, video_encoder_ctx_->time_base,
                              video_stream_->time_base);

  aptima_LOGD("Encoded a video packet, pts=%" PRId64 ", dts=%" PRId64 ", size=%d",
           packet_->pts, packet_->dts, packet_->size);

  // Write the encoded video packet to the output stream.
  ffmpeg_rc = av_interleaved_write_frame(output_format_ctx_, packet_);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Error writing video packet: %s", err_msg);
    }
    destroy_yuv_frame(yuv_frame);
    return ENCODE_STATUS_ERROR;
  }

  destroy_yuv_frame(yuv_frame);
  return ENCODE_STATUS_SUCCESS;
}

// Because the frame_size between the original audio and the requirement of the
// target audio codec would be different, we need a FIFO to queue samples.
bool muxer_t::allocate_audio_fifo_(AVCodecParameters *encoded_stream_params) {
  if (audio_fifo_ == nullptr) {
    audio_fifo_ = av_audio_fifo_alloc(
        static_cast<enum AVSampleFormat>(encoded_stream_params->format),
        encoded_stream_params->ch_layout.nb_channels,
        encoded_stream_params->frame_size);
    if (audio_fifo_ == nullptr) {
      aptima_LOGW("Failed to create audio FIFO.");
      return false;
    }
  }
  return true;
}

bool muxer_t::allocate_audio_frame_(AVCodecParameters *encoded_stream_params) {
  if (audio_frame_ == nullptr) {
    audio_frame_ = av_frame_alloc();
    if (audio_frame_ == nullptr) {
      aptima_LOGW("Failed to allocate audio frame.");
      return false;
    }

    audio_frame_->nb_samples = encoded_stream_params->frame_size;
    audio_frame_->format = encoded_stream_params->format;
    av_channel_layout_copy(&audio_frame_->ch_layout,
                           &encoded_stream_params->ch_layout);
    if (av_frame_get_buffer(audio_frame_, 32) < 0) {
      aptima_LOGW("Failed to allocate audio frame.");
      return false;
    }
  }
  return true;
}

// Initialize audio resampler.
bool muxer_t::create_audio_converter_(AVCodecParameters *encoded_stream_params,
                                      aptima::audio_frame_t &aptima_audio_frame) {
  if (audio_converter_ctx_ == nullptr) {
    audio_converter_ctx_ = swr_alloc();
    if (audio_converter_ctx_ == nullptr) {
      aptima_LOGW("Failed to create audio resampler.");
      return false;
    }

    auto sample_format = AV_SAMPLE_FMT_S16;
    auto bytes_per_sample = aptima_audio_frame.get_bytes_per_sample();
    auto data_fmt = aptima_audio_frame.get_data_fmt();

    if (data_fmt == aptima_AUDIO_FRAME_DATA_FMT_INTERLEAVE) {
      if (bytes_per_sample == 2) {
        sample_format = AV_SAMPLE_FMT_S16;
      } else {
        aptima_ASSERT(0, "not support yet.");
      }
    } else {
      if (bytes_per_sample == 4) {
        sample_format = AV_SAMPLE_FMT_FLTP;
      } else {
        aptima_ASSERT(0, "not support yet.");
      }
    }

    av_opt_set_channel_layout(audio_converter_ctx_, "in_channel_layout",
                              (int64_t)(aptima_audio_frame.get_channel_layout()),
                              0);
    av_opt_set_int(audio_converter_ctx_, "in_sample_rate",
                   aptima_audio_frame.get_sample_rate(), 0);
    av_opt_set_sample_fmt(audio_converter_ctx_, "in_sample_fmt", sample_format,
                          0);
    av_opt_set_channel_layout(audio_converter_ctx_, "out_channel_layout",
                              (int64_t)(encoded_stream_params->channel_layout),
                              0);
    av_opt_set_int(audio_converter_ctx_, "out_sample_rate",
                   encoded_stream_params->sample_rate, 0);
    av_opt_set_sample_fmt(
        audio_converter_ctx_, "out_sample_fmt",
        static_cast<enum AVSampleFormat>(encoded_stream_params->format), 0);

    aptima_LOGD("Audio resampler setting: from [%d, %d] to [%d, %d]",
             audio_stream_->codecpar->sample_rate, sample_format,
             audio_stream_->codecpar->sample_rate,
             audio_stream_->codecpar->format);

    int ffmpeg_rc = swr_init(audio_converter_ctx_);
    if (ffmpeg_rc < 0) {
      GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
        aptima_LOGD("Failed to initialize resampler: %s", err_msg);
      }
      return false;
    }
  }
  return true;
}

bool muxer_t::convert_audio_frame_(AVCodecParameters *encoded_stream_params,
                                   aptima::audio_frame_t &aptima_audio_frame) {
  uint8_t **dst_channels = nullptr;
  int32_t dst_nb_samples = aptima_audio_frame.get_samples_per_channel();

  // Some of the following functions would require a 'dst_nb_samples' of 'int'
  // type, so we check the value of it so that the casting would be safe.
  aptima_ASSERT(dst_nb_samples <= INT_MAX, "Should not happen.");

  int ffmpeg_rc = av_samples_alloc_array_and_samples(
      &dst_channels, nullptr, encoded_stream_params->ch_layout.nb_channels,
      static_cast<int>(dst_nb_samples),
      static_cast<enum AVSampleFormat>(encoded_stream_params->format), 0);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to allocate audio sample: %s", err_msg);
    }
    return false;
  }

  uint8_t *in[8] = {nullptr};
  aptima::buf_t locked_in_buf = aptima_audio_frame.lock_buf();
  in[0] = locked_in_buf.data();

  ffmpeg_rc = swr_convert(audio_converter_ctx_, dst_channels, dst_nb_samples,
                          const_cast<const uint8_t **>(in),
                          aptima_audio_frame.get_samples_per_channel());
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to convert audio samples: %s", err_msg);
    }

    aptima_audio_frame.unlock_buf(locked_in_buf);

    return false;
  }

  aptima_audio_frame.unlock_buf(locked_in_buf);

  ffmpeg_rc =
      av_audio_fifo_realloc(audio_fifo_, av_audio_fifo_size(audio_fifo_) +
                                             static_cast<int>(dst_nb_samples));
  if (ffmpeg_rc < 0) {
    aptima_LOGD("Failed to reallocate FIFO.");
    return false;
  }

  ffmpeg_rc =
      av_audio_fifo_write(audio_fifo_, reinterpret_cast<void **>(dst_channels),
                          static_cast<int>(dst_nb_samples));
  if (ffmpeg_rc < static_cast<int>(dst_nb_samples)) {
    aptima_LOGD("Failed to write audio sample to FIFO.");
    return false;
  }

  av_freep(&dst_channels[0]);
  av_freep(&dst_channels);

  return true;
}

ENCODE_STATUS muxer_t::encode_audio_frame(
    std::unique_ptr<aptima::audio_frame_t> aptima_audio_frame) {
  if (audio_encoder_ == nullptr) {
    aptima_LOGD("Must open audio stream first");
    return ENCODE_STATUS_ERROR;
  }

  if (aptima_audio_frame->is_eof()) {
    aptima_LOGD("Encode EOF audio frame.");
    flush_remaining_audio_frames();
    return ENCODE_STATUS_EOF;
  }

  AVCodecParameters *encoded_stream_params = audio_stream_->codecpar;

  // Assume no configuration change during the whole process.
  aptima_ASSERT(
      aptima_audio_frame->get_sample_rate() == encoded_stream_params->sample_rate,
      "Should not happen %d, %d", aptima_audio_frame->get_sample_rate(),
      encoded_stream_params->sample_rate);

  // Lazy create necessary components (FIFO, frame, resampler)
  if (!allocate_audio_fifo_(encoded_stream_params)) {
    return ENCODE_STATUS_ERROR;
  }
  if (!allocate_audio_frame_(encoded_stream_params)) {
    return ENCODE_STATUS_ERROR;
  }
  if (!create_audio_converter_(encoded_stream_params, *aptima_audio_frame)) {
    return ENCODE_STATUS_ERROR;
  }

  // "convert" and "push" converted sample to FIFO.
  uint8_t **dst_channels = nullptr;
  int32_t dst_nb_samples = aptima_audio_frame->get_samples_per_channel();

  // Some of the following functions would require a 'dst_nb_samples' of 'int'
  // type, so we check the value of it so that the casting would be safe.
  aptima_ASSERT(dst_nb_samples <= INT_MAX, "Should not happen.");

  int ffmpeg_rc = av_samples_alloc_array_and_samples(
      &dst_channels, nullptr, encoded_stream_params->ch_layout.nb_channels,
      dst_nb_samples,
      static_cast<enum AVSampleFormat>(encoded_stream_params->format), 0);
  if (ffmpeg_rc < 0) {
    aptima_LOGD("Failed to allocate audio sample.");
    return ENCODE_STATUS_ERROR;
  }

  uint8_t *in[8] = {nullptr};
  aptima::buf_t locked_in_buf = aptima_audio_frame->lock_buf();
  in[0] = locked_in_buf.data();

  ffmpeg_rc = swr_convert(audio_converter_ctx_, dst_channels, dst_nb_samples,
                          const_cast<const uint8_t **>(in),
                          aptima_audio_frame->get_samples_per_channel());
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      aptima_LOGD("Failed to convert audio samples: %s", err_msg);
    }

    aptima_audio_frame->unlock_buf(locked_in_buf);

    return ENCODE_STATUS_ERROR;
  }

  aptima_audio_frame->unlock_buf(locked_in_buf);

  ffmpeg_rc =
      av_audio_fifo_realloc(audio_fifo_, av_audio_fifo_size(audio_fifo_) +
                                             static_cast<int>(dst_nb_samples));
  if (ffmpeg_rc < 0) {
    aptima_LOGD("Failed to reallocate FIFO.");
    return ENCODE_STATUS_ERROR;
  }

  ffmpeg_rc =
      av_audio_fifo_write(audio_fifo_, reinterpret_cast<void **>(dst_channels),
                          static_cast<int>(dst_nb_samples));
  if (ffmpeg_rc < static_cast<int>(dst_nb_samples)) {
    aptima_LOGD("Failed to write audio sample to FIFO.");
    return ENCODE_STATUS_ERROR;
  }

  av_freep(&dst_channels[0]);
  av_freep(&dst_channels);

  // "pop" frame from FIFO if there are enough samples
  AVFrame *frame = audio_frame_;
  while (av_audio_fifo_size(audio_fifo_) >= encoded_stream_params->frame_size) {
    if (av_audio_fifo_read(audio_fifo_, reinterpret_cast<void **>(frame->data),
                           frame->nb_samples) < frame->nb_samples) {
      aptima_LOGD("Failed to read data from FIFO");
      return ENCODE_STATUS_ERROR;
    }

    frame->pts = next_audio_pts_();
    next_audio_idx_ += frame->nb_samples;

    if (!encode_audio_frame_(frame)) {
      return ENCODE_STATUS_ERROR;
    }
  }

  return ENCODE_STATUS_SUCCESS;
}

int64_t muxer_t::next_video_timing() {
  return video_stream_ != nullptr ? pts2ms(next_video_pts_(), video_stream_)
                                  : 0;
}

}  // namespace ffmpeg_extension
}  // namespace aptima
