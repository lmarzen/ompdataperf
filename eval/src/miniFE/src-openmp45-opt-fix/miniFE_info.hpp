#ifndef miniFE_info_hpp
#define miniFE_info_hpp

#define MINIFE_HOSTNAME "frost-4.las.iastate.edu"
#define MINIFE_KERNEL_NAME "'Linux'"
#define MINIFE_KERNEL_RELEASE "'5.14.0-427.28.1.el9_4.x86_64'"
#define MINIFE_PROCESSOR "'x86_64'"

#define MINIFE_CXX "'/work/LAS/jannesar-lab/ljmarzen/ompoffload/llvm19/bin/clang++'"
#define MINIFE_CXX_VERSION "'clang version 19.1.0 (https://github.com/llvm/llvm-project.git 64075837b5532108a1fe96a5b158feb7a9025694)'"
#define MINIFE_CXXFLAGS "'-v -O3 -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=/usr/local/cuda-11.8 -ffp-contract=fast -march=native -fslp-vectorize-aggressive -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize -Xcuda-ptxas -maxrregcount=32 '"

#endif
