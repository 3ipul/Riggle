# Riggle

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**Riggle** is a lightweight 2D skeletal animation tool with a clean ImGui-based interface, designed for efficient animation workflows.

---

## Features

- **Skeletal Hierarchy System**: Create parent-child bone structures for flexible character rigging.
- **Forward Kinematics (FK)**: Directly manipulate joints with precision.
- **Inverse Kinematics (IK)**: Easily pose characters using a CCD-based solver.
- **Sprite Import & Binding**: Import character parts and bind them to bones with intuitive workflows.
- **Timeline-Based Animation**: Keyframe animations with linear interpolation and playback tools.
- **Multi-Format Export**: Export animations as structured JSON or PNG image sequences.
- **Clean, Dockable GUI**: Built using ImGui, featuring asset panels, hierarchy view, timeline, and more.
---

## Build Instructions

### Prerequisites

To build Riggle, ensure the following tools and libraries are installed:

- [Microsoft Visual Studio](https://visualstudio.microsoft.com/) (MSVC) — recent version with C++ and SFML 3.0 support
- [CMake](https://cmake.org/) — for managing the build process

### Building the Project

**1. Clone or download the repository.**

**2. Navigate to the `Script/` directory and run one of the provided build scripts:**

#### Debug Build:
```bash
./Script/build_debug.bat
```
#### Release Build:
```bash
./Script/build_release.bat
```
These scripts will automatically build and run the project.

**OR,**

**Simply open the project folder in Visual Studio. Visual Studio will automatically configure the CMake project. You can build and run the project using the built-in Build and Run options.**

---

## Platform Support

Currently, Riggle officially supports **Windows** only and requires **Microsoft Visual Studio (MSVC)** for building and running.

File system operations like saving/loading projects and sprite insertion are implemented using Windows-specific APIs.

Cross-platform support is planned for future releases.

---

## Downloads

You can download recent releases from the [release](https://github.com/3ipul/Riggle/releases) tab.

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

### Third-Party Licenses

This project bundles the following third-party libraries:

- [Dear ImGui (Docking Branch)](https://github.com/ocornut/imgui/tree/docking) – MIT License ([View License](https://github.com/ocornut/imgui/blob/docking/LICENSE.txt))
- [ImGui-SFML](https://github.com/SFML/imgui-sfml) (modified) – MIT License ([View License](https://github.com/SFML/imgui-sfml/blob/master/LICENSE))
- [SFML 3.0](https://www.sfml-dev.org/) – zlib/libpng License ([View License](https://github.com/SFML/SFML/blob/master/license.md))
- [nlohmann/json](https://github.com/nlohmann/json) - MIT License ([View License](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT))
- [miniz](https://github.com/richgel999/miniz) – MIT License ([View License](https://github.com/richgel999/miniz/blob/master/LICENSE))

All included third-party libraries retain their original licenses and are credited to their respective authors.