#include "Editor/Utils/DialogManager.h"
#include "Editor/Export/JSONExporter.h"
#include "Editor/Export/PNGExporter.h"
#include <filesystem>
#include <iostream>

namespace Riggle {

DialogManager::DialogManager()
    : m_exportManager(nullptr)
    , m_projectManager(nullptr)
    , m_showExitConfirmation(false)
    , m_showControlsDialog(false)
    , m_showAboutDialog(false)
    , m_showProjectSettingsDialog(false)
    , m_showExportDialog(false)
    , m_showLayoutResetConfirmation(false)
    , m_shouldExit(false)
    , m_hasUnsavedChanges(false)
    , m_layoutResetRequested(false)
    , m_projectSettingsChanged(false)
    , m_projectSettingsDialogInitialized(false)
    , m_exportCharacter(nullptr)
    , m_selectedProjectExporter(0)
    , m_selectedAnimationExporter(0)
    , m_exportProject(true)
{
    m_projectSettingsName[0] = '\0';
    m_projectSettingsAuthor[0] = '\0';
    m_projectSettingsDescription[0] = '\0';
    m_outputPath[0] = '\0';
    strcpy(m_projectName, "MyProject");
}

void DialogManager::initialize(ExportManager* exportManager, ProjectManager* projectManager) {
    m_exportManager = exportManager;
    m_projectManager = projectManager;
}

void DialogManager::render() {
    renderExitConfirmation();
    renderNewProjectConfirmation();
    renderControlsDialog();
    renderAboutDialog();
    renderProjectSettingsDialog();
    renderExportDialog();
    renderLayoutResetConfirmation();
}

void DialogManager::showExitConfirmation(bool hasUnsavedChanges) {
    m_hasUnsavedChanges = hasUnsavedChanges;
    m_showExitConfirmation = true;
}

void DialogManager::showNewProjectConfirmation(bool hasUnsavedChanges) {
    m_hasUnsavedChanges = hasUnsavedChanges;
    m_showNewProjectConfirmation = true;
    m_newProjectConfirmed = false;
}

void DialogManager::showControlsDialog() {
    m_showControlsDialog = true;
}

void DialogManager::showAboutDialog() {
    m_showAboutDialog = true;
}

void DialogManager::showProjectSettingsDialog(const ProjectMetadata& metadata) {
    m_originalMetadata = metadata;
    m_showProjectSettingsDialog = true;
    m_projectSettingsDialogInitialized = false;
}

void DialogManager::showExportDialog(Character* character) {
    m_exportCharacter = character;
    m_showExportDialog = true;
    m_lastExportError.clear();
    
    // Reset animation selections
    if (character) {
        const auto& animations = character->getAnimations();
        m_exportAnimationSelections.assign(animations.size(), false);
    }
}

void DialogManager::showLayoutResetConfirmation() {
    m_showLayoutResetConfirmation = true;
}

ProjectMetadata DialogManager::getUpdatedProjectMetadata() const {
    ProjectMetadata metadata = m_originalMetadata;
    metadata.name = m_projectSettingsName;
    metadata.author = m_projectSettingsAuthor;
    metadata.description = m_projectSettingsDescription;
    return metadata;
}

void DialogManager::updateProjectMetadata(const ProjectMetadata& metadata) {
    m_originalMetadata = metadata;
    
    // Update dialog buffers
    strncpy(m_projectSettingsName, metadata.name.c_str(), sizeof(m_projectSettingsName) - 1);
    m_projectSettingsName[sizeof(m_projectSettingsName) - 1] = '\0';
    
    strncpy(m_projectSettingsAuthor, metadata.author.c_str(), sizeof(m_projectSettingsAuthor) - 1);
    m_projectSettingsAuthor[sizeof(m_projectSettingsAuthor) - 1] = '\0';
    
    strncpy(m_projectSettingsDescription, metadata.description.c_str(), sizeof(m_projectSettingsDescription) - 1);
    m_projectSettingsDescription[sizeof(m_projectSettingsDescription) - 1] = '\0';
    
    m_projectSettingsDialogInitialized = true;
}

void DialogManager::resetProjectSettingsDialog() {
    m_projectSettingsDialogInitialized = false;
    m_projectSettingsChanged = false;
}

void DialogManager::renderExitConfirmation() {
    if (m_showExitConfirmation) {
        ImGui::OpenPopup("Exit Riggle");
    }
    
    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Exit Riggle", nullptr, 
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        
        ImGui::Text("Are you sure you want to exit Riggle?");
        
        if (m_hasUnsavedChanges) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "You have unsaved changes!");
            ImGui::Text("Your work will be lost if you exit without saving.");
        }
        
        ImGui::Separator();
        ImGui::Spacing();
        
        // Button layout
        float buttonWidth = 120.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = m_hasUnsavedChanges ? (buttonWidth * 3 + spacing * 2) : (buttonWidth * 2 + spacing);
        float startPos = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
        
        if (startPos > 0) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startPos);
        }
        
        if (m_hasUnsavedChanges) {
            // Save and Exit button
            if (ImGui::Button("Save & Exit", ImVec2(buttonWidth, 0))) {
                if (m_onSaveAndExit) m_onSaveAndExit();
                m_shouldExit = true;
                m_showExitConfirmation = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
        }
        
        // Exit without saving
        if (ImGui::Button("Exit", ImVec2(buttonWidth, 0))) {
            if (m_onExit) m_onExit();
            m_shouldExit = true;
            m_showExitConfirmation = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        
        // Cancel
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
            m_showExitConfirmation = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void DialogManager::renderNewProjectConfirmation() {
    if (m_showNewProjectConfirmation) {
        ImGui::OpenPopup("New Project?");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("New Project?", &m_showNewProjectConfirmation, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to create a new project?");
        if (m_hasUnsavedChanges) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "You have unsaved changes!");
            ImGui::Text("Your work will be lost if you continue without saving.");
        }
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Continue")) {
            m_newProjectConfirmed = true;
            m_showNewProjectConfirmation = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_newProjectConfirmed = false;
            m_showNewProjectConfirmation = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DialogManager::renderControlsDialog() {
    if (!m_showControlsDialog) return;
    
    if (!ImGui::IsPopupOpen("Controls")) {
        ImGui::OpenPopup("Controls");
    }
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::BeginPopupModal("Controls", &m_showControlsDialog)) {
        ImGui::Text("Riggle Controls:");
        ImGui::Separator();
        
        ImGui::Text("Mouse Controls:");
        ImGui::BulletText("Left click: Select sprite/bone");
        ImGui::BulletText("Left drag: Move unbound sprite");
        ImGui::BulletText("Right/Middle drag: Pan view");
        ImGui::BulletText("Mouse wheel: Zoom in/out");
        
        ImGui::Spacing();
        ImGui::Text("Keyboard Shortcuts:");
        ImGui::BulletText("1: Sprite Tool");
        ImGui::BulletText("2: Bone Tool");
        ImGui::BulletText("ESC: Exit application");
        
        ImGui::Spacing();
        if (ImGui::Button("Close")) {
            m_showControlsDialog = false;
        }
        
        ImGui::EndPopup();
    }
}

void DialogManager::renderAboutDialog() {
    if (!m_showAboutDialog) return;
    
    if (!ImGui::IsPopupOpen("About Riggle")) {
        ImGui::OpenPopup("About Riggle");
    }
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("About Riggle", &m_showAboutDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Riggle 2D Animation Tool");
        ImGui::Separator();
        ImGui::Text("Developed By:");
        ImGui::BulletText("Bipul Gautam");
        ImGui::BulletText("Bishal Khatiwada");
        ImGui::BulletText("Bishal Rimal");
        
        ImGui::Spacing();
        if (ImGui::Button("Close")) {
            m_showAboutDialog = false;
        }
        
        ImGui::EndPopup();
    }
}

void DialogManager::renderProjectSettingsDialog() {
    if (!m_showProjectSettingsDialog) return;

    if (!ImGui::IsPopupOpen("Project Settings")) {
        ImGui::OpenPopup("Project Settings");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("Project Settings", &m_showProjectSettingsDialog)) {
        // Initialize with current values if not already done
        if (!m_projectSettingsDialogInitialized) {
            strncpy(m_projectSettingsName, m_originalMetadata.name.c_str(), sizeof(m_projectSettingsName) - 1);
            m_projectSettingsName[sizeof(m_projectSettingsName) - 1] = '\0';
            strncpy(m_projectSettingsAuthor, m_originalMetadata.author.c_str(), sizeof(m_projectSettingsAuthor) - 1);
            m_projectSettingsAuthor[sizeof(m_projectSettingsAuthor) - 1] = '\0';
            strncpy(m_projectSettingsDescription, m_originalMetadata.description.c_str(), sizeof(m_projectSettingsDescription) - 1);
            m_projectSettingsDescription[sizeof(m_projectSettingsDescription) - 1] = '\0';
            m_projectSettingsDialogInitialized = true;
        }

        ImGui::Text("Project Information:");
        ImGui::Separator();

        ImGui::InputText("Project Name", m_projectSettingsName, sizeof(m_projectSettingsName));
        ImGui::InputText("Author", m_projectSettingsAuthor, sizeof(m_projectSettingsAuthor));
        ImGui::InputTextMultiline("Description", m_projectSettingsDescription, sizeof(m_projectSettingsDescription), ImVec2(0, 100));

        ImGui::Separator();
        ImGui::Text("Created: %s", m_originalMetadata.createdDate.c_str());
        ImGui::Text("Modified: %s", m_originalMetadata.modifiedDate.c_str());

        ImGui::Separator();

        if (ImGui::Button("Save")) {
            m_projectSettingsChanged = true;
            if (m_onProjectSettingsSave) {
                m_onProjectSettingsSave(getUpdatedProjectMetadata());
            }
            m_showProjectSettingsDialog = false;
            m_projectSettingsDialogInitialized = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_showProjectSettingsDialog = false;
            m_projectSettingsDialogInitialized = false;
        }

        ImGui::EndPopup();
    }
}

void DialogManager::renderExportDialog() {
    if (!m_showExportDialog) return;

    // Open the popup immediately when flag is set
    if (!ImGui::IsPopupOpen("Export")) {
        ImGui::OpenPopup("Export");
    }
    
    // Center the dialog
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::BeginPopupModal("Export", &m_showExportDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Export JSON data or PNG image sequence");
        ImGui::Separator();
        
        // Export type selection
        if (ImGui::RadioButton("Export JSON", m_exportProject)) {
            m_exportProject = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Export PNG sequence", !m_exportProject)) {
            m_exportProject = false;
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (m_exportProject) {
            // Project export options
            ImGui::Text("JSON Export Settings:");
            ImGui::InputText("Project Name", m_projectName, sizeof(m_projectName));
            
            if (m_exportManager) {
                auto projectExporters = m_exportManager->getProjectExporters();
                if (!projectExporters.empty()) {
                    ImGui::Text("Export Format:");
                    for (int i = 0; i < static_cast<int>(projectExporters.size()); ++i) {
                        bool selected = (m_selectedProjectExporter == i);
                        if (ImGui::RadioButton(projectExporters[i]->getFormatName().c_str(), selected)) {
                            m_selectedProjectExporter = i;
                        }
                        
                        // Show file extension
                        ImGui::SameLine();
                        ImGui::TextDisabled("(%s)", projectExporters[i]->getFileExtension().c_str());
                    }
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No project exporters available!");
                }
            }
        } else {
            // Animation export options
            ImGui::Text("PNG Sequence Export Settings:");
            
            // Resolution presets
            static const char* resolutions[] = { "1280x720", "1920x1080", "2560x1440" };
            static int resIdx = 1;
            ImGui::Text("Resolution:");
            if (ImGui::Combo("##Resolution", &resIdx, resolutions, IM_ARRAYSIZE(resolutions))) {
                if (m_exportManager) {
                    auto animationExporters = m_exportManager->getAnimationExporters();
                    if (m_selectedAnimationExporter < static_cast<int>(animationExporters.size())) {
                        auto* pngExporter = dynamic_cast<PNGSequenceExporter*>(animationExporters[m_selectedAnimationExporter]);
                        if (pngExporter) pngExporter->setResolutionPreset(resIdx);
                    }
                }
            }

            // Aspect ratio presets
            static const char* aspectRatios[] = { "16:9", "4:3", "1:1", "21:9" };
            static int aspectIdx = 0;
            ImGui::Text("Aspect Ratio:");
            if (ImGui::Combo("##AspectRatio", &aspectIdx, aspectRatios, IM_ARRAYSIZE(aspectRatios))) {
                if (m_exportManager) {
                    auto animationExporters = m_exportManager->getAnimationExporters();
                    if (m_selectedAnimationExporter < static_cast<int>(animationExporters.size())) {
                        auto* pngExporter = dynamic_cast<PNGSequenceExporter*>(animationExporters[m_selectedAnimationExporter]);
                        if (pngExporter) pngExporter->setAspectRatioIndex(aspectIdx);
                    }
                }
            }

            // Frame rate presets
            static const char* fpsOptions[] = { "24", "30", "60" };
            static int fpsIdx = 1;
            ImGui::Text("Frame Rate:");
            if (ImGui::Combo("##FrameRate", &fpsIdx, fpsOptions, IM_ARRAYSIZE(fpsOptions))) {
                int fps = 24;
                if (fpsIdx == 1) fps = 30;
                else if (fpsIdx == 2) fps = 60;
                if (m_exportManager) {
                    auto animationExporters = m_exportManager->getAnimationExporters();
                    if (m_selectedAnimationExporter < static_cast<int>(animationExporters.size())) {
                        auto* pngExporter = dynamic_cast<PNGSequenceExporter*>(animationExporters[m_selectedAnimationExporter]);
                        if (pngExporter) pngExporter->setFrameRate(fps);
                    }
                }
            }

            // Zoom slider
            static float zoom = 1.0f;
            ImGui::Text("Zoom:");
            if (ImGui::SliderFloat("##Zoom", &zoom, 0.1f, 2.0f, "%.2fx")) {
                if (m_exportManager) {
                    auto animationExporters = m_exportManager->getAnimationExporters();
                    if (m_selectedAnimationExporter < static_cast<int>(animationExporters.size())) {
                        auto* pngExporter = dynamic_cast<PNGSequenceExporter*>(animationExporters[m_selectedAnimationExporter]);
                        if (pngExporter) pngExporter->setZoom(zoom);
                    }
                }
            }

            // Background color picker
            static float bgColor[4] = { 1, 1, 1, 0 }; // Default transparent
            ImGui::Text("Background Color:");
            if (ImGui::ColorEdit4("##BGColor", bgColor)) {
                sf::Color color(
                    static_cast<std::uint8_t>(bgColor[0] * 255),
                    static_cast<std::uint8_t>(bgColor[1] * 255),
                    static_cast<std::uint8_t>(bgColor[2] * 255),
                    static_cast<std::uint8_t>(bgColor[3] * 255)
                );
                if (m_exportManager) {
                    auto animationExporters = m_exportManager->getAnimationExporters();
                    if (m_selectedAnimationExporter < static_cast<int>(animationExporters.size())) {
                        auto* pngExporter = dynamic_cast<PNGSequenceExporter*>(animationExporters[m_selectedAnimationExporter]);
                        if (pngExporter) pngExporter->setBackgroundColor(color);
                    }
                }
            }
            
            // Show animations to select
            if (m_exportCharacter) {
                const auto& animations = m_exportCharacter->getAnimations();
                // Resize selection vector if needed
                if (m_exportAnimationSelections.size() != animations.size()) {
                    m_exportAnimationSelections.assign(animations.size(), false);
                }
                if (animations.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No animations available to export!");
                } else {
                    ImGui::Text("Select animations to export:");
                    if (ImGui::Button("Select All")) {
                        std::fill(m_exportAnimationSelections.begin(), m_exportAnimationSelections.end(), true);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Select None")) {
                        std::fill(m_exportAnimationSelections.begin(), m_exportAnimationSelections.end(), false);
                    }
                    for (size_t i = 0; i < animations.size(); ++i) {
                        if (animations[i]) {
                            bool checked = m_exportAnimationSelections[i];
                            if (ImGui::Checkbox(animations[i]->getName().c_str(), &checked)) {
                                m_exportAnimationSelections[i] = checked ? true : false;
                            }
                        }
                    }
                }
            }
            
            if (m_exportManager) {
                auto animationExporters = m_exportManager->getAnimationExporters();
                if (!animationExporters.empty()) {
                    ImGui::Text("Export Format:");
                    for (int i = 0; i < static_cast<int>(animationExporters.size()); ++i) {
                        bool selected = (m_selectedAnimationExporter == i);
                        if (ImGui::RadioButton(animationExporters[i]->getFormatName().c_str(), selected)) {
                            m_selectedAnimationExporter = i;
                        }
                        
                        // Show format description
                        ImGui::SameLine();
                        std::string ext = animationExporters[i]->getFileExtension();
                        if (ext.empty()) {
                            ImGui::TextDisabled("(Directory)");
                        } else {
                            ImGui::TextDisabled("(%s)", ext.c_str());
                        }
                    }
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No animation exporters available!");
                }
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Output path selection
        ImGui::Text("Output Path:");
        ImGui::InputText("##OutputPath", m_outputPath, sizeof(m_outputPath));
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            std::string selectedPath;
            if (m_exportProject) {
                // JSON export: show save file dialog with default name and .json filter
                std::vector<FileFilter> filters = {
                    FileFilter("JSON Files", ".json"),
                    FileFilter("All Files", ".*")
                };
                std::string defaultName = std::string(m_projectName) + ".json";
                if (FileDialogManager::getInstance().saveFileDialog(selectedPath, filters, "", defaultName)) {
                    // strncpy(m_outputPath, selectedPath.c_str(), sizeof(m_outputPath) - 1);
                    // m_outputPath[sizeof(m_outputPath) - 1] = '\0';
                    // performExport();

                    // Extract directory and filename
                    std::filesystem::path p(selectedPath);
                    strncpy(m_outputPath, p.parent_path().string().c_str(), sizeof(m_outputPath) - 1);
                    m_outputPath[sizeof(m_outputPath) - 1] = '\0';
                    strncpy(m_projectName, p.stem().string().c_str(), sizeof(m_projectName) - 1);
                    m_projectName[sizeof(m_projectName) - 1] = '\0';
                    // Perform export immediately
                    performExport();
                }
            } else {
                // PNG sequence: directory dialog logic
                bool isDirectory = false;
                if (m_exportManager) {
                    auto animationExporters = m_exportManager->getAnimationExporters();
                    if (m_selectedAnimationExporter < static_cast<int>(animationExporters.size())) {
                        isDirectory = animationExporters[m_selectedAnimationExporter]->getFileExtension().empty();
                    }
                }
                if (openFileDialog(selectedPath, isDirectory)) {
                    strncpy(m_outputPath, selectedPath.c_str(), sizeof(m_outputPath) - 1);
                    m_outputPath[sizeof(m_outputPath) - 1] = '\0';
                }
            }
        }
        
        // Show any export errors
        if (!m_lastExportError.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Export Error:");
            ImGui::TextWrapped("%s", m_lastExportError.c_str());
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Buttons
        float buttonWidth = 100.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = buttonWidth * 2 + spacing;
        float startPos = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
        
        if (startPos > 0) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startPos);
        }
        
        // Export button
        bool canExport = strlen(m_outputPath) > 0 && m_exportCharacter;
        if (!canExport) {
            ImGui::BeginDisabled();
        }
        
        if (ImGui::Button("Export", ImVec2(buttonWidth, 0))) {
            performExport();
        }
        
        if (!canExport) {
            ImGui::EndDisabled();
        }
        
        ImGui::SameLine();
        
        // Cancel button
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
            m_showExportDialog = false;
            m_lastExportError.clear();
        }
        
        ImGui::EndPopup();
    }
}

void DialogManager::renderLayoutResetConfirmation() {
    if (m_showLayoutResetConfirmation) {
        ImGui::OpenPopup("Reset Layout?");
    }

    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Reset Layout?", &m_showLayoutResetConfirmation, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Spacing();
        ImGui::Text("Are you sure you want to reset the panel layout?");
        ImGui::Spacing();

        if (ImGui::Button("Yes")) {
            m_layoutResetRequested = true;
            if (m_onLayoutReset) {
                m_onLayoutReset();
            }
            m_showLayoutResetConfirmation = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            m_showLayoutResetConfirmation = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DialogManager::performExport() {
    if (!m_exportCharacter || !m_exportManager) {
        m_lastExportError = "No character or export manager available";
        return;
    }
    
    m_lastExportError.clear();
    bool success = false;
    
    try {
        if (m_exportProject) {
            auto projectExporters = m_exportManager->getProjectExporters();
            if (m_selectedProjectExporter < static_cast<int>(projectExporters.size())) {
                auto* exporter = projectExporters[m_selectedProjectExporter];

                // Build output path: directory + filename + extension
                std::string outputDir = m_outputPath;
                std::string extension = exporter->getFileExtension();
                std::string projectNameStr = m_projectName;
                if (outputDir.empty()) {
                    m_lastExportError = "Please specify an output directory.";
                    return;
                }
                std::filesystem::path outPath = std::filesystem::path(outputDir) / (projectNameStr + extension);

                success = m_exportManager->exportProject(*m_exportCharacter, projectNameStr, exporter, outPath.string());

                if (success) {
                    std::cout << "Project exported successfully to: " << outPath.string() << std::endl;
                    m_showExportDialog = false;
                } else {
                    m_lastExportError = m_exportManager->getLastError();
                }
            } else {
                m_lastExportError = "Invalid project exporter selected";
            }
        } else {
            // Export animations
            auto animationExporters = m_exportManager->getAnimationExporters();
            if (m_selectedAnimationExporter < static_cast<int>(animationExporters.size())) {
                auto* exporter = animationExporters[m_selectedAnimationExporter];

                const auto& animations = m_exportCharacter->getAnimations();
                bool anyExported = false;
                for (size_t i = 0; i < m_exportAnimationSelections.size(); ++i) {
                    if (m_exportAnimationSelections[i] && animations[i]) {
                        std::string animName = animations[i]->getName();
                        std::string animationFolder = std::string(m_outputPath) + "/" + animName;
                        bool ok = m_exportManager->exportAnimation(*m_exportCharacter, animName, exporter, animationFolder);
                        if (ok) {
                            std::cout << "Animation exported successfully to: " << animationFolder << std::endl;
                            anyExported = true;
                        } else {
                            m_lastExportError += m_exportManager->getLastError() + "\n";
                        }
                    }
                }
                if (anyExported) {
                    m_showExportDialog = false;
                }
                if (!anyExported && m_lastExportError.empty()) {
                    m_lastExportError = "No animations selected for export";
                }
            }
        }
    }
    catch (const std::exception& e) {
        m_lastExportError = "Exception during export: " + std::string(e.what());
    }
}

bool DialogManager::openFileDialog(std::string& selectedPath, bool isDirectory) {
    if (isDirectory) {
        return FileDialogManager::getInstance().directoryDialog(selectedPath, "Select Export Directory");
    } else {
        std::vector<FileFilter> filters = {
            FileFilter("All Files", ".*")
        };
        return FileDialogManager::getInstance().openFileDialog(selectedPath, filters);
    }
}

} // namespace Riggle