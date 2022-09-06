cmake_minimum_required(VERSION 3.16)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

project(linux-fix)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h" "src/*.cc")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

if(DEFINED ENV{DEBUG_AUTO_INSTALL})
  execute_process(
    COMMAND /usr/bin/logname
    OUTPUT_VARIABLE CURRENT_USER
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  file(GLOB DISCORD_VOICE
       "/home/${CURRENT_USER}/.config/discordcanary/*/modules/discord_voice/")
  list(GET DISCORD_VOICE 0 DISCORD_VOICE)
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${PROJECT_NAME}>"
            "${DISCORD_VOICE}" && echo "Copied to discord_voice directory."
    COMMENT "Copying to discord_voice directory")
endif()

target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

# Include N-API wrappers
execute_process(
  COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(ELF REQUIRED libelf)
target_include_directories(
  ${PROJECT_NAME} SYSTEM
  PUBLIC ${GIO_INCLUDE_DIRS}
         ${GLIB2_INCLUDE_DIRS}
         ${ELF_INCLUDE_DIRS})
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC ${GIO_LINK_LIBRARIES}
         ${GLIB2_LINK_LIBRARIES}
         ${ELF_LINK_LIBRARIES})

find_package(X11 REQUIRED COMPONENTS Xcomposite Xdamage Xfixes Xrender Xrandr
                                     Xtst)
target_include_directories(
  ${PROJECT_NAME} SYSTEM
  PRIVATE ${X11_X11_INCLUDE_PATH}
          ${X11_Xlib_INCLUDE_PATH}
          ${X11_Xcomposite_INCLUDE_PATH}
          ${X11_Xdamage_INCLUDE_PATH}
          ${X11_Xext_INCLUDE_PATH}
          ${X11_Xfixes_INCLUDE_PATH}
          ${X11_Xrender_INCLUDE_PATH}
          ${X11_Xrandr_INCLUDE_PATH}
          ${X11_Xtst_INCLUDE_PATH})
target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE ${X11_X11_LIB}
          ${X11_Xcomposite_LIB}
          ${X11_Xdamage_LIB}
          ${X11_Xext_LIB}
          ${X11_Xfixes_LIB}
          ${X11_Xrender_LIB}
          ${X11_Xrandr_LIB}
          ${X11_Xtst_LIB})

find_package(
  FFmpeg REQUIRED
  COMPONENTS avcodec
             avfilter
             avdevice
             avutil
             swscale
             avformat
             swresample)
target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE FFmpeg::avcodec
          FFmpeg::avfilter
          FFmpeg::avformat
          FFmpeg::avdevice
          FFmpeg::avutil
          FFmpeg::swscale
          FFmpeg::swresample)

find_package(Libpci REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE LIBPCI::LIBPCI)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_directories(${PROJECT_NAME} PRIVATE lib)
target_link_libraries(${PROJECT_NAME} PRIVATE webrtc)

target_compile_definitions(
  ${PROJECT_NAME} PUBLIC NAPI_VERSION=3 WEBRTC_USE_X11 WEBRTC_USE_PIPEWIRE
                         WEBRTC_POSIX WEBRTC_USE_H264 CONFIG_VAAPI)
