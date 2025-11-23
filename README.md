# OMPDataPerf

## Usage
```
Usage: ompdataperf [options] [program] [program arguments]
Options:
  -h, --help              Show this help message
  -q, --quiet             Suppress warnings
  -v, --verbose           Enable verbose output
  --version               Print the version of ompdataperf
```

## Dependencies

The provided [docker containers](#Docker) can be used to simplify environment setup.
1. Clang >=19.1 with OpenMP GPU Offloading support
2. elfutils (libdw) >= 0.189
3. CMake >=3.12
4. Python3
5. GPU toolchain(s):
    - NVIDIA: CUDA toolkit
    - AMD: ROCm device libraries

## Building
```bash
git clone https://github.com/lmarzen/ompdataperf.git
cd ompdataperf
git submodule update --init
bash build.sh
```

### Tool Evaluation and Testing (optional)
```bash
cd eval
bash download_dataset.sh
```

The build scripts for the included benchmarks were intended for use on a machine with an Nvidia GPU, if you have an AMD GPU you’ll need to replace the -fopenmp-targets=nvptx flag. This can be quickly done with the following command.
```bash
find . -type f -exec sed -i 's/nvptx64-nvidia-cuda/amdgcn-amd-amdhsa/g; s/nvptx64/amdgcn-amd-amdhsa/g' {} +
```

Compile the benchmarks and programs used for evaluation:
```bash
make
```

Run a benchmark:
```bash
../build/ompdataperf ./src/hotspot/hotspot_offload 64 64 2 4 data/hotspot/temp_64 data/hotspot/power_64 output.out
```


## Docker

OMPDataperf currently has two Docker files, one for ROCM and the other for CUDA.

### NVIDIA GPUs
To get GPU support for NVIDIA GPUs working in a docker container you may require additional setup:
```bash
sudo apt-get install -y nvidia-container-toolkit
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker
```

To run the container for Nvidia GPUs:
```bash
cd docker/cuda-docker-image
docker build -t ompdataperf:latest .
docker run --rm -it --gpus all ompdataperf:latest bash
```

### AMD GPUs
To run the container for AMD GPUs:
```bash
cd docker/rocm-docker-image
docker build -t ompdataperf:latest .
docker run --rm -it --device=/dev/kfd --device=/dev/dri --group-add video ompdataperf:latest bash
```
