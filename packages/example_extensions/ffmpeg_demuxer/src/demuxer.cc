//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "demuxer.h"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>

#include "libavcodec/packet.h"
#include "libavutil/channel_layout.h"
#include "libswresample/swresample.h"
#include "ten_runtime/binding/cpp/ten.h"
#include "ten_runtime/msg/video_frame/video_frame.h"
#include "ten_utils/macro/check.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/codec_id.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avio.h>
#include <libavutil/avstring.h>
#include <libavutil/avutil.h>
#include <libavutil/common.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#include "demuxer_thread.h"
#include "ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

#define GET_FFMPEG_ERROR_MESSAGE(err_msg, errnum)                            \
  /* NOLINTNEXTLINE */                                                       \
  for (char err_msg[AV_ERROR_MAX_STRING_SIZE], times = 0;                    \
       get_ffmpeg_error_message_(err_msg, AV_ERROR_MAX_STRING_SIZE, errnum), \
                                               times == 0;                   \
       ++times)

#define TEN_AUDIO_FRAME_SAMPLE_FMT AV_SAMPLE_FMT_S16
#define TEN_VIDEO_FRAME_PIXEL_FMT AV_PIX_FMT_RGB24

namespace ten {
namespace ffmpeg_extension {

void get_ffmpeg_error_message_(char *buf, size_t buf_length, int errnum) {
  TEN_ASSERT(buf && buf_length, "Invalid argument.");

  // Get error from ffmpeg.
  if (av_strerror(errnum, buf, buf_length) != 0) {
    (void)snprintf(buf, buf_length, "Unknown ffmpeg error code: %d", errnum);
  }
}

demuxer_t::demuxer_t(ten::ten_env_proxy_t *ten_env_proxy,
                     demuxer_thread_t *demuxer_thread)
    : demuxer_thread_(demuxer_thread),
      ten_env_proxy_(ten_env_proxy),
      input_format_context_(nullptr),
      interrupt_cb_param_(nullptr),
      video_stream_idx_(-1),
      audio_stream_idx_(-1),
      video_decoder_ctx_(nullptr),
      audio_decoder_ctx_(nullptr),
      video_decoder_(nullptr),
      audio_decoder_(nullptr),
      video_converter_ctx_(nullptr),
      audio_converter_ctx_(nullptr),
      packet_(av_packet_alloc()),
      frame_(av_frame_alloc()),
      rotate_degree_(0),
      audio_sample_rate_(0),
      audio_channel_layout_(0) {}

demuxer_t::~demuxer_t() {
  av_packet_free(&packet_);
  av_frame_free(&frame_);

  // The ownership of 'video_decoder' belongs to ffmpeg, and it's not necessary
  // to delete it when encountering errors.

  if (video_decoder_ctx_ != nullptr) {
    avcodec_free_context(&video_decoder_ctx_);
  }

  // The ownership of 'audio_decoder' belongs to ffmpeg, and it's not necessary
  // to delete it when encountering errors.

  if (audio_decoder_ctx_ != nullptr) {
    avcodec_free_context(&audio_decoder_ctx_);
  }

  if (video_converter_ctx_ != nullptr) {
    sws_freeContext(video_converter_ctx_);
  }

  if (audio_converter_ctx_ != nullptr) {
    swr_free(&audio_converter_ctx_);
  }

  if (input_format_context_ != nullptr) {
    // We need to use avformat_close_input() to close all the contexts opened by
    // avformat_open_input(), otherwise we will encounter memory leakage in
    // some format (ex: hls).
    avformat_close_input(&input_format_context_);
  }

  if (interrupt_cb_param_ != nullptr) {
    TEN_FREE(interrupt_cb_param_);
    interrupt_cb_param_ = nullptr;
  }

  TEN_LOGD("Demuxer instance destructed.");
}

// This is a callback which will be called during the processing of the FFmpeg,
// and if returning a non-zero value from this callback, this will break the
// processing job of FFmpeg at that time, therefore, preventing FFmpeg from
// blocking infinitely.
int interrupt_cb_(void *p) {
  TEN_ASSERT(p, "Invalid argument.");

  auto *r = reinterpret_cast<interrupt_cb_param_t *>(p);
  if (r->last_time > 0) {
    if (time(nullptr) - r->last_time > 20) {
      // 1 second timeout.
      return 1;
    }
  }

  return 0;
}

AVFormatContext *demuxer_t::create_input_format_context_internal_(
    const std::string &input_stream_loc) {
  TEN_ASSERT(input_stream_loc.length() > 0, "Invalid argument.");

  AVFormatContext *input_format_context = avformat_alloc_context();
  if (input_format_context == nullptr) {
    TEN_LOGE("Failed to create AVFormatContext.");
    return nullptr;
  }

  if (interrupt_cb_param_ == nullptr) {
    interrupt_cb_param_ = static_cast<interrupt_cb_param_t *>(
        TEN_MALLOC(sizeof(interrupt_cb_param_t)));
    TEN_ASSERT(interrupt_cb_param_, "Failed to allocate memory.");
  }

  input_format_context->interrupt_callback.callback = interrupt_cb_;
  input_format_context->interrupt_callback.opaque = interrupt_cb_param_;

  AVDictionary *av_options = nullptr;

  // This value could be decreased to improve the latency.
  av_dict_set(&av_options, "analyzeduration", "1000000", 0);  // 1000 msec

  // Wait for the input stream to appear.
  interrupt_cb_param_->last_time = time(nullptr);

  // Open an input stream and read the header.
  int ffmpeg_rc = avformat_open_input(
      &input_format_context, input_stream_loc.c_str(), nullptr, &av_options);

  av_dict_free(&av_options);

  if (ffmpeg_rc == 0) {
    TEN_LOGD("Open input stream %s successfully.", input_stream_loc.c_str());
  } else {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      TEN_LOGW("Failed to open input stream %s: %s", input_stream_loc.c_str(),
               err_msg);
    }

    // Close the input, and the caller might try again.
    avformat_close_input(&input_format_context);
  }

  return input_format_context;
}

AVFormatContext *demuxer_t::create_input_format_context_(
    const std::string &input_stream_loc) {
  TEN_ASSERT(input_stream_loc.length() > 0, "Invalid argument.");

  AVFormatContext *input_format_context = nullptr;
  while (true) {
    input_format_context =
        create_input_format_context_internal_(input_stream_loc);
    if (input_format_context != nullptr) {
      break;
    } else {
      // Does not detect any input stream, and does not create a corresponding
      // av format context yet.
      if (demuxer_thread_->is_stopped()) {
        TEN_LOGW(
            "Giving up to detect any input stream, because the demuxer thread "
            "is stopped.");
        return nullptr;
      } else {
        // The demuxer thread is still running, try again to detect input.
      }
    }
  }

  return input_format_context;
}

bool demuxer_t::analyze_input_stream_() {
  // "avformat_find_stream_info" will take "analyzeduration" time to analyze
  // the input stream, so it will increase the latency. If we can regularize
  // the input stream format, and want to minimize the latency, we can use some
  // fixed logic here instead of calling 'avformat_find_stream_info' to analyze
  // the input stream for us.
  int ffmpeg_rc = avformat_find_stream_info(input_format_context_, nullptr);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      TEN_LOGE("Failed to find input stream info: %s", err_msg);
    }
    return false;
  }

  return true;
}

bool demuxer_t::open_input_stream(const std::string &input_stream_loc) {
  TEN_ASSERT(input_stream_loc.length() > 0, "Invalid argument.");

  if (is_av_decoder_opened_()) {
    TEN_LOGD("Demuxer has already opened.");
    return true;
  }

  input_format_context_ = create_input_format_context_(input_stream_loc);
  if (input_format_context_ == nullptr) {
    return false;
  }

  input_stream_loc_ = input_stream_loc;

  if (!analyze_input_stream_()) {
    return false;
  }

  open_video_decoder_();
  open_audio_decoder_();

  if (!is_av_decoder_opened_()) {
    TEN_LOGW("Failed to find supported A/V codec for %s",
             input_stream_loc_.c_str());
    return false;
  }

  TEN_LOGD("Input stream [%s] is opened.", input_stream_loc_.c_str());

  return true;
}

bool demuxer_t::is_av_decoder_opened_() {
  return video_decoder_ != nullptr || audio_decoder_ != nullptr;
}

AVCodecParameters *demuxer_t::get_video_decoder_params_() const {
  if (input_format_context_ != nullptr && video_stream_idx_ >= 0) {
    return input_format_context_->streams[video_stream_idx_]->codecpar;
  } else {
    return nullptr;
  }
}

AVCodecParameters *demuxer_t::get_audio_decoder_params_() const {
  if (input_format_context_ != nullptr && audio_stream_idx_ >= 0) {
    return input_format_context_->streams[audio_stream_idx_]->codecpar;
  } else {
    return nullptr;
  }
}

bool demuxer_t::create_audio_converter_(const AVFrame *frame) {
  if (audio_converter_ctx_ == nullptr) {
    // Some audio codec (ex: pcm_mclaw) doesn't have channel layout setting.
    uint64_t src_channel_layout =
        frame->channel_layout
            ? frame->channel_layout
            : (uint64_t)av_get_default_channel_layout(frame->channels);

    uint64_t dst_channel_layout = (audio_channel_layout_ != 0)
                                      ? audio_channel_layout_
                                      : src_channel_layout;

    int dst_sample_rate =
        (audio_sample_rate_ != 0) ? audio_sample_rate_ : frame->sample_rate;

    audio_converter_ctx_ = swr_alloc();
    TEN_ASSERT(audio_converter_ctx_, "Failed to create audio resampler");

    av_opt_set_int(audio_converter_ctx_, "in_channel_layout",
                   (int64_t)src_channel_layout, 0);
    av_opt_set_int(audio_converter_ctx_, "out_channel_layout",
                   (int64_t)dst_channel_layout, 0);
    av_opt_set_int(audio_converter_ctx_, "in_sample_rate", frame->sample_rate,
                   0);
    av_opt_set_int(audio_converter_ctx_, "out_sample_rate", dst_sample_rate, 0);
    av_opt_set_sample_fmt(audio_converter_ctx_, "in_sample_fmt",
                          audio_decoder_ctx_->sample_fmt, 0);
    av_opt_set_sample_fmt(audio_converter_ctx_, "out_sample_fmt",
                          TEN_AUDIO_FRAME_SAMPLE_FMT, 0);

    int swr_init_rc = swr_init(audio_converter_ctx_);
    if (swr_init_rc < 0) {
      GET_FFMPEG_ERROR_MESSAGE(err_msg, swr_init_rc) {
        TEN_LOGD("Failed to initialize resampler: %s", err_msg);
      }
      return false;
    }
  }

  return true;
}

std::unique_ptr<ten::audio_frame_t> demuxer_t::to_ten_audio_frame_(
    const AVFrame *frame) {
  TEN_ASSERT(frame, "Invalid argument.");

  if (!create_audio_converter_(frame)) {
    return nullptr;
  }

  int ffmpeg_rc = 0;
  auto audio_frame = ten::audio_frame_t::create("audio_frame");

  // Allocate memory for each audio channel.
  int dst_channels = frame->ch_layout.nb_channels;

  auto length = frame->nb_samples * dst_channels *
                av_get_bytes_per_sample(TEN_AUDIO_FRAME_SAMPLE_FMT);
  audio_frame->alloc_buf(length);

  // Convert this audio frame to the desired audio format.
  uint8_t *out[8] = {nullptr};
  ten::buf_t locked_out_buf = audio_frame->lock_buf();
  out[0] = locked_out_buf.data();

  ffmpeg_rc = swr_convert(audio_converter_ctx_, out, frame->nb_samples,
                          const_cast<const uint8_t **>(frame_->data),  // NOLINT
                          frame_->nb_samples);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      TEN_LOGD("Failed to convert audio samples: %s", err_msg);
    }

    audio_frame->unlock_buf(locked_out_buf);

    return nullptr;
  }

  audio_frame->unlock_buf(locked_out_buf);

  // The amount of the converted samples might be less than the expected,
  // because they might be queued in the swr.
  int actual_audio_samples_cnt = ffmpeg_rc;

  audio_frame->set_data_fmt(TEN_AUDIO_FRAME_DATA_FMT_INTERLEAVE);
  audio_frame->set_bytes_per_sample(
      av_get_bytes_per_sample(TEN_AUDIO_FRAME_SAMPLE_FMT));
  audio_frame->set_sample_rate(frame->sample_rate);
  audio_frame->set_channel_layout(frame->channel_layout);
  audio_frame->set_number_of_channels(frame->ch_layout.nb_channels);
  audio_frame->set_samples_per_channel(actual_audio_samples_cnt);

  AVRational time_base =
      input_format_context_->streams[audio_stream_idx_]->time_base;
  int64_t start_time =
      input_format_context_->streams[audio_stream_idx_]->start_time;
  if (frame->best_effort_timestamp < start_time) {
    TEN_LOGD("Audio timestamp=%" PRId64 " < start_time=%" PRId64 "!",
             frame->best_effort_timestamp, start_time);
  }

  audio_frame->set_timestamp(
      av_rescale(frame->best_effort_timestamp - start_time,
                 static_cast<int64_t>(time_base.num) * 1000, time_base.den));

  return audio_frame;
}

bool demuxer_t::create_video_converter_(int width, int height) {
  if (video_converter_ctx_ == nullptr) {
    video_converter_ctx_ = sws_getContext(
        width, height, video_decoder_ctx_->pix_fmt, width, height,
        TEN_VIDEO_FRAME_PIXEL_FMT, SWS_POINT, nullptr, nullptr, nullptr);
    if (video_converter_ctx_ == nullptr) {
      TEN_LOGD("Failed to create converter context for video frame.");
      return false;
    }
  }

  return true;
}

// Debug purpose only.
TEN_UNUSED static void save_avframe(const AVFrame *avFrame) {
  FILE *fDump = fopen("decode", "ab");

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
void save_img_frame(ten::video_frame_t &pFrame, int index) {
  FILE *pFile = nullptr;
  char szFilename[32];
  int y = 0;

  int width = pFrame.get_width();
  int height = pFrame.get_height();

  // Open file
  (void)snprintf(szFilename, 32, "frame%d.ppm", index);
  pFile = fopen(szFilename, "wb");

  if (pFile == nullptr) {
    return;
  }

  // Write header
  (void)fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  // Write pixel data
  ten::buf_t locked_buf = pFrame.lock_buf();
  for (y = 0; y < height; y++) {
    (void)fwrite(locked_buf.data() + y * width * 3, 1, width * 3, pFile);
  }
  pFrame.unlock_buf(locked_buf);

  // Close file
  (void)fclose(pFile);
}

std::unique_ptr<ten::video_frame_t> demuxer_t::to_ten_video_frame_(
    const AVFrame *frame) {
  TEN_ASSERT(frame, "Invalid argument.");

  int frame_width = frame->width;
  int frame_height = frame->height;
  AVRational video_time_base =
      input_format_context_->streams[video_stream_idx_]->time_base;
  int64_t video_start_time =
      input_format_context_->streams[video_stream_idx_]->start_time;

  if (frame->best_effort_timestamp < video_start_time) {
    TEN_LOGI("Video timestamp=%" PRId64 " < start_time=%" PRId64 "!",
             frame->best_effort_timestamp, video_start_time);
  }

  auto ten_video_frame = ten::video_frame_t::create("video_frame");

  if (frame->format == AV_PIX_FMT_YUV420P ||
      frame->format == AV_PIX_FMT_YUVJ420P) {
    ten_video_frame->set_pixel_fmt(TEN_PIXEL_FMT_I420);
    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, frame_width,
                                        frame_height, 16);

    ten_video_frame->set_width(frame_width);
    ten_video_frame->set_height(frame_height);
    ten_video_frame->set_timestamp(av_rescale(
        frame->best_effort_timestamp - video_start_time,
        static_cast<int64_t>(video_time_base.num) * 1000, video_time_base.den));

    ten_video_frame->alloc_buf(size + 32);
    ten::buf_t locked_buf = ten_video_frame->lock_buf();

    auto *y_data = locked_buf.data();
    auto *u_data = y_data + frame_width * frame_height;
    auto *v_data = u_data + frame_width * frame_height / 4;

    av_image_copy_plane(y_data, frame_width, frame->data[0], frame->linesize[0],
                        frame_width, frame_height);
    av_image_copy_plane(u_data, frame_width / 2, frame->data[1],
                        frame->linesize[1], frame_width / 2, frame_height / 2);
    av_image_copy_plane(v_data, frame_width / 2, frame->data[2],
                        frame->linesize[2], frame_width / 2, frame_height / 2);

    ten_video_frame->unlock_buf(locked_buf);

    // convert YUV to RGB first.

    // if (!create_video_converter_(frame_width, frame_height)) {
    //   return ten::video_frame_t();
    // }
    // // sws_scale require 16 bytes aligned.
    // int dst_linesize = ((frame_width * 3 + 15) / 16) * 16;
    // int size = av_image_get_buffer_size(TEN_VIDEO_FRAME_PIXEL_FMT,
    // frame_width,
    //                                     frame_height, 16);

    // ten_video_frame->set_width(frame_width);
    // ten_video_frame->set_height(frame_height);
    // ten_video_frame->set_timestamp(av_rescale(
    //     frame->best_effort_timestamp - video_start_time,
    //     static_cast<int64_t>(video_time_base.num) * 1000,
    //     video_time_base.den));
    // ten_video_frame->set_buf_size(size);
    // ten_video_frame->set_buf(static_cast<uint8_t*>(TEN_MALLOC(size + 32)));

    // uint8_t* rgb_data[1] = {ten_video_frame->get_buf()};  // NOLINT
    // int rgb_linesize[1] = {dst_linesize};                // NOLINT

    // sws_scale(video_converter_ctx_, static_cast<uint8_t*
    // const*>(frame->data),
    //           frame->linesize, 0, frame_height, rgb_data, rgb_linesize);

    // // DEBUG
    // // save_avframe(frame);

    // // There would be paddings in the destination image from sws_scale()
    // // when the width of the source image is not multiple of 16 (ex:
    // 854x480).
    // // We use memmove() to eliminate these paddings.
    // if (dst_linesize != frame_width * 3) {
    //   // There are paddings at each row, do "dst" => "packed".

    //   // sws_scale's output, with padding.
    //   uint8_t* dst_ptr = ten_video_frame->get_buf() + dst_linesize;

    //   int packed_linesize = frame_width * 3;

    //   // Packed result, no padding.
    //   uint8_t* packed_ptr = ten_video_frame->get_buf() + packed_linesize;

    //   int h = frame_height - 1;
    //   for (; h > 0; h--) {
    //     memmove(packed_ptr, dst_ptr, packed_linesize);
    //     dst_ptr += dst_linesize;
    //     packed_ptr += packed_linesize;
    //   }

    //   ten_video_frame->set_buf_size(static_cast<int64_t>(packed_linesize) *
    //                                frame_height);
    // }
  } else if (frame->format == AV_PIX_FMT_RGB24) {
    ten_video_frame->set_pixel_fmt(TEN_PIXEL_FMT_RGB24);
    ten_video_frame->set_width(frame_width);
    ten_video_frame->set_height(frame_height);
    ten_video_frame->set_timestamp(av_rescale(
        frame->best_effort_timestamp - video_start_time,
        static_cast<int64_t>(video_time_base.num) * 1000, video_time_base.den));

    ten_video_frame->alloc_buf(
        static_cast<int64_t>(frame_width) * frame_height * 3 + 32);
    ten::buf_t locked_buf = ten_video_frame->lock_buf();

    av_image_copy_plane(locked_buf.data(), frame_width * 3, frame->data[0],
                        frame->linesize[0], frame_width * 3, frame_height);

    ten_video_frame->unlock_buf(locked_buf);
  } else {
    TEN_LOGD("Input video frame format(%d) is not YUV420P(%d) nor RGB24(%d)",
             frame->format, AV_PIX_FMT_YUV420P, AV_PIX_FMT_RGB24);
  }

  return ten_video_frame;
}

bool demuxer_t::decode_next_video_packet_(DECODE_STATUS &decode_status) {
  int ffmpeg_rc = avcodec_send_packet(video_decoder_ctx_, packet_);
  if (ffmpeg_rc != 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      TEN_LOGD("Failed to decode a video packet: %s", err_msg);
    }

    decode_status = DECODE_STATUS_ERROR;
    return false;
  }

  ffmpeg_rc = avcodec_receive_frame(video_decoder_ctx_, frame_);
  if (ffmpeg_rc == AVERROR(EAGAIN)) {
    TEN_LOGD("Need more data to decode a video frame.");
    return true;
  } else if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      TEN_LOGW("Failed to decode a video frame: %s", err_msg);
    }

    decode_status = DECODE_STATUS_ERROR;
    return false;
  } else {
    auto video_frame = to_ten_video_frame_(frame_);
    if (video_frame != nullptr) {
      // DEBUG
      // save_img_frame(video_frame, frame_->pts);
      auto video_frame_shared =
          std::make_shared<std::unique_ptr<ten::video_frame_t>>(
              std::move(video_frame));

      ten_env_proxy_->notify([video_frame_shared](ten::ten_env_t &ten_env) {
        ten_env.send_video_frame(std::move(*video_frame_shared));
      });
    }

    decode_status = DECODE_STATUS_SUCCESS;
    return false;
  }
}

bool demuxer_t::decode_next_audio_packet_(DECODE_STATUS &decode_status) {
  int ffmpeg_rc = avcodec_send_packet(audio_decoder_ctx_, packet_);

  // Skip invalid mp3 packet/frame.
  if ((AV_CODEC_ID_MP3 == audio_decoder_ctx_->codec_id) &&
      ffmpeg_rc == AVERROR_INVALIDDATA) {
    TEN_LOGD("mp3 header is missing and lookup next packet.");
    return true;
  } else if (ffmpeg_rc != 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      TEN_LOGE("Failed to decode audio packet: %s", err_msg);
    }

    decode_status = DECODE_STATUS_ERROR;
    return false;
  }

  // It's possible that there is more than one frame in a packet, so the
  // following while loop is used to handle all these frames in one packet.
  while (true) {
    ffmpeg_rc = avcodec_receive_frame(audio_decoder_ctx_, frame_);

    if (ffmpeg_rc == AVERROR(EAGAIN)) {
      TEN_LOGD("Need more data to decode audio frame");
      return true;
    } else if (ffmpeg_rc < 0) {
      GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
        TEN_LOGE("Failed to decode audio frame: %s", err_msg);
      }

      decode_status = DECODE_STATUS_ERROR;
      return false;
    } else {
      auto audio_frame = to_ten_audio_frame_(frame_);
      if (audio_frame != nullptr) {
        auto audio_frame_shared =
            std::make_shared<std::unique_ptr<ten::audio_frame_t>>(
                std::move(audio_frame));

        ten_env_proxy_->notify([audio_frame_shared](ten::ten_env_t &ten_env) {
          ten_env.send_audio_frame(std::move(*audio_frame_shared));
        });
      }

      decode_status = DECODE_STATUS_SUCCESS;
      return false;
    }
  }
}

DECODE_STATUS demuxer_t::decode_next_packet() {
  if (!is_av_decoder_opened_()) {
    TEN_LOGD("Must open stream first.");
    return DECODE_STATUS_ERROR;
  }

  DECODE_STATUS decode_status = DECODE_STATUS_SUCCESS;

  while (true) {
    av_packet_unref(packet_);

    interrupt_cb_param_->last_time = time(nullptr);
    int ffmpeg_rc = av_read_frame(input_format_context_, packet_);
    if (ffmpeg_rc < 0) {
      if (ffmpeg_rc == AVERROR_EOF) {
        flush_remaining_video_frames();
        flush_remaining_audio_frames();
        return DECODE_STATUS_EOF;
      } else {
        GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
          TEN_LOGE("Failed to get frame from input: %s", err_msg);
        }
        return DECODE_STATUS_ERROR;
      }
    }

    if (packet_->stream_index == video_stream_idx_) {
      if (!decode_next_video_packet_(decode_status)) {
        break;
      }
    } else if (packet_->stream_index == audio_stream_idx_) {
      if (!decode_next_audio_packet_(decode_status)) {
        break;
      }
    }
  }

  return decode_status;
}

void demuxer_t::flush_remaining_audio_frames() {
  int ffmpeg_rc = avcodec_send_packet(audio_decoder_ctx_, nullptr);

  // Skip invalid mp3 packet/frame.
  if ((AV_CODEC_ID_MP3 == audio_decoder_ctx_->codec_id) &&
      ffmpeg_rc == AVERROR_INVALIDDATA) {
    TEN_LOGD("mp3 header is missing and lookup next packet.");
    return;
  } else if (ffmpeg_rc != 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      TEN_LOGE("Failed to decode audio packet when flushing: %s", err_msg);
    }
    return;
  }

  while (ffmpeg_rc >= 0) {
    ffmpeg_rc = avcodec_receive_frame(audio_decoder_ctx_, frame_);

    if (ffmpeg_rc == AVERROR(EAGAIN)) {
      TEN_LOGD("Need more data to decode audio frame when flushing");
      ffmpeg_rc = 0;
      continue;
    } else if (ffmpeg_rc < 0) {
      GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
        TEN_LOGE("Failed to decode audio frame when flushing: %s", err_msg);
      }
      return;
    } else {
      auto audio_frame = to_ten_audio_frame_(frame_);
      if (audio_frame != nullptr) {
        auto audio_frame_shared =
            std::make_shared<std::unique_ptr<ten::audio_frame_t>>(
                std::move(audio_frame));

        ten_env_proxy_->notify([audio_frame_shared](ten::ten_env_t &ten_env) {
          ten_env.send_audio_frame(std::move(*audio_frame_shared));
        });
      }
    }
  }
}

void demuxer_t::flush_remaining_video_frames() {
  int ffmpeg_rc = avcodec_send_packet(video_decoder_ctx_, nullptr);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      TEN_LOGE("Failed to decode a video when flushing packet: %s", err_msg);
    }
    return;
  }

  while (ffmpeg_rc >= 0) {
    ffmpeg_rc = avcodec_receive_frame(video_decoder_ctx_, frame_);
    if (ffmpeg_rc == AVERROR(EAGAIN)) {
      TEN_LOGD("Need more data to decode a video frame when flushing.");
      ffmpeg_rc = 0;
      continue;
    } else if (ffmpeg_rc < 0) {
      GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
        TEN_LOGW("Failed to decode a video frame when flushing: %s", err_msg);
      }
      return;
    } else {
      auto video_frame = to_ten_video_frame_(frame_);
      if (video_frame != nullptr) {
        auto video_frame_shared =
            std::make_shared<std::unique_ptr<ten::video_frame_t>>(
                std::move(video_frame));

        ten_env_proxy_->notify([video_frame_shared](ten::ten_env_t &ten_env) {
          ten_env.send_video_frame(std::move(*video_frame_shared));
        });
      }
    }
  }
}

void demuxer_t::dump_video_info_() {
  av_dump_format(input_format_context_, video_stream_idx_,
                 input_stream_loc_.c_str(), 0);

  TEN_LOGD("v:width           %d", video_decoder_ctx_->width);
  TEN_LOGD("v:height          %d", video_decoder_ctx_->height);
  TEN_LOGD("v:time_base:      %d/%d",
           input_format_context_->streams[video_stream_idx_]->time_base.num,
           input_format_context_->streams[video_stream_idx_]->time_base.den);
  TEN_LOGD("v:start_time:     %" PRId64,
           input_format_context_->streams[video_stream_idx_]->start_time);
  TEN_LOGD("v:duration:       %" PRId64,
           input_format_context_->streams[video_stream_idx_]->duration);
  TEN_LOGD("v:nb_frames:      %" PRId64,
           input_format_context_->streams[video_stream_idx_]->nb_frames);
  TEN_LOGD(
      "v:avg_frame_rate: %d/%d",
      input_format_context_->streams[video_stream_idx_]->avg_frame_rate.num,
      input_format_context_->streams[video_stream_idx_]->avg_frame_rate.den);
}

void demuxer_t::dump_audio_info_() {
  av_dump_format(input_format_context_, audio_stream_idx_,
                 input_stream_loc_.c_str(), 0);

  TEN_LOGD("a:time_base:      %d/%d",
           input_format_context_->streams[audio_stream_idx_]->time_base.num,
           input_format_context_->streams[audio_stream_idx_]->time_base.den);
  TEN_LOGD("a:start_time:     %" PRId64,
           input_format_context_->streams[audio_stream_idx_]->start_time);
  TEN_LOGD("a:duration:       %" PRId64,
           input_format_context_->streams[audio_stream_idx_]->duration);
  TEN_LOGD("a:nb_frames:      %" PRId64,
           input_format_context_->streams[audio_stream_idx_]->nb_frames);
}

int demuxer_t::width() const {
  return video_decoder_ctx_ != nullptr
             ? ((rotate_degree_ == 0 || rotate_degree_ == 180)
                    ? video_decoder_ctx_->width
                    : video_decoder_ctx_->height)
             : 0;
}

int demuxer_t::height() const {
  return video_decoder_ctx_ != nullptr
             ? ((rotate_degree_ == 0 || rotate_degree_ == 180)
                    ? video_decoder_ctx_->height
                    : video_decoder_ctx_->width)
             : 0;
}

int64_t demuxer_t::bit_rate() const {
  return video_decoder_ctx_ != nullptr ? video_decoder_ctx_->bit_rate : 0;
}

AVRational demuxer_t::frame_rate() const {
  if ((input_format_context_ != nullptr) && video_stream_idx_ >= 0) {
    return input_format_context_->streams[video_stream_idx_]->avg_frame_rate;
  } else {
    return (AVRational){0, 1};
  }
}

AVRational demuxer_t::video_time_base() const {
  if ((input_format_context_ != nullptr) && video_stream_idx_ >= 0) {
    return input_format_context_->streams[video_stream_idx_]->time_base;
  } else {
    return (AVRational){0, 1};
  }
}

AVRational demuxer_t::audio_time_base() const {
  if ((input_format_context_ != nullptr) && audio_stream_idx_ >= 0) {
    return input_format_context_->streams[audio_stream_idx_]->time_base;
  } else {
    return (AVRational){0, 1};
  }
}

int64_t demuxer_t::number_of_frames() const {
  if ((input_format_context_ != nullptr) && video_stream_idx_ >= 0) {
    return input_format_context_->streams[video_stream_idx_]->nb_frames;
  } else {
    return 0;
  }
}

void demuxer_t::open_video_decoder_() {
  TEN_ASSERT(input_format_context_, "Invalid argument.");

  int idx = av_find_best_stream(input_format_context_, AVMEDIA_TYPE_VIDEO, -1,
                                -1, &video_decoder_, 0);
  if (idx < 0) {
    TEN_LOGW(
        "Failed to create video decoder, because video input stream not "
        "found.");
    return;
  }
  video_stream_idx_ = idx;

  if (video_decoder_ == nullptr) {
    TEN_LOGE(
        "Video input stream is found, but failed to create input video "
        "decoder, it might because the input video codec is not supported.");
    return;
  }

  AVCodecParameters *params = get_video_decoder_params_();
  TEN_ASSERT(params, "Invalid argument.");

  video_decoder_ctx_ = avcodec_alloc_context3(video_decoder_);
  if ((video_decoder_ctx_ == nullptr) ||
      avcodec_parameters_to_context(video_decoder_ctx_, params) < 0) {
    TEN_LOGE("Failed to create video decoder context.");
    return;
  }

  int ffmpeg_rc = avcodec_open2(video_decoder_ctx_, video_decoder_, nullptr);
  if (ffmpeg_rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, ffmpeg_rc) {
      TEN_LOGE("Failed to bind video decoder to video decoder context: %s",
               err_msg);
    }
    return;
  }

  dump_video_info_();  // For debug.

  TEN_LOGD("Successfully open '%s' video decoder for input stream %d",
           video_decoder_->name, video_stream_idx_);

  // Check metadata for rotation.
  AVDictionary *metadata =
      input_format_context_->streams[video_stream_idx_]->metadata;
  AVDictionaryEntry *tag =
      av_dict_get(metadata, "", nullptr, AV_DICT_IGNORE_SUFFIX);
  while (tag != nullptr) {
    TEN_LOGD("metadata: %s = %s", tag->key, tag->value);
    if (strcmp(tag->key, "rotate") == 0) {
      rotate_degree_ = (int)strtol(tag->value, nullptr, 10);
      TEN_LOGD("Found rotate = %d in video stream.", rotate_degree_);
    }
    tag = av_dict_get(metadata, "", tag, AV_DICT_IGNORE_SUFFIX);
  }
}

void demuxer_t::open_audio_decoder_() {
  int idx = av_find_best_stream(input_format_context_, AVMEDIA_TYPE_AUDIO, -1,
                                -1, &audio_decoder_, 0);
  if (idx < 0) {
    TEN_LOGW(
        "Failed to create audio decoder, because audio input stream not "
        "found.");
    return;
  }
  audio_stream_idx_ = idx;

  if (audio_decoder_ == nullptr) {
    TEN_LOGE(
        "Audio input stream is found, but failed to create input audio "
        "decoder, it might because the input audio codec is not supported.");
    return;
  }

  AVCodecParameters *params = get_audio_decoder_params_();
  audio_decoder_ctx_ = avcodec_alloc_context3(audio_decoder_);
  if ((audio_decoder_ctx_ == nullptr) ||
      avcodec_parameters_to_context(audio_decoder_ctx_, params) < 0) {
    TEN_LOGD("Failed to create audio decoder context.");
    return;
  }

  int rc = avcodec_open2(audio_decoder_ctx_, audio_decoder_, nullptr);
  if (rc < 0) {
    GET_FFMPEG_ERROR_MESSAGE(err_msg, rc) {
      TEN_LOGE("Failed to bind audio decoder to audio decoder context: %s",
               err_msg);
    }
    return;
  }

  dump_audio_info_();  // For debug.

  TEN_LOGD("Successfully open '%s' audio decoder for input stream %d",
           audio_decoder_->name, audio_stream_idx_);

  audio_sample_rate_ = params->sample_rate;
  audio_num_of_channels_ = params->ch_layout.nb_channels;

  // NOLINTNEXTLINE(cert-dcl03-c,hicpp-static-assert,misc-static-assert)
  audio_channel_layout_ = params->channel_layout;
  if (!audio_channel_layout_) {
    // some audio codec (ex: pcm_mclaw) doesn't have channel layout setting.
    int64_t default_change_layout =
        av_get_default_channel_layout(params->channels);
    TEN_ASSERT(default_change_layout >= 0, "Should not happen.");
    audio_channel_layout_ = (uint64_t)default_change_layout;
  }
}

}  // namespace ffmpeg_extension
}  // namespace ten
