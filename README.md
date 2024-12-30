# Webcam Plugin for Nodos

![build-badge](https://github.com/nodos-dev/webcam/actions/workflows/build.yml/badge.svg)

This folder contains the Nodos plugin for webcam I/O.

## Build Instructions
1. Download latest Nodos release from [nodos.dev](https://nodos.dev)
2. Clone the repository under Nodos workspace Module directory
```bash
git clone https://github.com/nodos-dev/webcam.git --recurse-submodules Module/webcam
```
3. Generate project files from workspace root directory using CMake:
```bash
cmake -S ./Toolchain/CMake -B Build
```
4. Build the project:
```bash
cmake --build Build
```

## WebcamOut
Webcam Plugin uses Softcam library to create virtual camera. If you want to use it:

# Building from Source
1. Build the project. 
2. Run `BuildDriverAndGetInstaller.bat`. It will create `softcam.dll` and `softcam_installer.exe` in Binaries folder.

# Install
1. Run `RegisterSoftcam.bat` as administrator.
2. Don't delete or move `softcam.dll` because we don't copy it anywhere else
