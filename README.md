# Riggle

**Riggle** is a lightweight, open-source 2D skeletal animation tool built for developers, educators, and creators of interactive media. It bridges the gap between overly complex professional tools and limited basic software by offering a streamlined, intuitive animation workflow.

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
These scripts will automatically build the project using CMake and MSVC.

---

## Downloads

You can download recent releases from the [release](https://github.com/3ipul/Riggle/releases) tab.