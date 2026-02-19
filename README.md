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

Some third-party datasets for benchmarks must be downloaded:
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

OMPDataperf supports two Dockerfiles, one for NVIDIA CUDA and the other for AMD ROCM.

### NVIDIA GPUs
To get GPU support for NVIDIA GPUs working in a docker container you may require additional setup on your host system:
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

## Citation

If you use this code for your research, please cite the following work:

> Luke Marzen, Junhyung Shim, and Ali Jannesari. 2026. Dynamic Detection of Inefficient Data Mapping Patterns in Heterogeneous OpenMP Applications. In Proceedings of the 31st ACM SIGPLAN Annual Symposium on Principles and Practice of Parallel Programming (PPoPP '26). Association for Computing Machinery, New York, NY, USA, 177–189. https://doi.org/10.1145/3774934.3786454

```tex
@inproceedings{10.1145/3774934.3786454,
author = {Marzen, Luke and Shim, Junhyung and Jannesari, Ali},
title = {Dynamic Detection of Inefficient Data Mapping Patterns in Heterogeneous OpenMP Applications},
year = {2026},
isbn = {9798400723100},
publisher = {Association for Computing Machinery},
address = {New York, NY, USA},
url = {https://doi.org/10.1145/3774934.3786454},
doi = {10.1145/3774934.3786454},
abstract = {With the growing prevalence of heterogeneous computing, CPUs are increasingly being paired with accelerators to achieve new levels of performance and energy efficiency. However, data movement between devices remains a significant bottleneck, complicating application development. Existing performance tools require considerable programmer intervention to diagnose and locate data transfer inefficiencies. To address this, we propose dynamic analysis techniques to detect and profile inefficient data transfer and allocation patterns in heterogeneous applications. We implemented these techniques into OMPDataPerf, which provides detailed traces of problematic data mappings, source code attribution, and assessments of optimization potential in heterogeneous OpenMP applications. OMPDataPerf uses the OpenMP Tools Interface (OMPT) and incurs only a 5 \% geometric‑mean runtime overhead.},
booktitle = {Proceedings of the 31st ACM SIGPLAN Annual Symposium on Principles and Practice of Parallel Programming},
pages = {177–189},
numpages = {13},
keywords = {Data Transfer, Dynamic Analysis, GPUs, Heterogeneous Computing, OpenMP, Performance Profiling},
location = {Sydney, NSW, Australia},
series = {PPoPP '26}
}
```
