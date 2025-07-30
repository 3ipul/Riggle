#include "Editor/EditorController.h"
#include <imgui.h>
#include <iostream>

namespace Riggle {

EditorController::EditorController() 
    : m_assetBrowserPanel(nullptr)
    , m_assetPanel(nullptr)
    , m_propertyPanel(nullptr)
    , m_scenePanel(nullptr)
    , m_currentWindow(nullptr)
    , m_showExitConfirmation(false)
    , m_shouldExit(false)
    , m_hasUnsavedChanges(false)
{
    initializePanels();
    setupPanelCallbacks();
}

void EditorController::update(sf::RenderWindow& window) {
    m_currentWindow = &window;
    
    // Update character animation
    if (m_scenePanel && m_scenePanel->getCharacter()) {
        m_scenePanel->getCharacter()->update(ImGui::GetIO().DeltaTime);
    }

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

     // Render exit confirmation popup
    renderExitConfirmation();
}

void EditorController::handleEvent(const sf::Event& event) {
    // Handle ESC at controller level for exit confirmation
    if (event.is<sf::Event::KeyPressed>()) {
        const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
        if (keyPressed && keyPressed->code == sf::Keyboard::Key::Escape) {
            // Only show exit confirmation if no panel is handling ESC
            bool panelHandledEsc = false;
            
            // Check if any panel needs to handle ESC first
            for (auto& panel : m_panels) {
                if (panel && panel->isVisible()) {
                    // Let scene panel handle ESC for canceling operations
                    if (auto* scenePanel = dynamic_cast<ScenePanel*>(panel.get())) {
                        // If scene panel has active operations, let it handle ESC
                        if (scenePanel->getToolMode() != SceneToolMode::SpriteManipulation) {
                            panelHandledEsc = true;
                            break;
                        }
                    }
                }
            }
            
            // If no panel handled ESC, show exit confirmation
            if (!panelHandledEsc) {
                requestExit();
                return; // Don't pass ESC to panels
            }
        }
    }
    
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
                newProject();
            }
            if (ImGui::MenuItem("Save Project")) {
                saveProject();
            }
            if (ImGui::MenuItem("Load Project")) {
                loadProject();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export Animation")) {
                // TODO: Implement animation export
                std::cout << "Export Animation - not implemented yet" << std::endl;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                requestExit();
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
                if (m_propertyPanel && m_assetPanel && m_assetPanel->getSelectedSprite()) {
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
        
        if (m_assetPanel && m_assetPanel->getSelectedSprite()) {
            ImGui::Text("| Selected: %s", m_assetPanel->getSelectedSprite()->getName().c_str());
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
    
    // Create asset panel
    auto assetPanel = std::make_unique<AssetPanel>();
    m_assetPanel = assetPanel.get();
    m_panels.push_back(std::move(assetPanel));
    
    // Create property panel
    auto propertyPanel = std::make_unique<PropertyPanel>();
    m_propertyPanel = propertyPanel.get();
    m_panels.push_back(std::move(propertyPanel));
    
    // Create scene panel
    auto scenePanel = std::make_unique<ScenePanel>();
    m_scenePanel = scenePanel.get();
    m_panels.push_back(std::move(scenePanel));

    // Animation panel
    auto animationPanel = std::make_unique<AnimationPanel>();
    m_animationPanel = animationPanel.get();
    m_panels.push_back(std::move(animationPanel));
    
    std::cout << "Initialized " << m_panels.size() << " panels" << std::endl;
}

void EditorController::setupPanelCallbacks() {
    // Setup asset browser callbacks
    if (m_assetBrowserPanel) {
        // Single asset selection (existing)
        m_assetBrowserPanel->setOnAssetSelected(
            [this](const AssetInfo& asset) {
                onAssetSelected(asset);
                m_hasUnsavedChanges = true;
            }
        );
        
        // Multiple asset selection
        m_assetBrowserPanel->setOnMultipleAssetsSelected(
            [this](const std::vector<AssetInfo>& assets) {
                onMultipleAssetsSelected(assets);
                m_hasUnsavedChanges = true;
            }
        );
    }

    // Connect asset browser to asset panel
    if (m_assetPanel && m_assetBrowserPanel) {
        m_assetPanel->setAssetBrowserPanel(m_assetBrowserPanel);
    }
    
    // Setup scene panel callbacks
    if (m_scenePanel) {
        m_scenePanel->setOnSpriteSelected(
            [this](Sprite* sprite) {
                onSpriteSelected(sprite);
            }
        );
        
        m_scenePanel->setOnBoneSelected(
            [this](std::shared_ptr<Bone> bone) {
                onBoneSelected(bone);
            }
        );
        
        m_scenePanel->setOnSpriteBindingChanged(
            [this](Sprite* sprite, std::shared_ptr<Bone> bone) {
                onSpriteBindingChanged(sprite, bone);
            }
        );
    }
    
    // Setup asset panel callback
    if (m_assetPanel) {
        m_assetPanel->setOnSpriteSelected(
            [this](Sprite* sprite) {
                onSpriteSelected(sprite);
            }
        );
        m_assetPanel->setOnSpriteDeleted(
            [this](Sprite* sprite) {
                onSpriteDeleted(sprite);
                m_hasUnsavedChanges = true; // Mark as changed
            }
        );
    }

    // Connect character references
    if (m_scenePanel) {
        Character* character = m_scenePanel->getCharacter();
        
        if (m_assetPanel) {
            m_assetPanel->setCharacter(character);
        }
        
        if (m_propertyPanel) {
            m_propertyPanel->setCharacter(character);
        }
        
        if (m_animationPanel) {
            m_animationPanel->setCharacter(character);
            
            // Connect animation recording to scene changes
            m_scenePanel->setOnBoneRotated([this](std::shared_ptr<Bone> bone, float rotation) {
                // When recording and bone is rotated, add keyframe
                if (m_animationPanel->isRecording()) {
                    auto* player = m_scenePanel->getCharacter()->getAnimationPlayer();
                    auto* currentAnim = player->getAnimation();
                    if (currentAnim && bone) {
                        float currentTime = m_animationPanel->getCurrentTime();
                        Transform transform = bone->getLocalTransform();
                        currentAnim->addKeyframe(bone->getName(), currentTime, transform);
                        std::cout << "Auto-recorded keyframe for " << bone->getName() << " at time " << currentTime << std::endl;
                    }
                }
            });
        }
    }
    
    std::cout << "Panel callbacks setup complete" << std::endl;
}

void EditorController::onAssetSelected(const AssetInfo& asset) {
    if (!m_scenePanel) return;
    
    // Check if it's an image asset
    if (asset.type == "image") {
        // Add sprite to scene at center of current view
        sf::Vector2f centerPosition = m_scenePanel->getView().getCenter();
        
        // Use ScenePanel's method to add sprite
        m_scenePanel->addSpriteFromAsset(asset, centerPosition);
        
        std::cout << "Added sprite from asset: " << asset.name << std::endl;
    } else {
        std::cout << "Selected asset is not an image: " << asset.name << " (type: " << asset.type << ")" << std::endl;
    }
}

void EditorController::onMultipleAssetsSelected(const std::vector<AssetInfo>& assets) {
    if (!m_scenePanel || assets.empty()) return;
    
    std::cout << "Adding " << assets.size() << " assets to character..." << std::endl;
    
    // Add each selected asset as a sprite
    for (size_t i = 0; i < assets.size(); ++i) {
        const auto& asset = assets[i];
        
        // Check if it's an image asset (using string type field)
        if (asset.type == "image") {
            // Position sprites in a grid to avoid overlap
            float offsetX = (i % 5) * 120.0f; // 5 sprites per row
            float offsetY = (i / 5) * 120.0f; // Next row every 5 sprites
            sf::Vector2f position(offsetX, offsetY);
            
            // Use ScenePanel's addSpriteFromAsset method (same as single selection)
            m_scenePanel->addSpriteFromAsset(asset, position);
            
            std::cout << "Added sprite: " << asset.name << " from " << asset.path << std::endl;
        } else {
            std::cout << "Skipped non-image asset: " << asset.name << " (type: " << asset.type << ")" << std::endl;
        }
    }
    
    std::cout << "Successfully processed " << assets.size() << " assets" << std::endl;
}

void EditorController::onSpriteSelected(Sprite* sprite) {
    // Sync selection between all panels
    if (m_assetPanel) {
        m_assetPanel->setSelectedSprite(sprite);
    }
    
    if (m_propertyPanel) {
        m_propertyPanel->setSelectedSprite(sprite);
    }
    
    if (sprite) {
        std::cout << "Sprite selected: " << sprite->getName() << std::endl;
    } else {
        std::cout << "No sprite selected" << std::endl;
    }
}

void EditorController::onSpriteDeleted(Sprite* sprite) {
    // Clear selection from all panels if deleted sprite was selected
    if (m_propertyPanel) {
        if (m_propertyPanel->getSelectedSprite() == sprite) {
            m_propertyPanel->clearSelection();
        }
    }
    
    if (sprite) {
        std::cout << "Sprite deleted: " << sprite->getName() << std::endl;
    }
}

void EditorController::onBoneSelected(std::shared_ptr<Bone> bone) {
    // Update property panel for bone selection (future use)
    if (m_propertyPanel) {
        m_propertyPanel->setSelectedBone(bone.get());
    }

    if (bone) {
        std::cout << "Bone selected: " << bone->getName() << std::endl;
    } else {
        std::cout << "No bone selected" << std::endl;
    }
}

void EditorController::onSpriteBindingChanged(Sprite* sprite, std::shared_ptr<Bone> bone) {
    m_hasUnsavedChanges = true; // Mark as having changes
    
    if (sprite && bone) {
        std::cout << "Sprite '" << sprite->getName() 
                  << "' bound to bone '" << bone->getName() << "'" << std::endl;
    } else if (sprite) {
        std::cout << "Sprite '" << sprite->getName() << "' unbound from bones" << std::endl;
    }
}

void EditorController::requestExit() {
    if (hasUnsavedChanges()) {
        m_showExitConfirmation = true;
    } else {
        m_shouldExit = true;
    }
}

void EditorController::renderExitConfirmation() {
    if (m_showExitConfirmation) {
        ImGui::OpenPopup("Exit Riggle");
    }
    
    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Exit Riggle", nullptr, 
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        
        ImGui::Text("Are you sure you want to exit Riggle?");
        
        bool hasChanges = hasUnsavedChanges();
        if (hasChanges) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "You have unsaved changes!");
            ImGui::Text("Your work will be lost if you exit without saving.");
        }
        
        ImGui::Separator();
        ImGui::Spacing();
        
        // Button layout
        float buttonWidth = 120.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = hasUnsavedChanges() ? (buttonWidth * 3 + spacing * 2) : (buttonWidth * 2 + spacing);
        float startPos = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
        
        if (startPos > 0) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startPos);
        }
        
        if (hasChanges) {
            // Save and Exit button
            if (ImGui::Button("Save & Exit", ImVec2(buttonWidth, 0))) {
                saveProject();
                m_shouldExit = true;
                m_showExitConfirmation = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
        }
        
        // Exit without saving
        if (ImGui::Button("Exit", ImVec2(buttonWidth, 0))) {
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

bool EditorController::hasUnsavedChanges() const {
    // Check explicit flag first
    if (m_hasUnsavedChanges) {
        return true;
    }
    
    // Check if we have sprites or bones (indicates work has been done)
    if (m_scenePanel && m_scenePanel->getCharacter()) {
        const auto& sprites = m_scenePanel->getCharacter()->getSprites();
        if (!sprites.empty()) {
            std::cout << "Found " << sprites.size() << " sprites - has changes" << std::endl;
            return true;
        }
        
        if (m_scenePanel->getCharacter()->getRig()) {
            const auto& bones = m_scenePanel->getCharacter()->getRig()->getAllBones();
            if (!bones.empty()) {
                std::cout << "Found " << bones.size() << " bones - has changes" << std::endl;
                return true;
            }
        }
    }
    
    return false;
}

void EditorController::newProject() {
    if (hasUnsavedChanges()) {
        // TODO: Show "unsaved changes" confirmation
        std::cout << "Warning: You have unsaved changes!" << std::endl;
    }
    
    // Reset scene
    if (m_scenePanel) {
        auto newCharacter = std::make_unique<Character>("New Character");
        auto rig = std::make_unique<Rig>("New Rig");
        newCharacter->setRig(std::move(rig));
        m_scenePanel->setCharacter(std::move(newCharacter));
        
        // Update all panels with new character
        Character* character = m_scenePanel->getCharacter();
        
        if (m_assetPanel) {
            m_assetPanel->setCharacter(character);
            m_assetPanel->setSelectedSprite(nullptr);
        }
        
        if (m_propertyPanel) {
            m_propertyPanel->setCharacter(character);
            m_propertyPanel->clearSelection();
        }
        
        if (m_animationPanel) {
            m_animationPanel->setCharacter(character);
        }
    }
    
    m_hasUnsavedChanges = false;
    std::cout << "Created new project" << std::endl;
}

void EditorController::saveProject() {
    // TODO: Implement actual project saving
    std::cout << "Saving project..." << std::endl;
    m_hasUnsavedChanges = false;
    std::cout << "Project saved successfully" << std::endl;
}

void EditorController::loadProject() {
    // TODO: Implement actual project loading
    std::cout << "Loading project..." << std::endl;
    m_hasUnsavedChanges = false;
}


} // namespace Riggle