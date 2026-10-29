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

set(SIENNA_SOURCES
  src/main.cpp
  src/core/input.cpp
  src/gfx/draw.cpp
  src/gfx/text.cpp
  src/ui/now_playing.cpp
  src/ui/dashboard/dashboard.cpp
  src/modules/placeholder_section.cpp
)

if(VITA)
  list(APPEND SIENNA_SOURCES src/core/platform_vita.cpp)
else()
  list(APPEND SIENNA_SOURCES src/core/platform_desktop.cpp)
endif()

add_executable(sienna ${SIENNA_SOURCES})
target_include_directories(sienna PRIVATE src)
target_compile_options(sienna PRIVATE -Wall -Wextra)

if(VITA)
  # static freetype doesn't carry its own dependencies, so they're listed after it.
  target_link_libraries(sienna PRIVATE
    SDL2_ttf::SDL2_ttf-static SDL2::SDL2main SDL2::SDL2-static
    freetype png bz2 z
  )

  set(VITA_APP_NAME "SIENNA")
  set(VITA_TITLEID "SIEN00001")
  set(VITA_VERSION "01.00")

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
  if(TARGET SDL2::SDL2main)
    target_link_libraries(sienna PRIVATE SDL2::SDL2main)
  endif()
  target_link_libraries(sienna PRIVATE SDL2_ttf::SDL2_ttf SDL2::SDL2)

  add_custom_command(TARGET sienna POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:sienna>/res
  )
endif()
