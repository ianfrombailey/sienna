#!/usr/bin/env bash
# builds a decode-only ffmpeg for the Vita into build/ffmpeg-vita.
# based on the vdpm recipe, plus the mov demuxer so .m4a files open.
set -euo pipefail

: "${VITASDK:?set VITASDK to your VitaSDK install}"

VERSION=9.0.2
SHA256=8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$ROOT/build/ffmpeg-src"
PREFIX="$ROOT/build/ffmpeg-vita"

mkdir -p "$WORK"
cd "$WORK"
if [ ! -f "ffmpeg-$VERSION.tar.xz" ]; then
  curl -fL "https://ffmpeg.org/releases/ffmpeg-$VERSION.tar.xz" -o "ffmpeg-$VERSION.tar.xz"
fi
echo "$SHA256  ffmpeg-$VERSION.tar.xz" | shasum -a 256 -c -
rm -rf "ffmpeg-$VERSION"
tar xf "ffmpeg-$VERSION.tar.xz"
cd "ffmpeg-$VERSION"

./configure --prefix="$PREFIX" \
  --enable-cross-compile \
  --cross-prefix="$VITASDK/bin/arm-vita-eabi-" \
  --arch=armv7-a \
  --cpu=cortex-a9 \
  --target-os=none \
  --disable-shared \
  --enable-static \
  --enable-small \
  --disable-debug \
  --disable-runtime-cpudetect \
  --disable-armv5te \
  --disable-armv6t2 \
  --disable-programs \
  --disable-doc \
  --disable-network \
  --disable-everything \
  --disable-avdevice \
  --disable-avfilter \
  --disable-swscale \
  --enable-decoder=aac,ac3,alac,ape,flac,mp3,mp3float,opus,vorbis,pcm_s16le,pcm_s16be,pcm_s24le,pcm_s24be,pcm_s32le,pcm_f32le,pcm_s8,pcm_u8,tta,wavpack,wmalossless \
  --enable-demuxer=aac,ac3,aiff,ape,flac,matroska,mov,mp3,ogg,tta,wav,wv \
  --enable-parser=aac,ac3,flac,mpegaudio,opus,vorbis \
  --enable-protocol=file \
  --disable-bzlib \
  --disable-iconv \
  --disable-lzma \
  --disable-zlib \
  --disable-sdl2 \
  --disable-securetransport \
  --disable-xlib \
  --enable-pthreads \
  --extra-cflags="-std=gnu11 -O2 -ftree-vectorize -fomit-frame-pointer -ffast-math -D_BSD_SOURCE -Wno-error=implicit-function-declaration -Wno-error=int-conversion -Wno-error=incompatible-pointer-types" \
  --extra-ldflags="-L$VITASDK/lib"

make -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"
make install
echo "ffmpeg for Vita installed to $PREFIX"
