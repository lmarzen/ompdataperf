cmake_minimum_required(VERSION 3.12)
project(OMPDataProf C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Wno-unused-parameter -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
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

option(MEASURE_HASHING_OVERHEAD "Times how long it takes to perform hashing of data transfers." OFF)
if(MEASURE_HASHING_OVERHEAD)
  message(STATUS "Measuring hashing overhead")
  add_compile_definitions(MEASURE_HASHING_OVERHEAD)
else()
  message(STATUS "Not measuring hashing overhead")
endif()

option(PRINT_TRANSFER_RATE "Calculates and prints average data transfer rate in summary." OFF)
if(PRINT_TRANSFER_RATE)
  message(STATUS "Printing effective data transfer rate")
  add_compile_definitions(PRINT_TRANSFER_RATE)
else()
  message(STATUS "Not printing effective data transfer rate")
endif()

# Set a default hash function if none is provided
set(HASH_FUNCTION "t1ha0_ia32aes_avx2" CACHE STRING "Hash function to use")
add_compile_definitions(HASH_FUNCTION_${HASH_FUNCTION})
add_compile_definitions(HASH_FUNCTION=${HASH_FUNCTION})
if(HASH_FUNCTION STREQUAL "CityHash32" OR
   HASH_FUNCTION STREQUAL "CityHash64" OR
   HASH_FUNCTION STREQUAL "CityHash128" OR
   HASH_FUNCTION STREQUAL "CityHashCrc128")
  include_directories(hashes/cityhash/src)
  configure_file(
    ${CMAKE_SOURCE_DIR}/hashes/cityhash/config.h.in
    ${CMAKE_BINARY_DIR}/hashes/cityhash/config.h
    COPYONLY
  )
  add_library(cityhash STATIC hashes/cityhash/src/city.cc)
  target_include_directories(cityhash PRIVATE ${CMAKE_BINARY_DIR}/hashes/cityhash)
  set_target_properties(cityhash PROPERTIES COMPILE_FLAGS "-O3 -DNDEBUG -march=native")
  target_link_libraries(libompdataprof PRIVATE cityhash)
elseif(HASH_FUNCTION STREQUAL "FarmHash32" OR
       HASH_FUNCTION STREQUAL "FarmHash64" OR
       HASH_FUNCTION STREQUAL "FarmHash128")
  include_directories(hashes/farmhash/src)
  configure_file(
    ${CMAKE_SOURCE_DIR}/hashes/farmhash/config.h.in
    ${CMAKE_BINARY_DIR}/hashes/farmhash/config.h
    COPYONLY
  )
  add_library(farmhash STATIC hashes/farmhash/src/farmhash.cc)
  target_include_directories(farmhash PRIVATE ${CMAKE_BINARY_DIR}/hashes/farmhash)
  set_target_properties(farmhash PROPERTIES COMPILE_FLAGS "-O3 -DNDEBUG -march=native")
  target_link_libraries(libompdataprof PRIVATE farmhash)
elseif(HASH_FUNCTION STREQUAL "MeowHash")
  include_directories(hashes/meow_hash)
  set_source_files_properties(src/analyze.cc src/tool.cc
                              PROPERTIES COMPILE_FLAGS "-Wno-unused-function")
elseif(HASH_FUNCTION STREQUAL "rapidhash")
  include_directories(hashes/rapidhash)
elseif(HASH_FUNCTION STREQUAL "t1ha0_ia32aes_avx" OR
       HASH_FUNCTION STREQUAL "t1ha0_ia32aes_avx2" OR
       HASH_FUNCTION STREQUAL "t1ha0_ia32aes_noavx" OR
       HASH_FUNCTION STREQUAL "t1ha0_32le" OR
       HASH_FUNCTION STREQUAL "t1h0a_32be" OR
       HASH_FUNCTION STREQUAL "t1ha1_le" OR
       HASH_FUNCTION STREQUAL "t1ha1_be" OR
       HASH_FUNCTION STREQUAL "t1ha2_atonce")
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
  target_link_libraries(libompdataprof PRIVATE t1ha)
elseif(HASH_FUNCTION STREQUAL "XXH32" OR
       HASH_FUNCTION STREQUAL "XXH64" OR
       HASH_FUNCTION STREQUAL "XXH3_64bits" OR
       HASH_FUNCTION STREQUAL "XXH3_128bits")
  include_directories(hashes/xxHash)
  add_library(xxhash STATIC hashes/xxHash/xxhash.c)
  set_target_properties(xxhash PROPERTIES COMPILE_FLAGS "-O3 -DNDEBUG -march=native")
  target_link_libraries(libompdataprof PRIVATE xxhash)
else()
  message(FATAL_ERROR "Unsupported hash function: ${HASH_FUNCTION}")
endif()
