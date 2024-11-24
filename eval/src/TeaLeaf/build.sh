#!/usr/bin/env bash


module load cmake

rm -r build
mkdir build
cd build
CC=clang CXX=clang++ cmake -DMODEL=omp-target -DENABLE_MPI=OFF -DOFFLOAD=ON -DOFFLOAD_FLAGS="-fopenmp -fopenmp-targets=nvptx64" -DCMAKE_BUILD_TYPE="Debug" ..
#CC=clang CXX=clang++ cmake -DMODEL=omp-target -DENABLE_MPI=OFF -DOFFLOAD=ON -DOFFLOAD_FLAGS="-fopenmp -fopenmp-targets=nvptx64" ..
make -j
