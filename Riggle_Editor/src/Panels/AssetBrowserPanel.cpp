#include "Editor/Panels/AssetBrowserPanel.h"
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
        m_showDirectoryInput = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        refreshAssets();
    }

    // Directory input dialog
    if (m_showDirectoryInput) {
        ImGui::OpenPopup("Select Directory");
    }

    if (ImGui::BeginPopupModal("Select Directory", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char dirBuffer[256];
        strncpy(dirBuffer, m_currentDirectory.c_str(), sizeof(dirBuffer) - 1);
        dirBuffer[sizeof(dirBuffer) - 1] = '\0';

        ImGui::Text("Enter directory path:");
        if (ImGui::InputText("##directory", dirBuffer, sizeof(dirBuffer))) {
            // Input changed
        }

        if (ImGui::Button("OK")) {
            m_currentDirectory = dirBuffer;
            refreshAssets();
            m_showDirectoryInput = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_showDirectoryInput = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::Separator();

    // Asset list
    ImGui::Text("Available Assets:");
    ImGui::Text("Images: %zu", m_assetManager->getImageAssets().size());

    ImGui::Separator();

    // Display image assets
    if (m_assetManager->getImageAssets().empty()) {
        ImGui::Text("No image assets found.");
        ImGui::Text("Place PNG/JPG images in the assets folder");
        ImGui::Text("and click 'Refresh'");
    } else {
        // Create a child region for scrollable asset list
        ImGui::BeginChild("AssetList", ImVec2(0, -30), true);

        for (const auto& asset : m_assetManager->getImageAssets()) {
            bool selected = ImGui::Selectable(asset.name.c_str(), false, ImGuiSelectableFlags_DontClosePopups);
            
            if (selected && m_onAssetSelected) {
                m_onAssetSelected(asset);
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Path: %s\nClick to add to scene", asset.path.c_str());
            }
        }

        ImGui::EndChild();
    }

    // Status
    ImGui::Separator();
    ImGui::Text("Status: Ready");

    ImGui::End();
}

void AssetBrowserPanel::update(sf::RenderWindow& window) {
    // No update logic needed for now
}

void AssetBrowserPanel::refreshAssets() {
    scanDirectory(m_currentDirectory);
}

void AssetBrowserPanel::scanDirectory(const std::string& directory) {
    m_assetManager->scanDirectory(directory);
    std::cout << "Scanned directory: " << directory << std::endl;
    std::cout << "Found " << m_assetManager->getImageAssets().size() << " image assets" << std::endl;
}

} // namespace Riggle