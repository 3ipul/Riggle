#include "Editor/EditorController.h"
#include <imgui.h>
#include <iostream>

namespace Riggle {

EditorController::EditorController() 
    : m_assetBrowserPanel(nullptr)
    , m_scenePanel(nullptr)
    , m_spriteInspectorPanel(nullptr)
    , m_currentWindow(nullptr)
{
    initializePanels();
    setupPanelCallbacks();
}

void EditorController::update(sf::RenderWindow& window) {
    m_currentWindow = &window;
    
    // Update all panels
    for (auto& panel : m_panels) {
        if (panel && panel->isVisible()) {
            panel->update(window);
        }
    }
}

void EditorController::render(sf::RenderWindow& window) {
    m_currentWindow = &window;
    
    // Render main menu bar
    renderMainMenuBar();
    
    // Render all panels (viewport rendering is now handled within ScenePanel)
    for (auto& panel : m_panels) {
        if (panel && panel->isVisible()) {
            panel->render();
        }
    }
}

void EditorController::handleEvent(const sf::Event& event) {
    // Forward events to panels
    for (auto& panel : m_panels) {
        if (panel && panel->isVisible()) {
            panel->handleEvent(event);
        }
    }
}

void EditorController::renderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {
                // Reset scene
                if (m_scenePanel) {
                    auto newCharacter = std::make_unique<Character>("New Character");
                    auto rig = std::make_unique<Rig>("New Rig");
                    newCharacter->setRig(std::move(rig));
                    m_scenePanel->setCharacter(std::move(newCharacter));
                    
                    // Update sprite inspector with new character
                    if (m_spriteInspectorPanel) {
                        m_spriteInspectorPanel->setCharacter(m_scenePanel->getCharacter());
                        m_spriteInspectorPanel->setSelectedSprite(nullptr);
                    }
                }
            }
            if (ImGui::MenuItem("Save Project")) {
                // TODO: Implement project saving
                std::cout << "Save Project - not implemented yet" << std::endl;
            }
            if (ImGui::MenuItem("Load Project")) {
                // TODO: Implement project loading
                std::cout << "Load Project - not implemented yet" << std::endl;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export Animation")) {
                // TODO: Implement animation export
                std::cout << "Export Animation - not implemented yet" << std::endl;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                if (m_currentWindow) {
                    m_currentWindow->close();
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                // TODO: Implement undo system
                std::cout << "Undo - not implemented yet" << std::endl;
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                // TODO: Implement redo system
                std::cout << "Redo - not implemented yet" << std::endl;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Selected", "Delete")) {
                if (m_spriteInspectorPanel && m_spriteInspectorPanel->getSelectedSprite()) {
                    // TODO: Implement sprite deletion
                    std::cout << "Delete Selected Sprite - not implemented yet" << std::endl;
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            for (auto& panel : m_panels) {
                if (panel) {
                    bool visible = panel->isVisible();
                    if (ImGui::MenuItem(panel->getName().c_str(), nullptr, &visible)) {
                        panel->setVisible(visible);
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                // Reset all panels to visible
                for (auto& panel : m_panels) {
                    if (panel) {
                        panel->setVisible(true);
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Scene View")) {
                if (m_scenePanel) {
                    m_scenePanel->resetView();
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Refresh Assets")) {
                if (m_assetBrowserPanel) {
                    // TODO: Add refresh method to AssetBrowserPanel
                    std::cout << "Refresh Assets - not implemented yet" << std::endl;
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Create Bone Tool")) {
                // TODO: Implement bone creation tool
                std::cout << "Create Bone Tool - not implemented yet" << std::endl;
            }
            if (ImGui::MenuItem("Animation Timeline")) {
                // TODO: Implement animation timeline
                std::cout << "Animation Timeline - not implemented yet" << std::endl;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Controls")) {
                // TODO: Show controls help window
                std::cout << "Controls Help:" << std::endl;
                std::cout << "- Left click: Select sprite" << std::endl;
                std::cout << "- Left drag: Move sprite" << std::endl;
                std::cout << "- Right drag: Pan view" << std::endl;
                std::cout << "- Arrow keys/WASD: Move selected sprite" << std::endl;
                std::cout << "- Shift + Movement: 5x faster" << std::endl;
            }
            if (ImGui::MenuItem("About Riggle")) {
                // TODO: Show about dialog
                std::cout << "Riggle 2D Animation Tool v1.0" << std::endl;
            }
            ImGui::EndMenu();
        }
        
        // Status information in the menu bar
        ImGui::Separator();
        if (m_scenePanel && m_scenePanel->getCharacter()) {
            size_t spriteCount = m_scenePanel->getCharacter()->getSprites().size();
            ImGui::Text("Sprites: %zu", spriteCount);
        }
        
        if (m_spriteInspectorPanel && m_spriteInspectorPanel->getSelectedSprite()) {
            ImGui::Text("| Selected: %s", m_spriteInspectorPanel->getSelectedSprite()->getName().c_str());
        }
        
        ImGui::EndMainMenuBar();
    }
}

void EditorController::renderViewport() {
    // Viewport rendering is now handled within ScenePanel
}

void EditorController::initializePanels() {
    // Create asset browser panel
    auto assetBrowser = std::make_unique<AssetBrowserPanel>();
    m_assetBrowserPanel = assetBrowser.get();
    m_panels.push_back(std::move(assetBrowser));
    
    // Create scene panel
    auto scenePanel = std::make_unique<ScenePanel>();
    m_scenePanel = scenePanel.get();
    m_panels.push_back(std::move(scenePanel));
    
    // Create sprite inspector panel
    auto spriteInspector = std::make_unique<SpriteInspectorPanel>();
    m_spriteInspectorPanel = spriteInspector.get();
    m_panels.push_back(std::move(spriteInspector));
    
    std::cout << "Initialized " << m_panels.size() << " panels" << std::endl;
}

void EditorController::setupPanelCallbacks() {
    // Setup asset browser callback
    if (m_assetBrowserPanel) {
        m_assetBrowserPanel->setOnAssetSelected(
            [this](const AssetInfo& asset) {
                onAssetSelected(asset);
            }
        );
    }
    
    // Setup scene panel callback
    if (m_scenePanel) {
        m_scenePanel->setOnSpriteSelected(
            [this](Sprite* sprite) {
                onSpriteSelected(sprite);
            }
        );
    }
    
    // Setup sprite inspector callback
    if (m_spriteInspectorPanel) {
        m_spriteInspectorPanel->setOnSpriteSelected(
            [this](Sprite* sprite) {
                onSpriteSelected(sprite);
            }
        );
    }
    
    // Connect sprite inspector to scene panel's character
    if (m_scenePanel && m_spriteInspectorPanel) {
        m_spriteInspectorPanel->setCharacter(m_scenePanel->getCharacter());
    }
    
    std::cout << "Panel callbacks setup complete" << std::endl;
}

void EditorController::onAssetSelected(const AssetInfo& asset) {
    if (!m_scenePanel) return;
    
    // Add sprite to scene at center of current view
    sf::Vector2f centerPosition = m_scenePanel->getView().getCenter();
    m_scenePanel->addSpriteFromAsset(asset, centerPosition);
    
    std::cout << "Added sprite from asset: " << asset.name << std::endl;
}

void EditorController::onSpriteSelected(Sprite* sprite) {
    // Sync selection between scene panel and sprite inspector
    if (m_spriteInspectorPanel) {
        m_spriteInspectorPanel->setSelectedSprite(sprite);
    }
    
    if (sprite) {
        std::cout << "Sprite selected: " << sprite->getName() << std::endl;
    } else {
        std::cout << "No sprite selected" << std::endl;
    }
}

} // namespace Riggle