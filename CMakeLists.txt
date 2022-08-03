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

# This line will tell CMake that we're building a shared library
# from the above source files
# named after the project's name
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

# This line will give our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

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
target_include_directories(${PROJECT_NAME} PRIVATE include)
add_library(libwebrtc STATIC IMPORTED)
add_dependencies(discord-pawgers libwebrtc)
link_directories("./lib")
LINK_DIRECTORIES(${GLIB_LIBRARY_DIRS})
find_package(X11 REQUIRED)
target_link_libraries(discord-pawgers ${CMAKE_SOURCE_DIR}/lib/libwebrtc.a ${deps_LIBRARIES} ${X11_LIBRARIES} Xfixes Xrandr Xcomposite)