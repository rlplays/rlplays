project(coreloop CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (RLTRAIN)
    add_compile_options(-fPIC)
endif()

include_directories(include)
include_directories(../thirdparty/jsoncpp/include)
include_directories(../rlplays/include)
include_directories(../thirdparty/PufferLib/pufferlib/extensions)

# These two lines can be swapped with other gameplays.
include_directories(../plays)
file(GLOB_RECURSE PLAYS_FILES CONFIGURE_DEPENDS ../plays/*.h ../plays/*.cpp)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp ${PLAYS_FILES})
add_library(coreloop STATIC ${SOURCES} "src/game.cpp" "src/world.cpp" "include/log.h" "src/context.cpp" "include/base_types.h" "include/grid.h" "src/grid.cpp" "include/actions.h" "include/timer.h" "src/ablock.cpp" ${PLAYS_FILES} "src/timer.cpp" "include/game_progress.h" "src/grid_colliders.cpp" "include/world_editor.h" "include/world_fileset.h" "include/raylib_utils.h" "include/scenes.h" "src/scenes.cpp"  "include/game_actions.h" "include/grid_colliders.h"  "src/raylib_utils.cpp" "include/rl_player.h" "src/rl_player.cpp" "include/game_info.h")
add_compile_definitions(PUFFER_EXTERN=)


# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Dependencies


# For Raylib
include(FetchContent)

# Dependencies
set(RAYLIB_VERSION 5.0)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(raylib)

target_link_libraries(${PROJECT_NAME} raylib)


# Export include directory
target_include_directories(coreloop PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)