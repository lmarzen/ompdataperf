FROM ubuntu:24.04

# Avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies for LLVM and building OpenMP
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    ninja-build \
    python3 \
    ca-certificates \
    nano \
    vim \
    bzip2 \
    tar \
    elfutils \
    libdw-dev \
    llvm-20 \
    clang-20 \
    lld-20 \
    libomp-20-dev \
    nvidia-cuda-toolkit \
    && rm -rf /var/lib/apt/lists/*

    # libomp-20-dev \


#RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
#RUN add-apt-repository 'deb http://apt.llvm.org/noble/ llvm-toolchain-noble-20 main'

# Add ROCm repo key
RUN mkdir -p /etc/apt/keyrings && chmod 755 /etc/apt/keyrings
RUN wget https://repo.radeon.com/rocm/rocm.gpg.key -O - | \
    gpg --dearmor > /etc/apt/keyrings/rocm.gpg
# Add ROCm sources
RUN printf "%s\n" \
    "deb [arch=amd64 signed-by=/etc/apt/keyrings/rocm.gpg] https://repo.radeon.com/rocm/apt/7.1 noble main" \
    > /etc/apt/sources.list.d/rocm.list
# Pin ROCm packages
RUN printf "%s\n" \
    "Package: *" \
    "Pin: release o=repo.radeon.com" \
    "Pin-Priority: 600" \
    > /etc/apt/preferences.d/rocm-pin-600

RUN apt-get update && apt-get install -y \
       rocm-smi \
       rocm \
    && rm -rf /var/lib/apt/lists/*

ENV PATH="/usr/lib/llvm-20/bin:${PATH}"
ENV LD_LIBRARY_PATH="/usr/lib/llvm-20/lib:${LD_LIBRARY_PATH}"

# Verify installation
RUN echo $PATH && echo $LD_LIBRARY_PATH
RUN clang --version
