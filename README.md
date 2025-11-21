# OMPDataPerf


# Building
```bash
git clone https://github.com/lmarzen/ompdataperf.git
git submodule update --init
bash build.sh
```

# Usage
```
Usage: ompdataperf [options] [program] [program arguments]
Options:
  -h, --help              Show this help message
  -q, --quiet             Suppress warnings
  -v, --verbose           Enable verbose output
  --version               Print the version of ompdataperf
```

# Docker

OMPDataperf currently has two Docker files, one for ROCM and the other for CUDA.

To run the container for AMD GPUs:
```
cd rocm-docker-image
docker build -t ompdataperf:latest .
docker run --rm -it --device=/dev/kfd --device=/dev/dri --group-add video ompdataperf:latest bash
```

To run the container for Nvidia GPUs:
```
sudo apt-get install -y nvidia-container-toolkit
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker

cd cuda-docker-image
docker build -t ompdataperf:latest .
sudo docker run --rm -it --gpus all ompdataperf:latest bash
```
