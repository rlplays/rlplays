cmake_minimum_required(VERSION 3.22...3.30)
project(pufferlib_tests)
# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".dylib" ".dll")

if(RELEASE)
  add_compile_definitions(RELEASE=1)
  set(CMAKE_BUILD_TYPE Release)
  if(NOT WIN32)
    add_compile_options(-O3 -DNDEBUG)
  endif()
endif()

if (RLPLAYS_WAIT_AFTER_RUN)
    add_compile_definitions(RLPLAYS_WAIT_AFTER_RUN=1)
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
if (PUFFER_CUDA EQUAL 1)
  # 61;75 for 1080 GTX and 2080 RTX
  set(CMAKE_CUDA_ARCHITECTURES "61" CACHE STRING "CUDA architectures" FORCE)
  enable_language(CUDA)
  
  # set(CMAKE_CUDA_ARCHITECTURES "75" CACHE STRING "CUDA architectures" FORCE)
  # set(TORCH_CUDA_ARCH_LIST "7.5;8.6;8.9" CACHE STRING "Torch CUDA architectures" FORCE)

  # Set CUDA standard to match C++ standard
  set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  
  
  # Speed up NVCC compilation
  if(MSVC)
    # add_compile_options(/MP)
    # set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL")
    # set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /INCREMENTAL")
    string(APPEND CMAKE_CUDA_FLAGS " -Xcompiler=/Zc:preprocessor")
    string(APPEND CMAKE_CUDA_FLAGS " -DCCCL_IGNORE_MSVC_TRADITIONAL_PREPROCESSOR_WARNING")
    
    # Reduce debug info size and improve compile speed
    if(DEBUG)
      string(APPEND CMAKE_CUDA_FLAGS " -Xcompiler=/Zi")  # Use /Zi instead of /ZI
      string(APPEND CMAKE_CUDA_FLAGS " -lineinfo")       # Faster than -G for debug
    endif()
    
    # Use faster linker
    #string(APPEND CMAKE_CUDA_FLAGS " -Xcompiler=/DDEBUG:FASTLINK")
  endif()
  
  add_compile_definitions(PUFFER_CUDA=1)
  find_package(CUDAToolkit REQUIRED)
endif()


if (NOT (WIN32 OR ("${PLATFORM}" STREQUAL "Web")) AND (NOT RLPLAYS_TRAIN) AND (NOT APPLE))
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g3 -O0 -fno-optimize-sibling-calls)
    add_link_options(-fsanitize=address)    
endif()


# Set PUFFER_CUDA 1 via command-line to enable CUDA support.
if(WIN32)
  if (PUFFER_CUDA EQUAL 1)
    set(LIBTORCH_VERSION "2.9.1+cuda")
    if (DEBUG)
      set(LIBTORCH_URL "https://download.pytorch.org/libtorch/cu126/libtorch-win-shared-with-deps-debug-2.9.1%2Bcu126.zip")
    else()
      set(LIBTORCH_URL "https://download.pytorch.org/libtorch/cu126/libtorch-win-shared-with-deps-2.9.1%2Bcu126.zip")
    endif()
  else()
    # Non-CUDA version for Windows (CPU version to simplify things)
    set(LIBTORCH_VERSION "2.9.1+cpu")
    if (DEBUG)
      set(LIBTORCH_URL "https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-debug-2.9.1%2Bcpu.zip")
    else()
      set(LIBTORCH_URL "https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-2.9.1%2Bcpu.zip")
    endif()
  endif()    
  set(LIBTORCH_DIR "${CMAKE_BINARY_DIR}/libtorch")
elseif(UNIX AND NOT APPLE)
  if (PUFFER_CUDA EQUAL 1)
    set(LIBTORCH_VERSION "2.9.1+cu")
    set(LIBTORCH_URL "https://download.pytorch.org/libtorch/cu126/libtorch-shared-with-deps-2.9.1%2Bcu126.zip")
  else()
    set(LIBTORCH_VERSION "2.9.1+cpu")
    set(LIBTORCH_URL "https://download.pytorch.org/libtorch/cpu/libtorch-shared-with-deps-2.9.1%2Bcpu.zip")
  endif()
  set(LIBTORCH_DIR "${CMAKE_BINARY_DIR}/libtorch")
endif()

add_compile_definitions(RLPLAYS_TEST=1)
enable_testing()

if(WIN32 OR (UNIX AND NOT APPLE))
  # Check if LibTorch is already downloaded
  if(NOT EXISTS "${LIBTORCH_DIR}")
    message(STATUS "Downloading LibTorch ${LIBTORCH_VERSION} to ${LIBTORCH_DIR}...")
    file(DOWNLOAD 
      ${LIBTORCH_URL}
      "${CMAKE_BINARY_DIR}/libtorch.zip"
      SHOW_PROGRESS
      STATUS download_status
    )
    
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
      message(FATAL_ERROR "Failed to download LibTorch")
    endif()
    
    message(STATUS "Extracting LibTorch...")
    file(ARCHIVE_EXTRACT 
      INPUT "${CMAKE_BINARY_DIR}/libtorch.zip"
      DESTINATION "${CMAKE_BINARY_DIR}"
    )
    
    file(REMOVE "${CMAKE_BINARY_DIR}/libtorch.zip")
    message(STATUS "LibTorch extracted to ${LIBTORCH_DIR}")
  else()
    message(STATUS "Using existing LibTorch ${LIBTORCH_VERSION} at ${LIBTORCH_DIR}")
  endif()
  
  # Set Torch_DIR to help find_package locate LibTorch
  set(Torch_DIR "${LIBTORCH_DIR}/share/cmake/Torch")
endif()

if (PUFFER_CUDA EQUAL 1)
  # Set this to 6.1;7.5 if needed
  set(TORCH_CUDA_ARCH_LIST "6.1" CACHE STRING "Torch CUDA architectures" FORCE)
  set(CMAKE_CUDA_ARCHITECTURES "61" CACHE STRING "CUDA architectures" FORCE)
  
  # Prevent PyTorch from auto-detecting architectures
  set(CUDA_ARCH_LIST "6.1" CACHE STRING "CUDA architectures for Torch" FORCE)
  set(TORCH_NVCC_FLAGS "" CACHE STRING "Override Torch NVCC flags" FORCE)
endif()

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


find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Force override TORCH_CUDA_ARCH_LIST after find_package
if (PUFFER_CUDA EQUAL 1)
  set(CMAKE_CUDA_ARCHITECTURES "61" CACHE STRING "CUDA architectures" FORCE)
  # Clear any gencode flags PyTorch added
  string(REGEX REPLACE "-gencode arch=[^ ]+" "" CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS}")
  message(STATUS "CUDA Architectures set to: ${CMAKE_CUDA_ARCHITECTURES}")
endif()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "pufferlib_test.cpp")

if (PUFFER_CUDA EQUAL 1)
  file(GLOB PUFFER_CUDA_SOURCES CONFIGURE_DEPENDS 
    "../../thirdparty/PufferLib/pufferlib/puffer_cuda_kernels.cu")
  # Mark CUDA files explicitly
  set_source_files_properties(${PUFFER_CUDA_SOURCES} PROPERTIES LANGUAGE CUDA)
else()
  set(PUFFER_CUDA_SOURCES "")
endif()

set(SOURCES ${TEST_SOURCES})

add_executable(${PROJECT_NAME} )
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES}   ${PUFFER_CUDA_SOURCES})
add_compile_definitions(PUFFER_EXTERN=)

# Set CUDA properties if enabled
if (PUFFER_CUDA EQUAL 1)
  set_target_properties(${PROJECT_NAME} PROPERTIES
      CUDA_STANDARD 20
      CUDA_STANDARD_REQUIRED ON
      CUDA_SEPARABLE_COMPILATION ON
      CUDA_RESOLVE_DEVICE_SYMBOLS ON
      CUDA_ARCHITECTURES "61"      
      POSITION_INDEPENDENT_CODE ON
  )
  
  # Enable parallel compilation
  target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--threads 0>  # Use all available threads
  )
  
  target_link_libraries(${PROJECT_NAME} PUBLIC CUDA::cudart CUDA::cudart_static "${TORCH_LIBRARIES}")

  set_target_properties(${PROJECT_NAME} PROPERTIES 
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    POSITION_INDEPENDENT_CODE ON
  )
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
       target_precompile_headers(pufferlib_tests PRIVATE
        <torch/torch.h>
        <iostream>
        <vector>
        <memory>
        )
    endif (MSVC)  
else()
  target_link_libraries(${PROJECT_NAME} PUBLIC "${TORCH_LIBRARIES}")
endif()


# Include directories
add_subdirectory(../../coreloop ../../build/coreloop)

include_directories(../../coreloop/include)
include_directories(../../plays)
include_directories(../../thirdparty/jsoncpp/include)
include_directories(../include)
include_directories(../../thirdparty/PufferLib/pufferlib/ocean/)
include_directories(../../thirdparty/PufferLib/pufferlib/extensions)

target_link_libraries(${PROJECT_NAME} PUBLIC coreloop)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)
target_link_libraries(${PROJECT_NAME} PUBLIC raylib)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
target_link_libraries(${PROJECT_NAME} PUBLIC GTest::gtest_main)

FetchContent_MakeAvailable(googletest)
include(GoogleTest)
gtest_discover_tests(
    ${PROJECT_NAME}      
    DISCOVERY_TIMEOUT 60     # bump from default 5s
)
