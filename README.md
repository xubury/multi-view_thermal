multi-view_thermal
======
Code for the paper '3D Thermal Model Reconstruction using Shading-based Multiple-view Stereo'.

Compile Requirement
------
* [glbinding](https://github.com/cginternals/glbinding)
* [glm](https://github.com/g-truc/glm)
* [wxWidgets](https://github.com/wxWidgets/wxWidgets)
* CMake building system
* C++11 compatible compiler with OpenMP support
* Python 3

Quickstart
------
1. `git clone https://github.com/xubury/multi-view_thermal`
2. `cmake -G YOUR_COMPILE_GENERATOR -B BUILD_DIR`
3. Run `multi_view` to perform reconstruction
4. Run `scripts/main.py` for registeration and calibration

