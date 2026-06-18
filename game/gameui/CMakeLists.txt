cmake_minimum_required(VERSION 3.22...3.30)
if(RLPLAYS_CONVERTER)
  project(rlplays_converter)
  add_compile_definitions(RLPLAYS_CONVERTER=1)
  add_compile_definitions(RLPLAYS_HEADLESS=1)
elseif(RLPLAYS_TEST)
  project(rlplays_test)
  add_compile_definitions(RLPLAYS_HEADLESS=1)
else()
  project(rlplays_game)
endif()

if (RLPLAYS_TRAIN)
  add_compile_definitions(RLPLAYS_TRAIN=1)
endif()

if(DEBUG_TRACE)
  add_compile_definitions(DEBUG_TRACE=1)
endif()

if(RELEASE)
  add_compile_definitions(RELEASE=1)
  set(CMAKE_BUILD_TYPE Release)
  if(NOT WIN32)
    add_compile_options(-O3 -DNDEBUG)
  endif()
endif()


if(DEBUG)
  add_compile_definitions(DEBUG=1)
  set(CMAKE_BUILD_TYPE Debug)
  # Force debug symbols on all targets
  if(NOT WIN32)
    add_compile_options(-g -O0)
  endif()
  # TODO: ASAN on Windows doesn't work.
  #if(MSVC)
  #  add_compile_options(/fsanitize=address)
  #endif()
endif()

if(PUFFERLIB_SELFPLAY)
  add_compile_definitions(PUFFERLIB_SELFPLAY=1)
endif()


if (RLTRAIN)
    add_compile_options(-fPIC)
elseif (NOT (WIN32 OR ("${PLATFORM}" STREQUAL "Web")) AND (NOT RLPLAYS_TRAIN) AND (NOT APPLE))
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g3 -O0 -fno-optimize-sibling-calls)
    add_link_options(-fsanitize=address)    
endif()


if(RLPLAYS_EDITOR)
  add_compile_definitions(RLPLAYS_EDITOR=1)
endif()

if (EMSCRIPTEN)
  if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -DDEBUG=1 -s ALLOW_MEMORY_GROWTH")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -DDEBUG=1 -s ALLOW_MEMORY_GROWTH")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -gsource-map -s NO_DISABLE_EXCEPTION_CATCHING")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -gsource-map -s NO_DISABLE_EXCEPTION_CATCHING")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g -gsource-map -s ASSERTIONS=1 ")  
  else()
    add_compile_options(-O3 -DNDEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -s ALLOW_MEMORY_GROWTH")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -s ALLOW_MEMORY_GROWTH")
  endif()
  # -s DEMANGLE_SUPPORT=1 no longer supported, so removed.
  set(CMAKE_EXECUTABLE_SUFFIX ".html")

elseif (NOT DEBUG)
# Add debug symbols to Release builds / TODO NOT WORKING
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
#set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

####### Dependencies #######

# Fetch raylib dependency
include(FetchContent)
set(RAYLIB_VERSION 5.0)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(raylib)


if (RLPLAYS_TEST)
  add_compile_definitions(RLPLAYS_TEST=1)
  enable_testing()
endif()
if (RLPLAYS_WAIT_AFTER_RUN)
    add_compile_definitions(RLPLAYS_WAIT_AFTER_RUN=1)
endif()

# Our Project
add_executable(${PROJECT_NAME})
# Add specific source files as needed.
if(RLPLAYS_CONVERTER)
  file(GLOB SOURCES CONFIGURE_DEPENDS "src/converter.cpp")
elseif(RLPLAYS_TEST)
  file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "../tests/*.cpp")
  set(SOURCES ${TEST_SOURCES} )
  include_directories(../thirdparty/PufferLib/pufferlib/extensions)
  include_directories(../tests/)
else()
  file(GLOB SOURCES CONFIGURE_DEPENDS "src/raylib_game.cpp")
endif()  

# Conditionally add editor source/imgui/rlImGui when RLPLAYS_EDITOR is defined
if(RLPLAYS_EDITOR)
  add_subdirectory(../editor ../build/editor)
  include_directories(../thirdparty/raylibimgui)
  include_directories(../thirdparty/raylibimgui/extras)
  include_directories(../thirdparty/imgui)

  target_link_libraries(${PROJECT_NAME} editor)
endif()

# Add the source files to the parent target
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES}   )


# Add coreloop library
add_subdirectory(../coreloop ../build/coreloop)
include_directories(../coreloop/include)
include_directories(../plays)
include_directories(../thirdparty/jsoncpp/include)
include_directories(../rlplays/include)
include_directories(../thirdparty/PufferLib/pufferlib/ocean/)

# Link coreloop library to the project
target_link_libraries(${PROJECT_NAME} coreloop)


set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

  
if ("${PLATFORM}" STREQUAL "Web")
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/src/resources
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/src/resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/../data  ${CMAKE_SOURCE_DIR}/src/resources
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/src/resources/temp/

        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/../resources
        COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_SOURCE_DIR}/web/tile_0000.png $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    #DEPENDS ${PROJECT_NAME}
else()
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/src/resources
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/src/resources        
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/../data  ${CMAKE_SOURCE_DIR}/src/resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
    #DEPENDS ${PROJECT_NAME}
endif()

#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib)
# Web Configurations
if ("${PLATFORM}" STREQUAL "Web")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    target_link_options(${PROJECT_NAME} PUBLIC -sUSE_WEBGL2=1 -sUSE_GLFW=3 PUBLIC --preload-file resources)
    target_link_options(${PROJECT_NAME} PUBLIC --shell-file ${CMAKE_SOURCE_DIR}/web/minshell.html)
elseif ((NOT WIN32) AND (NOT APPLE) AND (NOT RLPLAYS_TRAIN)) 
    # Target asan on Linux builds.
    #target_link_libraries(${PROJECT_NAME} asan)
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

if (RLPLAYS_TEST)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )

  if (RLPLAYS_PUFFERLIB_TESTS)
    # add_subdirectory(../rlplays/tests ../build/pufferlib_tests)
    # target_link_libraries(${PROJECT_NAME}  pufferlib_tests)
    # TODO: Re-evaluate this - we are using shared libs now.
    # The following code block is suggested to be used on Windows.
    # According to https://github.com/pytorch/pytorch/issues/25457,
    # the DLLs need to be copied to avoid memory errors.  
    if (MSVC)
      set(LIBTORCH_DIR "${CMAKE_BINARY_DIR}/libtorch")
      set(Torch_DIR "${LIBTORCH_DIR}/share/cmake/Torch")
      find_package(Torch REQUIRED)
      file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
      add_custom_command(TARGET ${PROJECT_NAME}
                         POST_BUILD
                         COMMAND ${CMAKE_COMMAND} -E copy_if_different
                         ${TORCH_DLLS}
                         $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endif (MSVC)  
    # target_sources(${PROJECT_NAME} PRIVATE $<TARGET_OBJECTS:pufferlib_tests>)
  endif()
  target_link_libraries(${PROJECT_NAME} GTest::gtest_main)

  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  include(GoogleTest)
  gtest_discover_tests(
      ${PROJECT_NAME}      
      DISCOVERY_TIMEOUT 60     # bump from default 5s
  )
endif()

# Enable parallel builds by default
if(NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL)
    include(ProcessorCount)
    ProcessorCount(N)
    if(NOT N EQUAL 0)
        set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
    endif()
endif()