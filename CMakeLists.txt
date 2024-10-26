cmake_minimum_required(VERSION 3.12)
project(OMPDataProf C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Wno-unused-parameter -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(libompdataprof SHARED src/tool.cc)
target_sources(libompdataprof PRIVATE src/analyze.cc src/symbolizer.cc)

find_library(LIBDW dw REQUIRED)
target_link_libraries(libompdataprof PRIVATE ${LIBDW})

# Set the library output name (libompdataprof.so)
set_target_properties(libompdataprof PROPERTIES OUTPUT_NAME "ompdataprof")

add_executable(ompdataprof src/preload.cc)


option(ENABLE_COLLISION_CHECKING "Enable hash collision checking. WARNING: enabling this option will massively slow down profiling and hugely increase memory footprint" OFF)
if(ENABLE_COLLISION_CHECKING)
  message(STATUS "Collision check enabled")
  add_compile_definitions(ENABLE_COLLISION_CHECKING)
else()
  message(STATUS "Collision check disabled")
endif()

# Set a default hash function if none is provided
set(HASH_FUNCTION "RAPIDHASH" CACHE STRING "Hash function to use (options: MEOWHASH RAPIDHASH, T1HA0, XXH3)")
add_compile_definitions(HASH_FUNCTION_${HASH_FUNCTION})
if(HASH_FUNCTION STREQUAL "MEOWHASH")
  include_directories(hashes/meow_hash)
  set_source_files_properties(src/analyze.cc src/tool.cc
                              PROPERTIES COMPILE_FLAGS "-Wno-unused-function")
elseif(HASH_FUNCTION STREQUAL "RAPIDHASH")
  include_directories(hashes/rapidhash)
elseif(HASH_FUNCTION STREQUAL "T1HA0")
  include_directories(hashes/t1ha)
  add_library(t1ha STATIC hashes/t1ha/src/t1ha0.c
                          hashes/t1ha/src/t1ha1.c
                          hashes/t1ha/src/t1ha2.c
                          hashes/t1ha/src/t1ha0_ia32aes_noavx.c
                          hashes/t1ha/src/t1ha0_ia32aes_avx.c
                          hashes/t1ha/src/t1ha0_ia32aes_avx2.c)
  set_source_files_properties(hashes/t1ha/src/t1ha0_ia32aes_noavx.c
                              PROPERTIES COMPILE_FLAGS "-mno-avx2 -mno-avx -maes")
  set_source_files_properties(hashes/t1ha/src/t1ha0_ia32aes_avx.c
                              PROPERTIES COMPILE_FLAGS "-mno-avx2 -mavx -maes")
  set_source_files_properties(hashes/t1ha/src/t1ha0_ia32aes_avx2.c
                              PROPERTIES COMPILE_FLAGS "-mavx -mavx2 -maes")
  set_target_properties(t1ha PROPERTIES COMPILE_FLAGS "-O3 -DNDEBUG")
  set_target_properties(t1ha PROPERTIES C_STANDARD 11)
  set_target_properties(t1ha PROPERTIES C_STANDARD_REQUIRED ON)
  set_target_properties(t1ha PROPERTIES C_EXTENSIONS OFF)
  # target_include_directories(t1ha PRIVATE hashes/t1ha/src)
  target_link_libraries(libompdataprof PRIVATE t1ha)
elseif(HASH_FUNCTION STREQUAL "XXH3")
  include_directories(hashes/xxHash)
else()
  message(FATAL_ERROR "Unsupported hash function: ${HASH_FUNCTION}")
endif()

