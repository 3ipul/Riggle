# Riggle_Editor/CMakeLists.txt

# Set ImGui paths
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")
set(IMGUI_SFML_DIR "${CMAKE_SOURCE_DIR}/external/imgui-sfml")

# Add ImGui-SFML library
add_library(ImGui-SFML STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_SFML_DIR}/ImGui-SFML.cpp
)

target_include_directories(ImGui-SFML PUBLIC 
    ${IMGUI_DIR}
    ${IMGUI_SFML_DIR}
    ${SFML_INCLUDE_DIR}
)

target_compile_definitions(ImGui-SFML PRIVATE SFML_STATIC)
set_source_files_properties(src/Utils/miniz.c PROPERTIES LANGUAGE C)

# Main application
add_executable(Riggle_Editor
    src/main.cpp
    src/EditorApplication.cpp
    src/EditorController.cpp
    src/Render/BoneRenderer.cpp
    src/Render/SpriteRenderer.cpp
    src/Panels/AssetPanel.cpp
    src/Panels/AssetBrowserPanel.cpp
    src/Panels/HierarchyPanel.cpp
    src/Panels/PropertyPanel.cpp
    src/Panels/ViewportPanel.cpp
    src/Panels/AnimationPanel.cpp
    src/Tools/BoneTool.cpp
    src/Tools/SpriteTool.cpp
    src/Tools/IKSolverTool.cpp
    src/Export/ExportManager.cpp
    src/Export/JSONExporter.cpp
    src/Export/PNGExporter.cpp
    src/Utils/AssetManager.cpp
    src/Utils/FileDialogManager.cpp
    src/Utils/miniz.c
    src/Project/ProjectManager.cpp
)

# Group SFML Debug Libraries
set(SFML_LIBS_DEBUG
    ${SFML_LIB_DIR}/sfml-graphics-s-d.lib
    ${SFML_LIB_DIR}/sfml-window-s-d.lib
    ${SFML_LIB_DIR}/sfml-system-s-d.lib
    ${SFML_LIB_DIR}/freetyped.lib
)

# Group SFML Release Libraries
set(SFML_LIBS_RELEASE
    ${SFML_LIB_DIR}/sfml-graphics-s.lib
    ${SFML_LIB_DIR}/sfml-window-s.lib
    ${SFML_LIB_DIR}/sfml-system-s.lib
    ${SFML_LIB_DIR}/freetype.lib
)

# Link libraries with debug/release variants
target_link_libraries(Riggle_Editor PRIVATE
    Riggle_Core
    # common
    ImGui-SFML
    # SFML libraries and Freetype
    $<$<CONFIG:Debug>:${SFML_LIBS_DEBUG}>
    $<$<CONFIG:Release>:${SFML_LIBS_RELEASE}>
)

# Windows dependencies (static linking)
if(WIN32)
    target_link_libraries(Riggle_Editor PRIVATE
        opengl32.lib
        winmm.lib
        gdi32.lib
    )
    
    # For static linking, we need to define these symbols
    target_compile_definitions(Riggle_Editor PRIVATE
        SFML_STATIC
        FREETYPE_STATIC
    )

    # Hide console window on Windows Release builds
    target_link_options(Riggle_Editor PRIVATE
        $<$<CONFIG:Debug>:/PDBALTPATH:${SFML_LIB_DIR}/Debug>
        $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
        $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>
    )
endif()

# Include directories
target_include_directories(Riggle_Editor PRIVATE 
    include
    # ${CMAKE_SOURCE_DIR}/common
    ${IMGUI_DIR}
    ${IMGUI_SFML_DIR}
    ${SFML_INCLUDE_DIR}
)