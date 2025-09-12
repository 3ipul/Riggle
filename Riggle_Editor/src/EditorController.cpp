#include "Editor/EditorController.h"
#include "Editor/Export/JSONExporter.h"
#include "Editor/Export/PNGExporter.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <iostream>

namespace Riggle {

EditorController::EditorController() 
    : m_viewportPanel(nullptr)
    , m_assetPanel(nullptr)
    , m_assetBrowserPanel(nullptr)
    , m_hierarchyPanel(nullptr)
    , m_propertyPanel(nullptr)
    , m_currentWindow(nullptr)
    , m_shouldExit(false)
    , m_hasUnsavedChanges(false)
    , m_resetLayoutRequested(false)
{
    initializePanels();
    initializeExportSystem();
    initializeProjectSystem();
    setupDialogCallbacks();
}

void EditorController::update(sf::RenderWindow& window) {
    m_currentWindow = &window;
    
    // Update character animation
    if (m_character) {
        m_character->update(ImGui::GetIO().DeltaTime);
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
    
    // Render individual panels
    if (m_viewportPanel) {
        m_viewportPanel->render();
    }

    if(m_assetPanel) {
        m_assetPanel->render();
    }
    
    if (m_assetBrowserPanel) {
        m_assetBrowserPanel->render();
    }

    if (m_hierarchyPanel) {
        m_hierarchyPanel->render();
    }
    
    if (m_propertyPanel) {
        m_propertyPanel->render();
    }

    if (m_animationPanel) {
        m_animationPanel->render();
    }

    // Render dialogs
    if (m_dialogManager) {
        m_dialogManager->render();
    }
}

void EditorController::handleEvent(const sf::Event& event) {
    // Block global shortcuts if ImGui wants keyboard
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }

    // Handle tool switching shortcuts
    if (event.is<sf::Event::KeyPressed>()) {
        const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
        if (keyPressed) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                requestExit();
                return; // Don't pass ESC to panels
            }
            // Tool shortcuts
            else if (keyPressed->code == sf::Keyboard::Key::Num1) {
                if (m_viewportPanel) m_viewportPanel->setTool(ViewportTool::SpriteTool);
                return;
            }
            else if (keyPressed->code == sf::Keyboard::Key::Num2) {
                if (m_viewportPanel) m_viewportPanel->setTool(ViewportTool::BoneTool);
                return;
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

void EditorController::initializePanels() {
    // Create viewport panel
    auto viewportPanel = std::make_unique<ViewportPanel>();
    m_viewportPanel = viewportPanel.get();
    m_panels.push_back(std::move(viewportPanel));
    
    // Create asset browser panel
    auto assetBrowserPanel = std::make_unique<AssetBrowserPanel>();
    m_assetBrowserPanel = assetBrowserPanel.get();
    m_panels.push_back(std::move(assetBrowserPanel));
    
    // Create asset panel
    auto assetPanel = std::make_unique<AssetPanel>();
    m_assetPanel = assetPanel.get();
    m_panels.push_back(std::move(assetPanel));

    // Create hierarchy panel
    auto hierarchyPanel = std::make_unique<HierarchyPanel>();
    m_hierarchyPanel = hierarchyPanel.get();
    m_panels.push_back(std::move(hierarchyPanel));
    
    // Create property panel
    auto propertyPanel = std::make_unique<PropertyPanel>();
    m_propertyPanel = propertyPanel.get();
    m_panels.push_back(std::move(propertyPanel));

    // Animation panel
    auto animationPanel = std::make_unique<AnimationPanel>();
    m_animationPanel = animationPanel.get();
    m_panels.push_back(std::move(animationPanel));

    createDefaultCharacter();
    
    // Connect asset panels
    m_assetPanel->setAssetBrowserPanel(m_assetBrowserPanel);
    
    std::cout << "Initialized " << m_panels.size() << " panel(s)" << std::endl;
}

void EditorController::setupPanelCallbacks() {
    // Asset Browser -> Add sprites
    m_assetBrowserPanel->setOnAssetSelected([this](const AssetInfo& asset) {
        onAssetSelected(asset);
    });
    
    m_assetBrowserPanel->setOnMultipleAssetsSelected([this](const std::vector<AssetInfo>& assets) {
        onMultipleAssetsSelected(assets);
    });
    
    // Asset Panel -> Property Panel (sprite selection)
    m_assetPanel->setOnSpriteSelected([this](Sprite* sprite) {
       // Check if we have a bone selected for approach 2
        if (m_selectedBone && sprite) {
            // Approach 2: Multi-selection for binding
            updateSelectionState(sprite, m_selectedBone);
            std::cout << "Multi-selection: Sprite '" << sprite->getName() 
                      << "' + Bone '" << m_selectedBone->getName() << "'" << std::endl;
        } else {
            // Normal sprite selection
            updateSelectionState(sprite, nullptr);
        }
    });
    
    m_assetPanel->setOnSpriteDeleted([this](Sprite* sprite) {
        onSpriteDeleted(sprite);
    });

    // Hierarchy Panel -> Property Panel (bone selection)
    m_hierarchyPanel->setOnBoneSelected([this](std::shared_ptr<Bone> bone) {
        // Check if we have a sprite selected for approach 2  
        if (m_selectedSprite && bone) {
            // Approach 2: Multi-selection for binding
            updateSelectionState(m_selectedSprite, bone);
            std::cout << "Multi-selection: Bone '" << bone->getName() 
                      << "' + Sprite '" << m_selectedSprite->getName() << "'" << std::endl;
        } else {
            // Normal bone selection
            updateSelectionState(nullptr, bone);
        }
    });
    
    m_hierarchyPanel->setOnBoneDeleted([this](std::shared_ptr<Bone> bone) {
        onBoneDeleted(bone);
    });

    m_hierarchyPanel->setOnBoneRenamed([this](std::shared_ptr<Bone> bone, const std::string& oldName) {
        std::cout << "Bone renamed from '" << oldName << "' to '" << bone->getName() << "'" << std::endl;
        m_hasUnsavedChanges = true;
    });
    
    // Viewport Panel -> Property Panel (sprite/bone selection)
   // Viewport should clear multi-selection when selecting individually
    m_viewportPanel->setOnSpriteSelected([this](Sprite* sprite) {
        updateSelectionState(sprite, m_selectedBone);
    });
    
    m_viewportPanel->setOnBoneSelected([this](std::shared_ptr<Bone> bone) {
        updateSelectionState(nullptr, bone);
    });

    m_viewportPanel->setOnBoneCreated([this](std::shared_ptr<Bone> bone) {
        if (!bone) return;
        
        std::cout << "Created bone: " << bone->getName() << std::endl;
        
        // APPROACH 1: Ctrl+hover auto-binding
        // Check if we're in Ctrl+hover mode and have a selected sprite
        if (m_viewportPanel->isInCtrlHoverMode() && m_selectedSprite && bone->getSpriteCount() == 0) {
            std::cout << "Approach 1: Ctrl+hover auto-binding..." << std::endl;
            
            try {
                // Store original sprite position
                Transform originalSpriteWorld = m_selectedSprite->getWorldTransform();
                Vector2 originalSpritePos = originalSpriteWorld.position;
                float originalSpriteRot = originalSpriteWorld.rotation;
                
                // Get bone world transform
                Transform boneWorld = bone->getWorldTransform();
                Vector2 bonePos = boneWorld.position;
                
                // Calculate binding offset
                Vector2 bindOffset;
                bindOffset.x = originalSpritePos.x - bonePos.x;
                bindOffset.y = originalSpritePos.y - bonePos.y;
                float bindRotation = originalSpriteRot - boneWorld.rotation;
                
                // APPROACH 1: Set sprite-based bone name
                std::string newBoneName = m_selectedSprite->getName();
                size_t dotPos = newBoneName.find_last_of('.');
                if (dotPos != std::string::npos) {
                    newBoneName = newBoneName.substr(0, dotPos);
                }
                newBoneName += "_bone";
                bone->setName(newBoneName);
                
                // Bind sprite to bone
                m_selectedSprite->bindToBone(bone, bindOffset, bindRotation);
                
                std::cout << "Approach 1: Auto-bound sprite '" << m_selectedSprite->getName() 
                          << "' to bone '" << bone->getName() << "'" << std::endl;
                
            } catch (const std::exception& e) {
                std::cout << "Error during approach 1 binding: " << e.what() << std::endl;
            }
        }
        // APPROACH 2: Selected sprite from asset panel (normal mode, not Ctrl+hover)
        else if (!m_viewportPanel->isInCtrlHoverMode() && m_selectedSprite && bone->getSpriteCount() == 0) {
            std::cout << "Approach 2: Asset panel selection binding..." << std::endl;
            
            try {
                // Store original sprite position
                Transform originalSpriteWorld = m_selectedSprite->getWorldTransform();
                Vector2 originalSpritePos = originalSpriteWorld.position;
                float originalSpriteRot = originalSpriteWorld.rotation;
                
                // Get bone world transform
                Transform boneWorld = bone->getWorldTransform();
                Vector2 bonePos = boneWorld.position;
                
                // Calculate binding offset
                Vector2 bindOffset;
                bindOffset.x = originalSpritePos.x - bonePos.x;
                bindOffset.y = originalSpritePos.y - bonePos.y;
                float bindRotation = originalSpriteRot - boneWorld.rotation;
                
                // APPROACH 2: Set sprite-based bone name
                std::string newBoneName = m_selectedSprite->getName();
                size_t dotPos = newBoneName.find_last_of('.');
                if (dotPos != std::string::npos) {
                    newBoneName = newBoneName.substr(0, dotPos);
                }
                newBoneName += "_bone";
                bone->setName(newBoneName);
                
                // Bind sprite to bone
                m_selectedSprite->bindToBone(bone, bindOffset, bindRotation);
                
                std::cout << "Approach 2: Bound selected sprite '" << m_selectedSprite->getName() 
                          << "' to new bone '" << bone->getName() << "'" << std::endl;
                
            } catch (const std::exception& e) {
                std::cout << "Error during approach 2 binding: " << e.what() << std::endl;
            }
        }
        // APPROACH 3: Manual binding (no auto-binding, keep original bone name)
        else {
            std::cout << "Approach 3: Manual binding (bone name unchanged)" << std::endl;
        }
        
        // Update selection state
        if (m_selectedSprite && bone) {
            updateSelectionState(m_selectedSprite, bone);
        } else {
            updateSelectionState(nullptr, bone);
        }
        
        m_hasUnsavedChanges = true;
    });

    // Connect bone rotation to animation recording
    m_viewportPanel->setOnBoneRotated([this](std::shared_ptr<Bone> bone, float rotation) {
        // Record keyframe when bone is rotated during recording
        if (m_animationPanel && m_animationPanel->isRecording()) {
            auto* player = m_character->getAnimationPlayer();
            auto* currentAnim = player->getAnimation();
            if (currentAnim && bone) {
                float currentTime = m_animationPanel->getCurrentTime();
                Transform transform = bone->getLocalTransform();
                currentAnim->addKeyframe(bone->getName(), currentTime, transform);
                std::cout << "Recorded keyframe for " << bone->getName() << " at time " << currentTime << std::endl;
            }
        }
        m_hasUnsavedChanges = true;
    });

    // Setup transform event handler for auto-keyframing
    if (m_character) {
        m_character->addTransformEventHandler([this](const Character::TransformEvent& event) {
            // Check if we should auto-keyframe
            if (m_animationPanel && m_animationPanel->isRecording() && m_animationPanel->isAutoKeyEnabled()) {
                
                m_animationPanel->createKeyframeForBone(event.boneName);
                std::cout << "Auto-keyed bone: " << event.boneName << " at time " << event.timestamp << std::endl;
            }
        });
    }

    if (m_propertyPanel && m_viewportPanel) {
        m_propertyPanel->setIKTool(m_viewportPanel->getIKTool());
    }

    std::cout << "Panel callbacks setup complete" << std::endl;

}

void EditorController::onAssetSelected(const AssetInfo& asset) {
    if (!m_character) return;
    
    // Create sprite from asset
    auto sprite = std::make_unique<Sprite>(asset.name, asset.path);
    
    // Set initial transform
    Transform initialTransform;
    initialTransform.position = {0.0f, 0.0f};
    initialTransform.rotation = 0.0f;
    initialTransform.scale = {1.0f, 1.0f};
    sprite->setTransform(initialTransform);
    
    std::cout << "Created sprite: " << asset.name << " from " << asset.path << std::endl;
    
    // Add to character
    Sprite* spritePtr = sprite.get();
    m_character->addSprite(std::move(sprite));
    
    // Select the new sprite
    if (m_assetPanel) {
        m_assetPanel->setSelectedSprite(spritePtr);
    }
    if (m_propertyPanel) {
        m_propertyPanel->setSelectedSprite(spritePtr);
    }
    
    m_hasUnsavedChanges = true;
}

void EditorController::onMultipleAssetsSelected(const std::vector<AssetInfo>& assets) {
    if (!m_character) return;
    
    // Place all sprites at the SAME position (0,0) like single insert
    for (const auto& asset : assets) {
        // Create sprite from asset
        auto sprite = std::make_unique<Sprite>(asset.name, asset.path);
        
        // All sprites get the SAME initial transform
        Transform initialTransform;
        initialTransform.position = {0.0f, 0.0f}; // Same position for all
        initialTransform.rotation = 0.0f;
        initialTransform.scale = {1.0f, 1.0f};
        sprite->setTransform(initialTransform);
        
        std::cout << "Created sprite: " << asset.name << " at (0, 0)" << std::endl;
        
        // Add to character
        m_character->addSprite(std::move(sprite));
    }
    
    std::cout << "Added " << assets.size() << " sprites to character (all at same position)" << std::endl;
    m_hasUnsavedChanges = true;
}

void EditorController::onSpriteSelected(Sprite* sprite) {
    // Update property panel
    if (m_propertyPanel) {
        m_propertyPanel->setSelectedSprite(sprite);
    }
    
    // Update asset panel selection to match
    if (m_assetPanel && m_assetPanel->getSelectedSprite() != sprite) {
        m_assetPanel->setSelectedSprite(sprite);
    }
    
    // Update viewport selection to match  
    if (m_viewportPanel && m_viewportPanel->getSelectedSprite() != sprite) {
        m_viewportPanel->setSelectedSprite(sprite);
    }
    
    if (sprite) {
        std::cout << "Selected sprite: " << sprite->getName() << std::endl;
    } else {
        std::cout << "Cleared sprite selection" << std::endl;
    }
}

void EditorController::onSpriteDeleted(Sprite* sprite) {
    // Clear from property panel if it was selected
    if (m_propertyPanel && m_propertyPanel->getSelectedSprite() == sprite) {
        m_propertyPanel->clearSelection();
    }
    
    // Clear from viewport if it was selected
    if (m_viewportPanel && m_viewportPanel->getSelectedSprite() == sprite) {
        m_viewportPanel->setSelectedSprite(nullptr);
    }
    
    m_hasUnsavedChanges = true;
}

void EditorController::onBoneSelected(std::shared_ptr<Bone> bone) {
    // Update property panel
    if (m_propertyPanel) {
        m_propertyPanel->setSelectedBone(bone);
    }
    
    if (bone) {
        std::cout << "Selected bone: " << bone->getName() << std::endl;
    } else {
        std::cout << "Cleared bone selection" << std::endl;
    }
}

void EditorController::onBoneCreated(std::shared_ptr<Bone> bone) {
    if (bone) {
        std::cout << "Created bone: " << bone->getName() << std::endl;
        
        // Auto-select the new bone
        onBoneSelected(bone);
        m_hasUnsavedChanges = true;
    }
}

void EditorController::updateSelectionState(Sprite* sprite, std::shared_ptr<Bone> bone) {
    m_selectedSprite = sprite;
    m_selectedBone = bone;
    m_hasMultiSelection = (sprite && bone);
    
    // Update property panel based on selection state
    if (m_propertyPanel) {
        if (m_hasMultiSelection) {
            m_propertyPanel->setMultiSelection(sprite, bone);
        } else if (sprite) {
            m_propertyPanel->setSelectedSprite(sprite);
        } else if (bone) {
            m_propertyPanel->setSelectedBone(bone);
        } else {
            m_propertyPanel->clearSelection();
        }
    }
    
    // Sync other panels
    if (m_assetPanel && m_assetPanel->getSelectedSprite() != sprite) {
        m_assetPanel->setSelectedSprite(sprite);
    }
    
    if (m_hierarchyPanel && m_hierarchyPanel->getSelectedBone() != bone) {
        m_hierarchyPanel->setSelectedBone(bone);
    }
    
    if (m_viewportPanel) {
        if (m_viewportPanel->getSelectedSprite() != sprite) {
            m_viewportPanel->setSelectedSprite(sprite);
        }
        if (m_viewportPanel->getSelectedBone() != bone) {
            m_viewportPanel->setSelectedBone(bone);
        }
    }
}

void EditorController::clearAllSelections() {
    updateSelectionState(nullptr, nullptr);
}

void EditorController::createDefaultCharacter() {
    // Create default character with rig
    m_character = std::make_unique<Character>("Default Character");
    auto rig = std::make_unique<Rig>("Default Rig");
    m_character->setRig(std::move(rig));

    // Update all panels with new character
    if (m_viewportPanel) m_viewportPanel->setCharacter(m_character.get());
    if (m_assetPanel) m_assetPanel->setCharacter(m_character.get());
    if (m_hierarchyPanel) m_hierarchyPanel->setCharacter(m_character.get());
    if (m_propertyPanel) m_propertyPanel->setCharacter(m_character.get());
    if (m_animationPanel) m_animationPanel->setCharacter(m_character.get());

    setupPanelCallbacks();
    
    std::cout << "Created default character with rig" << std::endl;
}

void EditorController::renderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {
                if (hasUnsavedChanges()) {
                    m_dialogManager->showNewProjectConfirmation(true);
                } else {
                    newProject();
                }
            }
            if (ImGui::MenuItem("Save Project")) {
                saveProject();
            }
            if (ImGui::MenuItem("Load Project")) {
                loadProject();
            }
            if (ImGui::MenuItem("Project Settings...")) {
                //m_showProjectSettingsDialog = true;
                m_dialogManager->showProjectSettingsDialog(m_currentProjectMetadata);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export...")) {
                //m_showExportDialog = true;
                m_dialogManager->showExportDialog(m_character.get());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                requestExit();
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
                m_dialogManager->showLayoutResetConfirmation();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset View")) {
                if (m_viewportPanel) {
                    m_viewportPanel->resetView();
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (m_viewportPanel) {
                
                // Tool selection
                ViewportTool currentTool = m_viewportPanel->getCurrentTool();
                
                if (ImGui::MenuItem("Sprite Tool", nullptr, currentTool == ViewportTool::SpriteTool)) {
                    m_viewportPanel->setTool(ViewportTool::SpriteTool);
                }
                if (ImGui::MenuItem("Bone Tool", nullptr, currentTool == ViewportTool::BoneTool)) {
                    m_viewportPanel->setTool(ViewportTool::BoneTool);
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Controls")) {
                //m_showControlsDialog = true;
                m_dialogManager->showControlsDialog();
            }
            if (ImGui::MenuItem("About Riggle")) {
                //m_showAboutDialog = true;
                m_dialogManager->showAboutDialog();
            }
            ImGui::EndMenu();
        }
        
        // Status information in the menu bar
        ImGui::Separator();
        if (m_character) {
            size_t spriteCount = m_character->getSprites().size();
            size_t boneCount = 0;
            if (m_character->getRig()) {
                boneCount = m_character->getRig()->getAllBones().size();
            }
            ImGui::Text("Sprites: %zu | Bones: %zu", spriteCount, boneCount);
        }
        
        ImGui::EndMainMenuBar();
    }
    // Handle new project confirmation after rendering dialogs
    if (m_dialogManager && m_dialogManager->isNewProjectConfirmed()) {
        newProject();
        m_dialogManager->clearNewProjectConfirmedFlag();
    }
}

void EditorController::onBoneDeleted(std::shared_ptr<Bone> bone) {
    if (!bone || !m_character || !m_character->getRig()) return;
    
    std::string boneName = bone->getName();
    std::cout << "Deleting bone: " << boneName << std::endl;
    
    // Clear selections if deleting selected bone
    if (m_hierarchyPanel && bone == m_hierarchyPanel->getSelectedBone()) {
        m_hierarchyPanel->setSelectedBone(nullptr);
        if (m_propertyPanel) {
            m_propertyPanel->setSelectedBone(nullptr);
        }
    }
    
    if (m_viewportPanel && bone == m_viewportPanel->getSelectedBone()) {
        m_viewportPanel->setSelectedBone(nullptr);
    }
    
    // Remove from rig
    m_character->getRig()->removeBone(boneName);
    
    m_hasUnsavedChanges = true;
}

void EditorController::requestExit() {
    if (m_dialogManager) {
        m_dialogManager->showExitConfirmation(hasUnsavedChanges());
    }
}

bool EditorController::shouldExit() const { 
    if (m_dialogManager && m_dialogManager->shouldExit()) {
        return true;
    }
    return m_shouldExit; 
}

bool EditorController::isLayoutResetRequested() const { 
    if (m_dialogManager && m_dialogManager->isLayoutResetRequested()) {
        return true;
    }
    return m_resetLayoutRequested; 
}

bool EditorController::hasUnsavedChanges() const {
    // Check explicit flag first
    if (m_hasUnsavedChanges) {
        return true;
    }
    
    // Check if we have sprites or bones (indicates work has been done)
    if (m_character) {
        const auto& sprites = m_character->getSprites();
        if (!sprites.empty()) {
            return true;
        }
        
        if (m_character->getRig()) {
            const auto& bones = m_character->getRig()->getAllBones();
            if (!bones.empty()) {
                return true;
            }
        }
    }
    
    return false;
}

void EditorController::newProject() {
    if (hasUnsavedChanges()) {
        std::cout << "Warning: You have unsaved changes!" << std::endl;
    }

    // Update dialog manager with current project metadata
    if (m_dialogManager) {
        m_dialogManager->updateProjectMetadata(m_currentProjectMetadata);
    }
    
    // Create new character
    createDefaultCharacter();
    
    // Update all panels with new character
    if (m_viewportPanel) {
        m_viewportPanel->setCharacter(m_character.get());
    }
    if (m_assetPanel) {
        m_assetPanel->setCharacter(m_character.get());
        m_assetPanel->clearSelection();
    }
    if (m_propertyPanel) {
        m_propertyPanel->setCharacter(m_character.get());
        m_propertyPanel->clearSelection();
    }
    if (m_animationPanel) {
        m_animationPanel->setCharacter(m_character.get());
    }
    
    m_hasUnsavedChanges = false;
    std::cout << "Created new project" << std::endl;
}

void EditorController::initializeProjectSystem() {
    m_projectManager = std::make_unique<ProjectManager>();
    
    // Set default project metadata
    m_currentProjectMetadata.name = "Untitled Project";
    m_currentProjectMetadata.version = "1.0";
    m_currentProjectMetadata.author = "Riggle User";
    m_currentProjectMetadata.description = "Created with Riggle Animation Tool";
    
    std::cout << "Project system initialized" << std::endl;
}

void EditorController::saveProject() {
    if (!m_character || !m_projectManager) {
        std::cout << "No character or project manager available" << std::endl;
        return;
    }
    
    std::string filePath;
    std::vector<FileFilter> filters = {
        FileFilter("Riggle Project Files", ProjectManager::getProjectExtension()),
        FileFilter("All Files", ".*")
    };
    
    // Use current project path as default if available
    std::string defaultName = m_currentProjectMetadata.name + ProjectManager::getProjectExtension();
    
    if (FileDialogManager::getInstance().saveFileDialog(filePath, filters, "", defaultName)) {
        if (m_projectManager->saveProject(*m_character, filePath, m_currentProjectMetadata)) {
            m_currentProjectPath = filePath;
            m_hasUnsavedChanges = false;
            std::cout << "Project saved successfully to: " << filePath << std::endl;
        } else {
            std::cout << "Failed to save project: " << m_projectManager->getLastError() << std::endl;
        }
    }
}

void EditorController::loadProject() {
    std::string filePath;
    std::vector<FileFilter> filters = {
        FileFilter("Riggle Project Files", ProjectManager::getProjectExtension()),
        FileFilter("All Files", ".*")
    };
    
    if (FileDialogManager::getInstance().openFileDialog(filePath, filters)) {
        std::unique_ptr<Character> loadedCharacter;
        ProjectMetadata metadata;
        
        if (m_projectManager->loadProject(loadedCharacter, filePath, metadata)) {
            if (metadata.name.empty() || metadata.name == "Untitled Project") {
                std::filesystem::path path(filePath);
                metadata.name = path.stem().string();
            }

            // Replace current character
            m_character = std::move(loadedCharacter);
            m_currentProjectPath = filePath;
            m_currentProjectMetadata = metadata;

            // Update dialog manager with loaded project metadata
            if (m_dialogManager) {
                m_dialogManager->updateProjectMetadata(metadata);
            }
            
            // Update all panels with loaded character
            updateAllPanelsWithCharacter();
            
            m_hasUnsavedChanges = false;
            std::cout << "Project loaded successfully from: " << filePath << std::endl;
        } else {
            std::cout << "Failed to load project: " << m_projectManager->getLastError() << std::endl;
        }
    }
}

void EditorController::updateAllPanelsWithCharacter() {
    if (m_viewportPanel) m_viewportPanel->setCharacter(m_character.get());
    if (m_assetPanel) m_assetPanel->setCharacter(m_character.get());
    if (m_hierarchyPanel) m_hierarchyPanel->setCharacter(m_character.get());
    if (m_propertyPanel) m_propertyPanel->setCharacter(m_character.get());
    if (m_animationPanel) m_animationPanel->setCharacter(m_character.get());
    
    // Clear selections
    clearAllSelections();
    
    // Re-setup callbacks
    setupPanelCallbacks();
}

void EditorController::setupDialogCallbacks() {
    if (!m_dialogManager) return;
    
    m_dialogManager->setOnSaveAndExit([this]() {
        saveProject();
    });
    
    m_dialogManager->setOnExit([this]() {
        // Just set the flag, actual exit is handled by shouldExit()
    });
    
    m_dialogManager->setOnLayoutReset([this]() {
        for (auto& panel : m_panels) {
            if (panel) {
                panel->setVisible(true);
            }
        }
        m_resetLayoutRequested = true;
    });
    
    m_dialogManager->setOnProjectSettingsSave([this](const ProjectMetadata& metadata) {
        m_currentProjectMetadata = metadata;
        m_hasUnsavedChanges = true;

        // Keep dialog manager in sync
        if (m_dialogManager) {
            m_dialogManager->updateProjectMetadata(metadata);
        }
    });
}

void EditorController::initializeExportSystem() {
    m_exportManager = std::make_unique<ExportManager>();
    m_dialogManager = std::make_unique<DialogManager>();
    
    // Register built-in exporters
    m_exportManager->registerProjectExporter(std::make_unique<JSONProjectExporter>());
    m_exportManager->registerAnimationExporter(std::make_unique<PNGSequenceExporter>());
    
    // Initialize dialog manager with dependencies
    m_dialogManager->initialize(m_exportManager.get(), m_projectManager.get());
    
    std::cout << "Export system initialized" << std::endl;
}

void EditorController::setupInitialDockLayout(ImGuiID dockspace_id) {
    ImGui::DockBuilderRemoveNode(dockspace_id); // Clear previous layout
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    // Base dock id to split from
    ImGuiID dock_main_id = dockspace_id;

    // Split into left, right, and bottom
    ImGuiID dock_id_left   = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
    ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.28f, nullptr, &dock_main_id);
    ImGuiID dock_id_right  = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.28f, nullptr, &dock_main_id);

    // Split left into top and bottom
    ImGuiID dock_id_left_top = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.60f, nullptr, &dock_id_left);

    // Dock windows
    ImGui::DockBuilderDockWindow("Assets", dock_id_left_top);
    ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left_top);

    ImGui::DockBuilderDockWindow("Asset Browser", dock_id_left);       

    ImGui::DockBuilderDockWindow("Viewport", dock_main_id);            
    ImGui::DockBuilderDockWindow("Properties", dock_id_right);         
    ImGui::DockBuilderDockWindow("Animation", dock_id_bottom);

    ImGui::DockBuilderFinish(dockspace_id);
}

} // namespace Riggle