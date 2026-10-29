#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;
struct AVPacket;
struct AVFrame;

namespace sienna::audio {

struct TrackInfo {
  std::string title;
  std::string artist;
  std::string album;
  std::string albumArtist;
  int track = 0;
  int disc = 0;
  float duration = 0.f;  // seconds
};

// reads tags and length without decoding audio.
bool readInfo(const std::string& path, TrackInfo& out);
// pulls the embedded cover image (jpeg or png bytes), if the file has one.
bool readCoverArt(const std::string& path, std::vector<std::uint8_t>& out);

// decodes any supported file to 48kHz stereo 16-bit.
class Decoder {
public:
  Decoder() = default;
  ~Decoder();
  Decoder(const Decoder&) = delete;
  Decoder& operator=(const Decoder&) = delete;

  bool open(const std::string& path);
  void close();
  bool isOpen() const { return fmt_ != nullptr; }

  // returns frames written; fewer than asked (or 0) means the track ended.
  int read(std::int16_t* out, int frames);
  bool seek(double seconds);
  double duration() const;

private:
  bool decodeMore();

  AVFormatContext* fmt_ = nullptr;
  AVCodecContext* codec_ = nullptr;
  SwrContext* swr_ = nullptr;
  AVPacket* packet_ = nullptr;
  AVFrame* frame_ = nullptr;
  int stream_ = -1;
  bool draining_ = false;
  bool done_ = false;
  std::vector<std::int16_t> pending_;
  std::size_t pendingPos_ = 0;
};

}  // namespace sienna::audio
