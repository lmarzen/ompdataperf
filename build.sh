#!/bin/bash

cd "$(dirname "$0")"

rm -rf build
mkdir build
cd build
#cmake .. -DCMAKE_BUILD_TYPE='Release' -DENABLE_COLLISION_CHECKING=OFF -DMEASURE_HASHING_OVERHEAD=OFF -DHASH_FUNCTION='t1ha0_ia32aes_avx2'
cmake .. -DCMAKE_BUILD_TYPE='Debug' -DENABLE_COLLISION_CHECKING=ON -DMEASURE_HASHING_OVERHEAD=OFF -DHASH_FUNCTION='t1ha0_ia32aes_avx2'
make -j
