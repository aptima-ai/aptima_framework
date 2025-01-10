//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include <cstdint>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_id.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#include "aptima_runtime/binding/cpp/aptima.h"

namespace aptima {
namespace ffmpeg_extension {

enum ENCODE_STATUS {
  ENCODE_STATUS_SUCCESS,
  ENCODE_STATUS_EOF,
  ENCODE_STATUS_ERROR,
};

class muxer_t {
 public:
  muxer_t();
  ~muxer_t();

  muxer_t(const muxer_t &other) = delete;
  muxer_t(muxer_t &&other) = delete;

  muxer_t &operator=(const muxer_t &other) = delete;
  muxer_t &operator=(muxer_t &&other) = delete;

  bool open(const std::string &dest_name, bool realtime);

  ENCODE_STATUS encode_video_frame(
      std::unique_ptr<aptima::video_frame_t> video_frame);
  ENCODE_STATUS encode_audio_frame(
      std::unique_ptr<aptima::audio_frame_t> audio_frame);

  int64_t next_video_timing();

 private:
  friend class muxer_thread_t;

  bool is_av_encoder_opened_();

  bool open_video_encoder_(bool realtime);
  bool open_audio_encoder_();

  void dump_video_info_();
  void dump_audio_info_();

  int64_t next_video_pts_();
  int64_t next_audio_pts_();

  bool encode_audio_frame_(AVFrame *frame);
  bool encode_audio_silent_frame_();

  void flush_remaining_audio_frames();
  void flush_remaining_video_frames();

  bool allocate_audio_fifo_(AVCodecParameters *encoded_stream_params);
  bool allocate_audio_frame_(AVCodecParameters *encoded_stream_params);

  bool convert_audio_frame_(AVCodecParameters *encoded_stream_params,
                            aptima::audio_frame_t &aptima_audio_frame);
  AVFrame *convert_video_frame_(aptima::video_frame_t &video_frame);

  bool create_audio_converter_(AVCodecParameters *encoded_stream_params,
                               aptima::audio_frame_t &aptima_audio_frame);
  bool create_video_converter_(aptima::video_frame_t &video_frame);

  // @{
  // Source video settings.
  int src_video_width_;
  int src_video_height_;
  int64_t src_video_bit_rate_;
  int64_t src_video_number_of_frames_;
  AVRational src_video_frame_rate_;
  AVRational src_video_time_base_;
  // @}

  // @{
  // Source audio settings.
  int src_audio_sample_rate_;
  AVRational src_audio_time_base_;
  uint64_t src_audio_channel_layout_;
  // @}

  std::string dest_name_;

  AVFormatContext *output_format_ctx_;

  unsigned int video_stream_idx_;
  unsigned int audio_stream_idx_;

  int64_t next_video_idx_;  // Next video frame index in video encoder context.
  int64_t next_audio_idx_;  // Next audio sample index in audio encoder context.

  AVCodecContext *video_encoder_ctx_;
  AVCodecContext *audio_encoder_ctx_;

  const AVCodec *video_encoder_;
  const AVCodec *audio_encoder_;

  AVStream *video_stream_;
  AVStream *audio_stream_;

  struct SwsContext *video_converter_ctx_;
  struct SwrContext *audio_converter_ctx_;

  AVPacket *packet_;
  AVAudioFifo *audio_fifo_;
  AVFrame *audio_frame_;
  uint64_t audio_prepend_pts_;
};

}  // namespace ffmpeg_extension
}  // namespace aptima
