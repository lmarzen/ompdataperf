cmake_minimum_required(VERSION 3.12)
project(OMPDataProf CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_COMPILER "clang++")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(libompdataprof SHARED src/tool.cc)
target_sources(libompdataprof PRIVATE src/analyze.cc src/symbolizer.cc)

find_package(Boost REQUIRED CONFIG)
include_directories(${Boost_INCLUDE_DIRS})

find_library(LIBDW dw REQUIRED)
target_link_libraries(libompdataprof PRIVATE ${LIBDW})

include_directories(rapidhash)

# Set the library output name (libompdataprof.so)
set_target_properties(libompdataprof PROPERTIES OUTPUT_NAME "ompdataprof")

add_executable(ompdataprof src/preload.cc)


option(ENABLE_COLLISION_CHECKING "Enable hash collision checking. WARNING: enabling this option will massively slow down profiling and hugely increase memory footprint" OFF)
if(ENABLE_COLLISION_CHECKING)
    message(STATUS "Collision check enabled")
    add_definitions(-DENABLE_COLLISION_CHECKING)
else()
    message(STATUS "Collision check disabled")
endif()

