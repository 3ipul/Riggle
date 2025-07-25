# Project Architecture:

```
Riggle/                         ← Root of the project
├── CMakeLists.txt              ← Top-level build configuration
├── README.md                   ← This file
├── assets/                     ← Contains PNGs, rigs, fonts, and other resources like character/object images.
├── external/                   ← Houses external libraries, such as SFML and ImGui-SFML. This might include optional submodules or cloned repositories.
├── common/                     ← Provides shared math and utility headers used across different parts of the project.
├── Riggle_Core/                ← The core reusable animation system, designed to operate independently without dependencies on SFML or ImGui.
│   ├── include/                ← Public header files for the animation system components.
│   │   └── Riggle_Core/
│   │       └── Physics/        ← (Future) Contains components for physics simulations, such as a Verlet simulator for dynamic bones.
│   └── src/                    ← Source files for the core animation system.
│   │   └── Physics/
│   └── CMakeLists.txt          ← Build configuration for the Riggle_Core library.
├── Riggle_Editor/              ← The actual GUI-based editor application for Riggle.
    ├── include/                ← Public header files for the editor application.
    │   └── Riggle_Editor/
    │       ├── Panels/         ← Header files for various editor panels (e.g., Viewport, Timeline, Hierarchy, Properties).
    │       ├── Render/         ← Header files for rendering components (e.g., BoneRenderer, ImageRenderer).
    │       └── Export/         ← Header files for export functionalities (e.g., JsonExporter, PngExporter).
    ├── src/                    ← Source files for the editor application.
    │   ├── main.cpp            ← The application's entry point.
    │   ├── Panels/             ← Source files for the editor panels.
    │   ├── Render/             ← Source files for rendering components.
    │   └── Export/             ← Source files for export functionalities.
    └── CMakeLists.txt  
```

## Run the `build.bat` from `Riggle/script/`