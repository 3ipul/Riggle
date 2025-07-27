#include "Editor/Panels/SpriteInspectorPanel.h"
#include <imgui.h>
#include <algorithm>
#include <iostream>

namespace Riggle {

SpriteInspectorPanel::SpriteInspectorPanel()
    : BasePanel("Sprite Inspector")
    , m_character(nullptr)
    , m_selectedSprite(nullptr)
    , m_moveStep(10.0f)
    , m_moveStepInt(10)
{
}

void SpriteInspectorPanel::render() {
    if (!m_isVisible) return;

    ImGui::Begin(m_name.c_str(), &m_isVisible);

    if (!m_character) {
        ImGui::Text("No character loaded");
        ImGui::End();
        return;
    }

    // Sprite list section
    renderSpriteList();
    
    ImGui::Separator();
    
    if (m_selectedSprite) {
        // Transform controls section
        renderTransformControls();
        
        ImGui::Separator();
        
        // Movement buttons section
        renderMovementButtons();
        
        ImGui::Separator();
        
        // Additional sprite properties
        renderSpriteProperties();
    } else {
        ImGui::Text("No sprite selected");
        ImGui::Text("Select a sprite from the list above or click in the scene");
    }

    ImGui::End();
}

void SpriteInspectorPanel::update(sf::RenderWindow& window) {
    // Handle continuous keyboard input for smooth movement
    if (m_selectedSprite) {
        bool moved = false;
        float step = m_moveStep;
        
        // Check for shift key for faster movement
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)) {
            step *= 5.0f; // 5x faster when holding shift
        }
        
        // Check movement keys for continuous movement
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
            moveSprite(-step * 0.1f, 0); // Smaller steps for smooth movement
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            moveSprite(step * 0.1f, 0);
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
            moveSprite(0, -step * 0.1f);
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
            moveSprite(0, step * 0.1f);
            moved = true;
        }
    }
}

void SpriteInspectorPanel::handleEvent(const sf::Event& event) {
    // Use keyboard polling in update() method instead
}

void SpriteInspectorPanel::renderSpriteList() {
    ImGui::Text("Sprites in Scene (%zu)", m_character->getSprites().size());
    
    // Create a list box for sprite selection
    if (ImGui::BeginListBox("##SpriteList", ImVec2(-1, 150))) {
        
        const auto& sprites = m_character->getSprites();
        for (size_t i = 0; i < sprites.size(); ++i) {
            const auto& sprite = sprites[i];
            bool isSelected = (sprite.get() == m_selectedSprite);
            
            // Create a selectable item for each sprite
            std::string displayName = sprite->getName();
            if (displayName.empty()) {
                displayName = "Sprite " + std::to_string(i);
            }
            
            // Add position info to the display name
            Transform transform = sprite->getLocalTransform();
            displayName += " (" + std::to_string((int)transform.x) + ", " + std::to_string((int)transform.y) + ")";
            
            // FIXED: Add binding indicator to display name (NO LEGACY)
            if (sprite->isBoundToBones()) {
                const auto& bindings = sprite->getBoneBindings();
                displayName += " [" + std::to_string(bindings.size()) + " bones]";
            }
            
            if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                m_selectedSprite = sprite.get();
                if (m_onSpriteSelected) {
                    m_onSpriteSelected(m_selectedSprite);
                }
                std::cout << "Selected sprite from list: " << sprite->getName() << std::endl;
            }
            
            // Enhanced tooltip with binding info
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Name: %s", sprite->getName().c_str());
                ImGui::Text("Path: %s", sprite->getTexturePath().c_str());
                ImGui::Text("Position: (%.1f, %.1f)", transform.x, transform.y);
                ImGui::Text("Rotation: %.1f°", transform.rotation * 180.0f / 3.14159f);
                ImGui::Text("Scale: (%.2f, %.2f)", transform.scaleX, transform.scaleY);
                
                // FIXED: Show binding information in tooltip (NO LEGACY)
                if (sprite->isBoundToBones()) {
                    const auto& bindings = sprite->getBoneBindings();
                    ImGui::Separator();
                    ImGui::Text("Bound to %zu bones:", bindings.size());
                    for (const auto& binding : bindings) {
                        ImGui::Text("- %s (%.2f)", 
                                   binding.bone->getName().c_str(), 
                                   binding.weight);
                    }
                } else {
                    ImGui::Separator();
                    ImGui::Text("No bone bindings");
                }
                
                ImGui::EndTooltip();
            }
        }
        
        ImGui::EndListBox();
    }
}

void SpriteInspectorPanel::renderTransformControls() {
    ImGui::Text("Transform Controls");
    
    if (!m_selectedSprite) return;
    
    Transform transform = m_selectedSprite->getLocalTransform();
    bool changed = false;
    
    // Position controls
    if (ImGui::DragFloat2("Position", &transform.x, 1.0f, -10000.0f, 10000.0f, "%.1f")) {
        changed = true;
    }
    
    // Rotation control (convert to degrees for UI)
    float rotationDegrees = transform.rotation * 180.0f / 3.14159f;
    if (ImGui::DragFloat("Rotation", &rotationDegrees, 1.0f, -360.0f, 360.0f, "%.1f°")) {
        transform.rotation = rotationDegrees * 3.14159f / 180.0f;
        changed = true;
    }
    
    // Scale controls
    if (ImGui::DragFloat2("Scale", &transform.scaleX, 0.01f, 0.1f, 10.0f, "%.2f")) {
        changed = true;
    }
    
    // Apply changes
    if (changed) {
        m_selectedSprite->setTransform(transform);
    }
    
    // Reset buttons row
    if (ImGui::Button("Reset Position")) {
        Transform resetTransform = m_selectedSprite->getLocalTransform();
        resetTransform.x = 0;
        resetTransform.y = 0;
        m_selectedSprite->setTransform(resetTransform);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Rotation")) {
        Transform resetTransform = m_selectedSprite->getLocalTransform();
        resetTransform.rotation = 0;
        m_selectedSprite->setTransform(resetTransform);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Scale")) {
        Transform resetTransform = m_selectedSprite->getLocalTransform();
        resetTransform.scaleX = 1.0f;
        resetTransform.scaleY = 1.0f;
        m_selectedSprite->setTransform(resetTransform);
    }
}

void SpriteInspectorPanel::renderMovementButtons() {
    ImGui::Text("Precise Movement");
    
    // Movement step control
    ImGui::Text("Move Step:");
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderInt("##MoveStep", &m_moveStepInt, 1, 100, "%d px")) {
        m_moveStep = static_cast<float>(m_moveStepInt);
    }
    
    ImGui::SameLine();
    ImGui::Text("Hold Shift for 5x speed");
    
    // Movement buttons in a grid layout
    ImGui::Spacing();
    
    // Top row (Up button)
    ImGui::Columns(3, nullptr, false);
    ImGui::NextColumn(); // Skip first column
    if (ImGui::Button("↑##Up", ImVec2(50, 30))) {
        moveSprite(0, -m_moveStep);
    }
    ImGui::NextColumn(); // Skip third column
    
    // Middle row (Left, Reset, Right)
    ImGui::NextColumn();
    if (ImGui::Button("←##Left", ImVec2(50, 30))) {
        moveSprite(-m_moveStep, 0);
    }
    ImGui::NextColumn();
    if (ImGui::Button("⌂##Center", ImVec2(50, 30))) {
        Transform transform = m_selectedSprite->getLocalTransform();
        transform.x = 0;
        transform.y = 0;
        m_selectedSprite->setTransform(transform);
        std::cout << "Centered sprite" << std::endl;
    }
    ImGui::NextColumn();
    if (ImGui::Button("→##Right", ImVec2(50, 30))) {
        moveSprite(m_moveStep, 0);
    }
    
    // Bottom row (Down button)
    ImGui::NextColumn(); // Skip first column
    if (ImGui::Button("↓##Down", ImVec2(50, 30))) {
        moveSprite(0, m_moveStep);
    }
    
    ImGui::Columns(1); // Reset columns
    
    // Keyboard shortcut info
    ImGui::Spacing();
    ImGui::Text("Keyboard Shortcuts:");
    ImGui::Text("Arrow Keys / WASD: Move sprite");
    ImGui::Text("Shift + Movement: 5x faster");
    ImGui::Text("(Hold keys for continuous movement)");
}

void SpriteInspectorPanel::moveSprite(float deltaX, float deltaY) {
    if (!m_selectedSprite) return;
    
    Transform transform = m_selectedSprite->getLocalTransform();
    transform.x += deltaX;
    transform.y += deltaY;
    m_selectedSprite->setTransform(transform);
    
    // Only log for discrete movements (not continuous ones)
    if (deltaX >= 1.0f || deltaY >= 1.0f || deltaX <= -1.0f || deltaY <= -1.0f) {
        std::cout << "Moved sprite by (" << deltaX << ", " << deltaY << ") to (" << transform.x << ", " << transform.y << ")" << std::endl;
    }
}

void SpriteInspectorPanel::renderSpriteProperties() {
    ImGui::Text("Sprite Properties");
    
    if (!m_selectedSprite) return;
    
    // Display read-only information
    ImGui::Text("Name: %s", m_selectedSprite->getName().c_str());
    ImGui::Text("Texture Path: %s", m_selectedSprite->getTexturePath().c_str());
    
    // FIXED: Updated bone binding info using correct method names (NO LEGACY)
    ImGui::Separator();
    ImGui::Text("Bone Bindings:");
    
    if (m_selectedSprite->isBoundToBones()) {
        // Show multiple bone bindings
        const auto& bindings = m_selectedSprite->getBoneBindings();
        ImGui::Text("Current bindings (%zu):", bindings.size());
        
        for (size_t i = 0; i < bindings.size(); ++i) {
            const auto& binding = bindings[i];
            ImGui::Text("- %s (weight: %.2f)", 
                       binding.bone->getName().c_str(), 
                       binding.weight);
            
            // Add individual unbind buttons
            ImGui::SameLine();
            std::string unbindLabel = "Unbind##" + std::to_string(i);
            if (ImGui::Button(unbindLabel.c_str())) {
                m_selectedSprite->unbindFromBone(binding.bone);
                std::cout << "Unbound sprite from bone: " << binding.bone->getName() << std::endl;
            }
        }
        
        // Clear all bindings button
        if (ImGui::Button("Clear All Bindings")) {
            m_selectedSprite->clearAllBindings();
            std::cout << "Cleared all bindings from sprite" << std::endl;
        }
    } else {
        ImGui::Text("No bone bindings");
        ImGui::Text("Use Binding Mode (Tool 5) to bind to bones");
    }
    
    // Quick binding section
    ImGui::Separator();
    ImGui::Text("Quick Binding:");
    
    if (m_character && m_character->getRig()) {
        const auto& bones = m_character->getRig()->getAllBones();
        if (!bones.empty()) {
            ImGui::Text("Bind to bone:");
            
            // Create a combo box for bone selection
            static int selectedBoneIndex = -1;
            std::vector<const char*> boneNames;
            boneNames.push_back("Select bone...");
            
            for (const auto& bone : bones) {
                boneNames.push_back(bone->getName().c_str());
            }
            
            if (ImGui::Combo("##BoneSelect", &selectedBoneIndex, boneNames.data(), boneNames.size())) {
                if (selectedBoneIndex > 0 && selectedBoneIndex <= (int)bones.size()) {
                    // Bind to selected bone
                    auto selectedBone = bones[selectedBoneIndex - 1];
                    m_selectedSprite->bindToBone(selectedBone, 1.0f);
                    std::cout << "Bound sprite to bone: " << selectedBone->getName() << std::endl;
                    selectedBoneIndex = -1; // Reset selection
                }
            }
        } else {
            ImGui::Text("No bones available");
            ImGui::Text("Create bones in scene first");
        }
    }
    
    // Transform properties (read-only for now)
    ImGui::Separator();
    ImGui::Text("Transform (World):");
    Transform worldTransform = m_selectedSprite->getWorldTransform();
    ImGui::Text("Position: (%.1f, %.1f)", worldTransform.x, worldTransform.y);
    ImGui::Text("Rotation: %.1f°", worldTransform.rotation * 180.0f / 3.14159f);
    ImGui::Text("Scale: (%.2f, %.2f)", worldTransform.scaleX, worldTransform.scaleY);
    
    // Layer controls (for future implementation)
    ImGui::Spacing();
    ImGui::Text("Layer Controls (Future):");
    if (ImGui::Button("Move to Front")) {
        // TODO: Implement layer reordering
        std::cout << "Move to front - not implemented yet" << std::endl;
    }
    ImGui::SameLine();
    if (ImGui::Button("Move to Back")) {
        // TODO: Implement layer reordering
        std::cout << "Move to back - not implemented yet" << std::endl;
    }
    
    // Delete sprite button
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Delete Sprite")) {
        // TODO: Implement sprite deletion
        std::cout << "Delete sprite - not implemented yet" << std::endl;
    }
    ImGui::PopStyleColor();
}

} // namespace Riggle