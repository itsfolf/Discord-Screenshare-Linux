cmake_minimum_required(VERSION 3.4)

# Name of the project (will be the name of the plugin)
project (discord-pawgers)

set(CMAKE_CXX_STANDARD 17)
# Don't add this line if you will try_compile with boost.
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Essential include files to build a node addon,
# you should add this line in every CMake.js based project.
include_directories(${CMAKE_JS_INC})

# Declare the location of the source files
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h" "src/*.cc")


if(NOT EXISTS "${CMAKE_SOURCE_DIR}/lib/libfrida-core.a")
    file(DOWNLOAD
         https://github.com/frida/frida/releases/download/15.2.2/frida-core-devkit-15.2.2-linux-x86_64.tar.xz
         ${CMAKE_SOURCE_DIR}/lib/libfrida-core.tar.xz
         EXPECTED_HASH SHA512=8c49cc6efcb32c54fbec557d0afe8bd3dc7f120c11ff6f06a5d60a5f2bc141f25ee920d510826d428d1a7b8b03c15417ab2b66b51960b990a017f3a00a4fcfea
         SHOW_PROGRESS)
    execute_process(COMMAND tar -xf ${CMAKE_SOURCE_DIR}/lib/libfrida-core.tar.xz libfrida-core.a
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
endif()
# This line will tell CMake that we're building a shared library
# from the above source files
# named after the project's name
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

# This line will give our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

if (DEFINED ENV{DEBUG_AUTO_INSTALL})
  execute_process(COMMAND /usr/bin/logname
                  OUTPUT_VARIABLE CURRENT_USER
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  file(GLOB DISCORD_VOICE "/home/${CURRENT_USER}/.config/discordcanary/*/modules/discord_voice/")
  list(GET DISCORD_VOICE 0 DISCORD_VOICE)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
    COMMAND "${CMAKE_COMMAND}" -E copy
       "$<TARGET_FILE:${PROJECT_NAME}>"
      "${DISCORD_VOICE}" 
       && echo "Copied to discord_voice directory."
    COMMENT "Copying to discord_voice directory")
endif()

# Essential library files to link to a node addon,
# you should add this line in every CMake.js based project.
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})


# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

add_definitions(-DNAPI_VERSION=3)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET gio-2.0)

target_compile_definitions(discord-pawgers PUBLIC WEBRTC_USE_X11 PUBLIC WEBRTC_USE_PIPEWIRE WEBRTC_POSIX)
target_include_directories(${PROJECT_NAME} PRIVATE include ${deps_INCLUDE_DIRS})
add_library(libwebrtc STATIC IMPORTED)
add_dependencies(discord-pawgers libwebrtc)
add_library(frida-core STATIC IMPORTED)
add_dependencies(discord-pawgers frida-core)
target_link_directories(discord-pawgers PUBLIC lib PUBLIC ${GLIB_LIBRARY_DIRS})
find_package(X11 REQUIRED)
target_link_libraries(discord-pawgers libwebrtc.a libfrida-core.a ${deps_LIBRARIES} ${X11_LIBRARIES} Xfixes Xrandr Xcomposite elf)