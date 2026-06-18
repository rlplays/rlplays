project(editor CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# No longer using flat buffers:
# include(CoreLoopFlatBuffers.cmake)


# These two lines can be swapped with other gameplays.
include_directories(../plays)
include_directories(../coreloop)
include_directories(../coreloop/include)
include_directories(../thirdparty/jsoncpp/include)

file(GLOB_RECURSE PLAYS_FILES CONFIGURE_DEPENDS ../plays/*.h ../plays/*.cpp)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp ${PLAYS_FILES})
# Add include directory for raylibimgui
include_directories(../thirdparty/raylibimgui)
include_directories(../thirdparty/raylibimgui/extras)
include_directories(../thirdparty/imgui)
add_subdirectory(../thirdparty/imgui ../thirdparty/imgui/)
# Find all cpp files in the raylibimgui directory
file(GLOB RAYLIBIMGUI_SOURCES CONFIGURE_DEPENDS "../thirdparty/raylibimgui/*.cpp")
# Add the sources to the SOURCES variable
list(APPEND SOURCES ${RAYLIBIMGUI_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES}  ${PLAYS_FILES}  "editor_utils.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES}  "base_editor.cpp" "base_editor.h" "editors.cpp")
# For Raylib
include(FetchContent)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
set(RAYLIB_VERSION 5.0)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(raylib)



target_link_libraries(${PROJECT_NAME} imgui)

target_link_libraries(${PROJECT_NAME} coreloop)
target_link_libraries(${PROJECT_NAME} raylib)
# Export editor headers (no need for nested include as it's a simple library`)
target_include_directories(editor PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)