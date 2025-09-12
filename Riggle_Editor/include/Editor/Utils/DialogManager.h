#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <functional>
#include <Riggle/Character.h>
#include "Editor/Project/ProjectManager.h"
#include "Editor/Export/ExportManager.h"
#include "Editor/Utils/FileDialogManager.h"

namespace Riggle {

class DialogManager {
public:
    DialogManager();
    ~DialogManager() = default;

    // Initialize with required dependencies
    void initialize(ExportManager* exportManager, ProjectManager* projectManager);

    // Main render method - call this every frame
    void render();

    // Dialog show/hide methods
    void showExitConfirmation(bool hasUnsavedChanges);
    void showControlsDialog();
    void showAboutDialog();
    void showProjectSettingsDialog(const ProjectMetadata& metadata);
    void showExportDialog(Character* character);
    void showLayoutResetConfirmation();
    void showNewProjectConfirmation(bool hasUnsavedChanges);

    // State checks
    bool shouldExit() const { return m_shouldExit; }
    bool isNewProjectConfirmed() const { return m_newProjectConfirmed; }
    bool isLayoutResetRequested() const { return m_layoutResetRequested; }
    bool hasProjectSettingsChanged() const { return m_projectSettingsChanged; }

    // Get updated project metadata
    ProjectMetadata getUpdatedProjectMetadata() const;

    // Reset flags
    void clearExitFlag() { m_shouldExit = false; }
    void clearNewProjectConfirmedFlag() { m_newProjectConfirmed = false; }
    void clearLayoutResetFlag() { m_layoutResetRequested = false; }
    void clearProjectSettingsChangedFlag() { m_projectSettingsChanged = false; }

    // Callbacks
    void setOnSaveAndExit(std::function<void()> callback) { m_onSaveAndExit = callback; }
    void setOnExit(std::function<void()> callback) { m_onExit = callback; }
    void setOnLayoutReset(std::function<void()> callback) { m_onLayoutReset = callback; }
    void setOnProjectSettingsSave(std::function<void(const ProjectMetadata&)> callback) { m_onProjectSettingsSave = callback; }
    
    // Project metadata management
    void updateProjectMetadata(const ProjectMetadata& metadata);
    void resetProjectSettingsDialog();

private:
    // Dependencies
    ExportManager* m_exportManager;
    ProjectManager* m_projectManager;

    // Dialog states
    bool m_showExitConfirmation;
    bool m_showControlsDialog;
    bool m_showAboutDialog;
    bool m_showProjectSettingsDialog;
    bool m_showExportDialog;
    bool m_showLayoutResetConfirmation;

    // Exit dialog state
    bool m_shouldExit;
    bool m_hasUnsavedChanges;

    // New Project confirmation dialog state
    bool m_showNewProjectConfirmation = false;
    bool m_newProjectConfirmed = false;

    // Layout reset state
    bool m_layoutResetRequested;

    // Project settings dialog state
    bool m_projectSettingsChanged;
    bool m_projectSettingsDialogInitialized;
    char m_projectSettingsName[256];
    char m_projectSettingsAuthor[256];
    char m_projectSettingsDescription[512];
    ProjectMetadata m_originalMetadata;

    // Export dialog state
    Character* m_exportCharacter;
    int m_selectedProjectExporter;
    int m_selectedAnimationExporter;
    bool m_exportProject; // true for project, false for animation
    char m_outputPath[512];
    char m_projectName[256];
    std::string m_lastExportError;
    std::vector<bool> m_exportAnimationSelections;

    // Callbacks
    std::function<void()> m_onSaveAndExit;
    std::function<void()> m_onExit;
    std::function<void()> m_onLayoutReset;
    std::function<void(const ProjectMetadata&)> m_onProjectSettingsSave;

    // Individual dialog render methods
    void renderExitConfirmation();
    void renderNewProjectConfirmation();
    void renderControlsDialog();
    void renderAboutDialog();
    void renderProjectSettingsDialog();
    void renderExportDialog();
    void renderLayoutResetConfirmation();

    // Export helper methods
    void performExport();
    bool openFileDialog(std::string& selectedPath, bool isDirectory = false);
};

} // namespace Riggle