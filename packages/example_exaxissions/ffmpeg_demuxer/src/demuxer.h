//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#pragma once

#include <sys/types.h>

#include <cstdint>
#include <string>

#include "axis_runtime/binding/cpp/ten.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_par.h>
#include <libavcodec/packet.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/rational.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

namespace ten {
namespace ffmpeg_extension {

class demuxer_thread_t;

enum DECODE_STATUS {
  DECODE_STATUS_SUCCESS,
  DECODE_STATUS_EOF,
  DECODE_STATUS_ERROR,
};

struct interrupt_cb_param_t {
  time_t last_time;
};

class demuxer_t {
 public:
  demuxer_t(ten::axis_env_proxy_t *axis_env_proxy,
            demuxer_thread_t *demuxer_thread);
  ~demuxer_t();

  demuxer_t(const demuxer_t &other) = delete;
  demuxer_t(demuxer_t &&other) = delete;

  demuxer_t &operator=(const demuxer_t &other) = delete;
  demuxer_t &operator=(demuxer_t &&other) = delete;

  bool open_input_stream(const std::string &input_stream_loc);

  DECODE_STATUS decode_next_packet();

  int width() const;
  int height() const;
  int64_t bit_rate() const;
  AVRational frame_rate() const;
  AVRational video_time_base() const;
  AVRational audio_time_base() const;
  int64_t number_of_frames() const;

 private:
  friend class demuxer_thread_t;

  AVFormatContext *create_input_format_context_internal_(
      const std::string &input_stream_loc);
  AVFormatContext *create_input_format_context_(
      const std::string &input_stream_loc);

  void open_video_decoder_();
  void open_audio_decoder_();

  bool is_av_decoder_opened_();

  void dump_video_info_();
  void dump_audio_info_();

  void flush_remaining_audio_frames();
  void flush_remaining_video_frames();

  bool analyze_input_stream_();

  AVCodecParameters *get_video_decoder_params_() const;
  AVCodecParameters *get_audio_decoder_params_() const;

  bool create_audio_converter_(const AVFrame *frame);
  bool create_video_converter_(int width, int height);

  std::unique_ptr<ten::audio_frame_t> to_axis_audio_frame_(const AVFrame *frame);
  std::unique_ptr<ten::video_frame_t> to_axis_video_frame_(const AVFrame *frame);

  bool decode_next_video_packet_(DECODE_STATUS &decode_status);
  bool decode_next_audio_packet_(DECODE_STATUS &decode_status);

  std::string input_stream_loc_;
  demuxer_thread_t *demuxer_thread_;
  ten::axis_env_proxy_t *axis_env_proxy_;

  // This structure describes the basic information of a media file or media
  // streaming. This is the most basic structure in FFmpeg, which is the root of
  // all other structures. It is the fundamental abstract of a media file or
  // streaming.
  AVFormatContext *input_format_context_;

  interrupt_cb_param_t *interrupt_cb_param_;

  int video_stream_idx_;
  int audio_stream_idx_;

  AVCodecContext *video_decoder_ctx_;
  AVCodecContext *audio_decoder_ctx_;

  const AVCodec *video_decoder_;
  const AVCodec *audio_decoder_;

  struct SwsContext *video_converter_ctx_;
  SwrContext *audio_converter_ctx_;

  AVPacket *packet_;
  AVFrame *frame_;

  int rotate_degree_;
  int audio_sample_rate_;
  uint64_t audio_channel_layout_;
  int audio_num_of_channels_;
};

}  // namespace ffmpeg_extension
}  // namespace ten
