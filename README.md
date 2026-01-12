# AETHER

**AETHER** is an OpenGL-implemented 3D air combat game, developping as a project for ZJU course "Computer Graphics" (fall 2025).

This project is cross-platform and plan to support Windows and MacOS.

## Installation

### Prebuilt Binaries

Prebuilt binaries for Windows and MacOS are available in the [Releases](https://github.com/rzm0572/ZJU-CG-fa25-AETHER/releases) page.

### From Source

Make sure you have installed the following dependencies:

- git
- CMake
- C++ compiler (e.g. g++, clang++, LLVM-MinGW, MinGW, MSVC, etc.)
- OpenGL

For MacOS users, you need extra dependencies:

- Cocoa
- IOKit

You can install AETHER by the following steps:

1. Clone the repository and pull submodules:

    ```bash
    git clone https://github.com/rzm0572/ZJU-CG-fa25-AETHER.git
    cd ZJU-CG-fa25-AETHER
    git submodule update --init --recursive

    # build assimp
    cd repo/assimp
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make
    ```

2. run CMake to generate project files:

    For Windows:

    ```powershell
    cmake -B build -S . -G "MinGW Makefiles"
    ```

    **Note:** It is recommended to use [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) to compile the project on Windows.

    For MacOS / Linux:

    ```bash
    cmake -B build -S .
    ```

3. Build and install the project:

    ```bash
    cmake --build build
    cmake --install build
    ```

    The executable file will be installed to `bin` directory.

## Usage

To run the game, simply execute the `./aether` command in the `bin` directory. Enjoy!
