#include "audio/decoder.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "audio/output.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace sienna::audio {

namespace {

// tags live on the container for most formats, but on the stream for ogg and opus.
std::string tag(AVFormatContext* fmt, int stream, const char* key) {
  const AVDictionaryEntry* e = av_dict_get(fmt->metadata, key, nullptr, 0);
  if (!e && stream >= 0) {
    e = av_dict_get(fmt->streams[stream]->metadata, key, nullptr, 0);
  }
  return e && e->value ? std::string(e->value) : std::string();
}

int leadingNumber(const std::string& s) {
  return s.empty() ? 0 : std::atoi(s.c_str());
}

AVFormatContext* openFormat(const std::string& path) {
  AVFormatContext* fmt = nullptr;
  if (avformat_open_input(&fmt, path.c_str(), nullptr, nullptr) < 0) {
    return nullptr;
  }
  if (avformat_find_stream_info(fmt, nullptr) < 0) {
    avformat_close_input(&fmt);
    return nullptr;
  }
  return fmt;
}

}  // namespace

bool readInfo(const std::string& path, TrackInfo& out) {
  AVFormatContext* fmt = openFormat(path);
  if (!fmt) {
    return false;
  }
  const int stream = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  if (stream < 0) {
    avformat_close_input(&fmt);
    return false;
  }
  out.title = tag(fmt, stream, "title");
  out.artist = tag(fmt, stream, "artist");
  out.album = tag(fmt, stream, "album");
  out.albumArtist = tag(fmt, stream, "album_artist");
  out.track = leadingNumber(tag(fmt, stream, "track"));
  out.disc = leadingNumber(tag(fmt, stream, "disc"));
  out.duration = fmt->duration != AV_NOPTS_VALUE
                     ? static_cast<float>(fmt->duration) / AV_TIME_BASE
                     : 0.f;
  avformat_close_input(&fmt);
  return true;
}

bool readCoverArt(const std::string& path, std::vector<std::uint8_t>& out) {
  AVFormatContext* fmt = nullptr;
  if (avformat_open_input(&fmt, path.c_str(), nullptr, nullptr) < 0) {
    return false;
  }
  bool found = false;
  for (unsigned i = 0; i < fmt->nb_streams; ++i) {
    const AVStream* st = fmt->streams[i];
    if ((st->disposition & AV_DISPOSITION_ATTACHED_PIC) && st->attached_pic.size > 0) {
      out.assign(st->attached_pic.data, st->attached_pic.data + st->attached_pic.size);
      found = true;
      break;
    }
  }
  avformat_close_input(&fmt);
  return found;
}

Decoder::~Decoder() {
  close();
}

bool Decoder::open(const std::string& path) {
  close();
  fmt_ = openFormat(path);
  if (!fmt_) {
    return false;
  }
  const AVCodec* codec = nullptr;
  stream_ = av_find_best_stream(fmt_, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
  if (stream_ < 0 || !codec) {
    close();
    return false;
  }
  codec_ = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(codec_, fmt_->streams[stream_]->codecpar);
  if (avcodec_open2(codec_, codec, nullptr) < 0) {
    close();
    return false;
  }

  AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
  if (swr_alloc_set_opts2(&swr_, &outLayout, AV_SAMPLE_FMT_S16, kSampleRate, &codec_->ch_layout,
                          codec_->sample_fmt, codec_->sample_rate, 0, nullptr) < 0 ||
      swr_init(swr_) < 0) {
    close();
    return false;
  }

  packet_ = av_packet_alloc();
  frame_ = av_frame_alloc();
  draining_ = false;
  done_ = false;
  pending_.clear();
  pendingPos_ = 0;
  return true;
}

void Decoder::close() {
  av_frame_free(&frame_);
  av_packet_free(&packet_);
  swr_free(&swr_);
  avcodec_free_context(&codec_);
  if (fmt_) {
    avformat_close_input(&fmt_);
  }
  stream_ = -1;
  pending_.clear();
  pendingPos_ = 0;
}

double Decoder::duration() const {
  if (!fmt_ || fmt_->duration == AV_NOPTS_VALUE) {
    return 0.0;
  }
  return static_cast<double>(fmt_->duration) / AV_TIME_BASE;
}

// decodes one frame into pending_. returns false once nothing is left.
bool Decoder::decodeMore() {
  while (!done_) {
    const int r = avcodec_receive_frame(codec_, frame_);
    if (r == 0) {
      const int maxOut = swr_get_out_samples(swr_, frame_->nb_samples);
      pending_.resize(static_cast<std::size_t>(maxOut) * kChannels);
      std::uint8_t* outPlanes[1] = {reinterpret_cast<std::uint8_t*>(pending_.data())};
      const int got = swr_convert(swr_, outPlanes, maxOut,
                                  const_cast<const std::uint8_t**>(frame_->extended_data),
                                  frame_->nb_samples);
      av_frame_unref(frame_);
      pending_.resize(static_cast<std::size_t>(std::max(got, 0)) * kChannels);
      pendingPos_ = 0;
      if (got > 0) {
        return true;
      }
      continue;
    }
    if (r == AVERROR_EOF) {
      // whatever the resampler is still holding.
      const int maxOut = swr_get_out_samples(swr_, 0);
      done_ = true;
      if (maxOut > 0) {
        pending_.resize(static_cast<std::size_t>(maxOut) * kChannels);
        std::uint8_t* outPlanes[1] = {reinterpret_cast<std::uint8_t*>(pending_.data())};
        const int got = swr_convert(swr_, outPlanes, maxOut, nullptr, 0);
        pending_.resize(static_cast<std::size_t>(std::max(got, 0)) * kChannels);
        pendingPos_ = 0;
        return got > 0;
      }
      return false;
    }
    if (r != AVERROR(EAGAIN)) {
      done_ = true;
      return false;
    }

    // the codec wants more input.
    if (draining_) {
      done_ = true;
      return false;
    }
    const int rr = av_read_frame(fmt_, packet_);
    if (rr < 0) {
      draining_ = true;
      avcodec_send_packet(codec_, nullptr);
      continue;
    }
    if (packet_->stream_index == stream_) {
      avcodec_send_packet(codec_, packet_);
    }
    av_packet_unref(packet_);
  }
  return false;
}

int Decoder::read(std::int16_t* out, int frames) {
  if (!fmt_) {
    return 0;
  }
  int written = 0;
  while (written < frames) {
    if (pendingPos_ >= pending_.size() && !decodeMore()) {
      break;
    }
    const std::size_t availFrames = (pending_.size() - pendingPos_) / kChannels;
    const std::size_t n = std::min<std::size_t>(availFrames, static_cast<std::size_t>(frames - written));
    std::memcpy(out + static_cast<std::size_t>(written) * kChannels, pending_.data() + pendingPos_,
                n * kChannels * sizeof(std::int16_t));
    pendingPos_ += n * kChannels;
    written += static_cast<int>(n);
  }
  return written;
}

bool Decoder::seek(double seconds) {
  if (!fmt_) {
    return false;
  }
  const std::int64_t ts = static_cast<std::int64_t>(std::max(0.0, seconds) * AV_TIME_BASE);
  if (av_seek_frame(fmt_, -1, ts, AVSEEK_FLAG_BACKWARD) < 0) {
    return false;
  }
  avcodec_flush_buffers(codec_);
  swr_init(swr_);
  pending_.clear();
  pendingPos_ = 0;
  draining_ = false;
  done_ = false;
  return true;
}

}  // namespace sienna::audio
