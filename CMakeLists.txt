cmake_minimum_required(VERSION 3.16)

project(sienna VERSION 0.1.0 LANGUAGES C CXX)

if(VITA)
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)

set(SIENNA_SOURCES
  src/main.cpp
  src/audio/decoder.cpp
  src/core/fs.cpp
  src/core/input.cpp
  src/gfx/draw.cpp
  src/gfx/text.cpp
  src/gfx/wrap.cpp
  src/ui/now_playing.cpp
  src/ui/dashboard/dashboard.cpp
  src/ui/widgets/hints.cpp
  src/ui/widgets/menu.cpp
  src/ui/widgets/panel.cpp
  src/ui/widgets/prompt.cpp
  src/modules/placeholder_section.cpp
  src/modules/music/art_cache.cpp
  src/modules/music/library.cpp
  src/modules/music/music_section.cpp
  src/modules/music/player.cpp
  src/modules/music/playlists.cpp
  src/modules/music/track.cpp
  src/modules/notes/browser.cpp
  src/modules/notes/editor.cpp
  src/modules/notes/markdown.cpp
  src/modules/notes/notes_section.cpp
  src/modules/notes/notes_store.cpp
)

if(VITA)
  list(APPEND SIENNA_SOURCES src/core/platform_vita.cpp src/audio/output_vita.cpp)
else()
  list(APPEND SIENNA_SOURCES src/core/platform_desktop.cpp src/audio/output_sdl.cpp)
endif()

add_executable(sienna ${SIENNA_SOURCES})
target_include_directories(sienna PRIVATE src)
target_compile_options(sienna PRIVATE -Wall -Wextra)

if(VITA)
  # built by scripts/build_ffmpeg_vita.sh.
  set(FFMPEG_VITA ${CMAKE_SOURCE_DIR}/build/ffmpeg-vita)
  if(NOT EXISTS ${FFMPEG_VITA}/lib/libavformat.a)
    message(FATAL_ERROR "run scripts/build_ffmpeg_vita.sh first")
  endif()
  target_include_directories(sienna PRIVATE ${FFMPEG_VITA}/include)

  # static libraries don't carry their own dependencies, so they're listed after them.
  target_link_libraries(sienna PRIVATE
    SDL2_image::SDL2_image-static SDL2_ttf::SDL2_ttf-static SDL2::SDL2main SDL2::SDL2-static
    ${FFMPEG_VITA}/lib/libavformat.a
    ${FFMPEG_VITA}/lib/libavcodec.a
    ${FFMPEG_VITA}/lib/libswresample.a
    ${FFMPEG_VITA}/lib/libavutil.a
    freetype webp sharpyuv jpeg png bz2 z pthread m
    SceAppMgr_stub SceAudio_stub ScePower_stub
  )
  # libstdc++ only treats threads as enabled when pthread_cancel is linked in. without it
  # std::thread throws at startup and std::mutex silently does nothing.
  target_link_options(sienna PRIVATE -Wl,-u,pthread_cancel)

  set(VITA_APP_NAME "SIENNA")
  set(VITA_TITLEID "SIEN00001")
  set(VITA_VERSION "01.00")
  # keep running when the user leaves the app (0x8) and skip system auto-sleep (0x80000),
  # so music plays on in the background. 0x8000 is the SDK default.
  set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE=0x88008")

  vita_create_self(eboot.bin sienna)
  vita_create_vpk(sienna.vpk ${VITA_TITLEID} eboot.bin
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE sce_sys/icon0.png sce_sys/icon0.png
    FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
    FILE res res
  )
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libswresample libavutil)

  if(TARGET SDL2::SDL2main)
    target_link_libraries(sienna PRIVATE SDL2::SDL2main)
  endif()
  target_link_libraries(sienna PRIVATE
    SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf SDL2::SDL2 PkgConfig::FFMPEG
  )

  add_custom_command(TARGET sienna POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:sienna>/res
  )
endif()
