# Webcam Plugin for Nodos

![build-badge](https://github.com/nodos-dev/webcam/actions/workflows/release.yml/badge.svg)

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

