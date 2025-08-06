#include "Editor/Panels/AssetBrowserPanel.h"
#include "Editor/Utils/FileDialogManager.h"
#include <imgui.h>
#include <iostream>

namespace Riggle {

AssetBrowserPanel::AssetBrowserPanel() 
    : BasePanel("Asset Browser")
    , m_assetManager(std::make_unique<AssetManager>())
    , m_currentDirectory("assets/Sample_Character")
    , m_showDirectoryInput(false)
{
    // Try to scan default assets directory
    refreshAssets();
}

void AssetBrowserPanel::render() {
    if (!m_isVisible) return;

    ImGui::Begin(m_name.c_str(), &m_isVisible);

    // Directory controls
    ImGui::Text("Directory: %s", m_currentDirectory.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        openDirectoryDialog();
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        refreshAssets();
    }

    ImGui::Separator();

    // Asset list info and selection controls
    const auto& imageAssets = m_assetManager->getImageAssets();
    ImGui::Text("Available Assets: %zu", imageAssets.size());
    
    // Selection info and controls
    if (!imageAssets.empty()) {
        ImGui::SameLine();
        ImGui::Text("| Selected: %zu", m_selectedAssets.size());
        
        // Selection control buttons
        ImGui::Spacing();
        
        if (ImGui::Button("Select All")) {
            selectAll();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Select None")) {
            selectNone();
        }
        ImGui::SameLine();
        
        // Insert button - only enabled if assets are selected
        bool hasSelection = !m_selectedAssets.empty();
        if (!hasSelection) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }
        
        if (ImGui::Button("Insert Selected") && hasSelection) {
            insertSelected();
        }
        
        if (!hasSelection) {
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Select one or more assets to insert");
            }
        } else {
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Insert %zu selected asset(s) into the scene", m_selectedAssets.size());
            }
        }
    }

    ImGui::Separator();

    // Display image assets with checkboxes
    if (imageAssets.empty()) {
        ImGui::Text("No image assets found.");
        ImGui::Text("Place PNG/JPG images in the assets folder");
        ImGui::Text("and click 'Refresh'");
    } else {
        // Create a child region for scrollable asset list
        ImGui::BeginChild("AssetList", ImVec2(0, -30), true);

        // FIXED: Better column setup
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float checkboxWidth = 30.0f;
        float nameWidth = availableWidth - checkboxWidth - 10.0f; // Leave some padding
        
        // Use manual positioning instead of columns
        ImGui::Text("Asset Name");
        ImGui::SameLine(nameWidth + 5);
        ImGui::Text("Select");
        ImGui::Separator();

        // Asset rows with manual positioning
        for (const auto& asset : imageAssets) {
            ImGui::PushID(asset.path.c_str());
            
            // Get current cursor position
            ImVec2 startPos = ImGui::GetCursorPos();
            
            // Asset name area - make it clickable but not selectable
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0)); // Transparent
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.3f)); // Light gray on hover
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f)); // Darker gray when clicked
            
            // Create selectable for the name area
            bool singleSelected = ImGui::Selectable("##assetrow", false, 
                ImGuiSelectableFlags_AllowItemOverlap, ImVec2(nameWidth, 0));
            
            ImGui::PopStyleColor(3);
            
            if (singleSelected && m_onAssetSelected) {
                m_onAssetSelected(asset);
            }

            // Draw the asset name text over the selectable
            ImGui::SetCursorPos(ImVec2(startPos.x + 5, startPos.y + 2));
            ImGui::Text("%s", asset.name.c_str());
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Path: %s\nClick to add single asset\nUse checkbox for multiple selection", asset.path.c_str());
            }
            
            // Position checkbox on the right
            ImGui::SetCursorPos(ImVec2(startPos.x + nameWidth + 5, startPos.y + 2));
            
            bool isSelected = isAssetSelected(asset.path);
            if (ImGui::Checkbox("##select", &isSelected)) {
                toggleAssetSelection(asset.path);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(isSelected ? "Remove from selection" : "Add to selection");
            }
            
            // Move to next row
            ImGui::SetCursorPos(ImVec2(startPos.x, startPos.y + ImGui::GetTextLineHeightWithSpacing()));
            
            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    // Status
    ImGui::Separator();
    if (m_selectedAssets.empty()) {
        ImGui::Text("Status: Ready - Click asset names to add individually");
    } else {
        ImGui::Text("Status: %zu asset(s) selected - Click 'Insert Selected' to add all", m_selectedAssets.size());
    }

    ImGui::End();
}

void AssetBrowserPanel::update(sf::RenderWindow& window) {
    // No update logic needed for now
}

void AssetBrowserPanel::refreshAssets() {
    // Clear selection when refreshing (assets might have changed)
    m_selectedAssets.clear();
    scanDirectory(m_currentDirectory);
}

void AssetBrowserPanel::scanDirectory(const std::string& directory) {
    m_assetManager->scanDirectory(directory);
    std::cout << "Scanned directory: " << directory << std::endl;
    std::cout << "Found " << m_assetManager->getImageAssets().size() << " image assets" << std::endl;
}

void AssetBrowserPanel::openDirectoryDialog() {
    std::string selectedPath;
    if (FileDialogManager::getInstance().directoryDialog(selectedPath, "Select Assets Directory", m_currentDirectory)) {
        m_currentDirectory = selectedPath;
        refreshAssets();
        std::cout << "Asset directory changed to: " << selectedPath << std::endl;
    }
}

// Selection management functions
void AssetBrowserPanel::selectAll() {
    m_selectedAssets.clear();
    for (const auto& asset : m_assetManager->getImageAssets()) {
        m_selectedAssets.insert(asset.path);
    }
    std::cout << "Selected all " << m_selectedAssets.size() << " assets" << std::endl;
}

void AssetBrowserPanel::selectNone() {
    size_t count = m_selectedAssets.size();
    m_selectedAssets.clear();
    std::cout << "Deselected " << count << " assets" << std::endl;
}

void AssetBrowserPanel::insertSelected() {
    if (m_selectedAssets.empty()) return;
    
    // Build vector of selected assets
    std::vector<AssetInfo> selectedAssetInfos;
    const auto& allAssets = m_assetManager->getImageAssets();
    
    for (const auto& asset : allAssets) {
        if (isAssetSelected(asset.path)) {
            selectedAssetInfos.push_back(asset);
        }
    }
    
    // Call the multiple assets callback
    if (m_onMultipleAssetsSelected && !selectedAssetInfos.empty()) {
        m_onMultipleAssetsSelected(selectedAssetInfos);
        std::cout << "Inserted " << selectedAssetInfos.size() << " selected assets" << std::endl;
        
        // Optionally clear selection after insertion
        m_selectedAssets.clear();
    }
}

bool AssetBrowserPanel::isAssetSelected(const std::string& assetPath) const {
    return m_selectedAssets.find(assetPath) != m_selectedAssets.end();
}

void AssetBrowserPanel::toggleAssetSelection(const std::string& assetPath) {
    if (isAssetSelected(assetPath)) {
        m_selectedAssets.erase(assetPath);
        std::cout << "Deselected: " << assetPath << std::endl;
    } else {
        m_selectedAssets.insert(assetPath);
        std::cout << "Selected: " << assetPath << std::endl;
    }
}

void AssetBrowserPanel::setCurrentDirectory(const std::string& directory) {
    m_currentDirectory = directory;
    refreshAssets();
}

} // namespace Riggle