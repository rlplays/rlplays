cmake_minimum_required(VERSION 3.20)
project(imgui)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED)

# Generate compile_commands.json
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(include)
file(GLOB SOURCES CONFIGURE_DEPENDS "imgui.cpp" "imgui_draw.cpp" "imgui_widgets.cpp" "imgui_tables.cpp" "imgui_demo.cpp")

add_library(imgui STATIC ${SOURCES})

target_include_directories(imgui PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)