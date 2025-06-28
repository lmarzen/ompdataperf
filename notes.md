minifmm is missing fmm.hh in common/

For running eval on different machines:

```sh
    make CXXFLAGS="-fopenmp \
-fopenmp-targets=nvptx64-nvidia-cuda \
-Xopenmp-target=nvptx64-nvidia-cuda --cuda-path=<cuda path> \
-Xopenmp-target=nvptx64-nvidia-cuda --cuda-gpu-arch=<GPU sm version> \
--libomptarget-nvptx-bc-path=<directory that holds .bc>
```