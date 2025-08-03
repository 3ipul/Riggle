#include "Editor/Panels/PropertyPanel.h"
#include <imgui.h>
#include <iostream>

namespace Riggle {

PropertyPanel::PropertyPanel()
    : BasePanel("Properties")
    , m_character(nullptr)
    , m_currentType(PropertyType::None)
    , m_selectedSprite(nullptr)
    , m_selectedBone(nullptr)
    , m_moveStep(10.0f)
    , m_moveStepInt(10)
    , m_showHelp(false)
{
}

void PropertyPanel::render() {
    if (!m_isVisible) return;

    ImGui::Begin(m_name.c_str(), &m_isVisible);

    switch (m_currentType) {
        case PropertyType::Sprite:
            renderSpriteProperties();
            break;
        case PropertyType::Bone:
            renderBoneProperties();
            break;
        case PropertyType::MultiSelection:
            renderMultiSelectionProperties();
            break;
        case PropertyType::None:
        default:
            renderEmptyState();
            break;
    }

    renderHelpDialog();
    ImGui::End();
}

void PropertyPanel::update(sf::RenderWindow& window) {
    // Handle continuous keyboard input for smooth movement
    // Only allow keyboard movement for unbound sprites
    if (m_selectedSprite && m_currentType == PropertyType::Sprite && !m_selectedSprite->isBoundToBone()) {
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

void PropertyPanel::setSelectedSprite(Sprite* sprite) {
    m_selectedSprite = sprite;
    m_selectedBone = nullptr;
    m_currentType = sprite ? PropertyType::Sprite : PropertyType::None;
}

void PropertyPanel::setSelectedBone(std::shared_ptr<Bone> bone) {
    m_selectedBone = bone;
    m_selectedSprite = nullptr;
    m_currentType = bone ? PropertyType::Bone : PropertyType::None;
}

void PropertyPanel::clearSelection() {
    m_selectedSprite = nullptr;
    m_selectedBone = nullptr;
    m_currentType = PropertyType::None;
}

void PropertyPanel::renderEmptyState() {
    ImGui::Spacing();
    ImGui::TextWrapped("No object selected");
    ImGui::Spacing();
    ImGui::TextWrapped("Select a sprite from the Assets panel or a bone from the Hierarchy panel to view its properties.");
}

void PropertyPanel::renderSpriteProperties() {
    if (!m_selectedSprite) {
        renderEmptyState();
        return;
    }

    // Header with help button
    ImGui::Text("Sprite Properties");
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 50);
    if (ImGui::Button("Help")) {
        m_showHelp = true;
    }
    ImGui::Separator();

    // Basic info (always shown)
    ImGui::Text("Name: %s", m_selectedSprite->getName().c_str());
    ImGui::Text("Texture: %s", m_selectedSprite->getTexturePath().c_str());
    
    ImGui::Spacing();
    
    // Check if sprite is bound to bones
    bool isBound = m_selectedSprite->isBoundToBone();
    
    if (isBound) {
        // BOUND SPRITE: Show read-only info
        renderBoundSpriteInfo();
    } else {
        // UNBOUND SPRITE: Show full transform controls
        renderTransformControls();
        ImGui::Separator();
        renderMovementButtons();
    }
    
    ImGui::Separator();
    renderSpriteBindings();
}

void PropertyPanel::renderBoundSpriteInfo() {
    ImGui::Text("Transform (Read-Only)");
    ImGui::Spacing();
    
    Transform transform = m_selectedSprite->getLocalTransform();
    
    // Position (read-only)
    ImGui::Text("Position: (%.1f, %.1f)", transform.position.x, transform.position.y);
    
    // Rotation (read-only, show in degrees)
    float rotationDegrees = transform.rotation * 180.0f / 3.14159f;
    ImGui::Text("Rotation: %.1f°", rotationDegrees);
    
    // Scale (read-only)
    ImGui::Text("Scale: (%.2f, %.2f)", transform.scale.x, transform.scale.y);
    
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.3f, 1.0f)); // Yellow text
    ImGui::TextWrapped("Transform is controlled by bone bindings. Unbind from bones to edit manually.");
    ImGui::PopStyleColor();
}

void PropertyPanel::renderBoneProperties() {
    if (!m_selectedBone) {
        renderEmptyState();
        return;
    }
    
    // Header with help button
    ImGui::Text("Bone Properties");
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 50);
    if (ImGui::Button("Help##Bone")) {
        m_showHelp = true;
    }
    ImGui::Separator();
    
    // Basic info
    ImGui::Text("Name: %s", m_selectedBone->getName().c_str());
    ImGui::Separator();
    
    // Transform controls
    ImGui::Text("Transform");
    Transform transform = m_selectedBone->getLocalTransform();
    bool transformChanged = false;
    
    // Position (pivot point)
    float pos[2] = { transform.position.x, transform.position.y };
    if (ImGui::DragFloat2("Position", pos, 1.0f, -1000.0f, 1000.0f, "%.1f")) {
        transform.position.x = pos[0];
        transform.position.y = pos[1];
        transformChanged = true;
    }
    
    // Rotation (convert to degrees for UI)
    float rotationDegrees = transform.rotation * 180.0f / 3.14159f;
    if (ImGui::DragFloat("Rotation", &rotationDegrees, 1.0f, -360.0f, 360.0f, "%.1f°")) {
        transform.rotation = rotationDegrees * 3.14159f / 180.0f;
        transformChanged = true;
    }
    
    // Apply transform changes if any
    if (transformChanged) {
        m_selectedBone->setLocalTransform(transform);
        
        // Force update of entire hierarchy
        if (m_character && m_character->getRig()) {
            m_character->getRig()->forceUpdateWorldTransforms();
            m_character->updateDeformations();
        }
    }
    
    ImGui::Separator();
    
    // Length control (separate from transform)
    ImGui::Text("Bone Geometry");
    float length = m_selectedBone->getLength();
    if (ImGui::DragFloat("Length", &length, 1.0f, 1.0f, 500.0f, "%.1f px")) {
        m_selectedBone->setLength(length);  // This will call our new setLength method
        
        // Force update after length change
        if (m_character && m_character->getRig()) {
            m_character->getRig()->forceUpdateWorldTransforms();
            m_character->updateDeformations();
        }
    }
    
    ImGui::Separator();
    
    // Hierarchy info
    ImGui::Text("Hierarchy");
    if (auto parent = m_selectedBone->getParent()) {
        ImGui::Text("Parent: %s", parent->getName().c_str());
    } else {
        ImGui::Text("Parent: (Root bone)");
    }
    ImGui::Text("Children: %zu", m_selectedBone->getChildren().size());
    
    // Show hierarchy depth for debugging
    int depth = 0;
    auto current = m_selectedBone;
    while (auto parent = current->getParent()) {
        depth++;
        current = parent;
    }
    ImGui::Text("Hierarchy Depth: %d", depth);
}

void PropertyPanel::renderTransformControls() {
    ImGui::Text("Transform");
    
    if (!m_selectedSprite) return;
    
    Transform transform = m_selectedSprite->getLocalTransform();
    bool changed = false;
    
    // Position controls - using the correct member names from your Transform
    if (ImGui::DragFloat2("Position", &transform.position.x, 1.0f, -10000.0f, 10000.0f, "%.1f")) {
        changed = true;
    }
    
    // Rotation control (convert to degrees for UI)
    float rotationDegrees = transform.rotation * 180.0f / 3.14159f;
    if (ImGui::DragFloat("Rotation", &rotationDegrees, 1.0f, -360.0f, 360.0f, "%.1f°")) {
        transform.rotation = rotationDegrees * 3.14159f / 180.0f;
        changed = true;
    }
    
    // Scale controls
    if (ImGui::DragFloat2("Scale", &transform.scale.x, 0.01f, 0.1f, 10.0f, "%.2f")) {
        changed = true;
    }
    
    // Apply changes
    if (changed) {
        m_selectedSprite->setTransform(transform);
    }
    
    // Reset buttons row
    if (ImGui::Button("Reset Position")) {
        Transform resetTransform = m_selectedSprite->getLocalTransform();
        resetTransform.position = {0.0f, 0.0f};
        m_selectedSprite->setTransform(resetTransform);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Rotation")) {
        Transform resetTransform = m_selectedSprite->getLocalTransform();
        resetTransform.rotation = 0.0f;
        m_selectedSprite->setTransform(resetTransform);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Scale")) {
        Transform resetTransform = m_selectedSprite->getLocalTransform();
        resetTransform.scale = {1.0f, 1.0f};
        m_selectedSprite->setTransform(resetTransform);
    }
}

void PropertyPanel::renderMovementButtons() {
    ImGui::Text("Precise Movement");
    
    // Movement step control
    ImGui::Text("Move Step:");
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderInt("##MoveStep", &m_moveStepInt, 1, 100, "%d px")) {
        m_moveStep = static_cast<float>(m_moveStepInt);
    }
    
    ImGui::Spacing();
    
    // Movement buttons in proper grid layout
    float buttonSize = 40.0f;
    float spacing = 5.0f;
    float totalWidth = buttonSize * 3 + spacing * 2;
    float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
    
    // Top row (Up button) - centered
    ImGui::SetCursorPosX(startX + buttonSize + spacing);
    if (ImGui::Button("Up", ImVec2(buttonSize, buttonSize))) {
        moveSprite(0, -m_moveStep);
    }
    
    // Middle row (Left, Center, Right)
    ImGui::SetCursorPosX(startX);
    if (ImGui::Button("Left", ImVec2(buttonSize, buttonSize))) {
        moveSprite(-m_moveStep, 0);
    }
    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Reset", ImVec2(buttonSize, buttonSize))) {
        Transform transform = m_selectedSprite->getLocalTransform();
        transform.position = {0.0f, 0.0f};
        m_selectedSprite->setTransform(transform);
        std::cout << "Centered sprite" << std::endl;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset to center (0, 0)");
    }
    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Right", ImVec2(buttonSize, buttonSize))) {
        moveSprite(m_moveStep, 0);
    }
    
    // Bottom row (Down button) - centered
    ImGui::SetCursorPosX(startX + buttonSize + spacing);
    if (ImGui::Button("Down", ImVec2(buttonSize, buttonSize))) {
        moveSprite(0, m_moveStep);
    }
}

void PropertyPanel::renderSpriteBindings() {
    ImGui::Text("Bone Binding");
    
    if (!m_selectedSprite) return;
    
    if (m_selectedSprite->isBoundToBone()) {
        auto boundBone = m_selectedSprite->getBoundBone();
        if (boundBone && boundBone.use_count() > 0) {  // Check if bone is still valid
            const auto& binding = m_selectedSprite->getBoneBinding();
            
            ImGui::Text("Bound to: %s", boundBone->getName().c_str());
            ImGui::Text("Offset: (%.1f, %.1f)", binding.bindOffset.x, binding.bindOffset.y);
            ImGui::Text("Rotation: %.1f°", binding.bindRotation * 180.0f / 3.14159f);
            
            if (ImGui::Button("Unbind from Bone")) {
                m_selectedSprite->unbindFromBone();
                std::cout << "Unbound sprite from bone: " << boundBone->getName() << std::endl;
            }
        } else {
            ImGui::Text("Bound to invalid bone (cleaning up...)");
            m_selectedSprite->unbindFromBone();  // Clean up invalid binding
        }
    } else {
        ImGui::Text("Not bound to any bone");
    }
    
    // Quick binding section
    ImGui::Separator();
    ImGui::Text("Manual Binding:");
    
    if (!m_character || !m_character->getRig()) {
        ImGui::Text("No rig available");
        return;
    }
    
    const auto& bones = m_character->getRig()->getAllBones();
    if (bones.empty()) {
        ImGui::Text("No bones available");
        return;
    }
    
    // FIXED: Use a safer approach with individual buttons instead of Combo
    ImGui::Text("Select bone to bind:");
    
    // Create scrollable region for bones
    if (ImGui::BeginChild("BoneList", ImVec2(0, 150), true)) {
        for (size_t i = 0; i < bones.size(); ++i) {
            const auto& bone = bones[i];
            if (!bone || bone.use_count() == 0) continue;  // Skip invalid bones
            
            std::string buttonLabel = bone->getName() + " (" + std::to_string(bone->getSpriteCount()) + " sprites)";
            
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(-1, 0))) {
                try {
                    // Store sprite's current transform
                    Transform spriteTransform = m_selectedSprite->getLocalTransform();
                    Vector2 spritePos = Vector2(spriteTransform.position.x, spriteTransform.position.y);
                    float spriteRot = spriteTransform.rotation;
                    
                    // Calculate binding offset
                    Transform boneTransform = bone->getWorldTransform();
                    Vector2 bonePos = Vector2(boneTransform.position.x, boneTransform.position.y);
                    
                    Vector2 offset;
                    offset.x = spritePos.x - bonePos.x;
                    offset.y = spritePos.y - bonePos.y;
                    float rotationOffset = spriteRot - boneTransform.rotation;
                    
                    // Perform binding
                    m_selectedSprite->bindToBone(bone, offset, rotationOffset);
                    
                    // Verify position didn't change
                    Transform newTransform = m_selectedSprite->getLocalTransform();
                    Vector2 newPos = Vector2(newTransform.position.x, newTransform.position.y);
                    
                    float posDiff = std::sqrt(std::pow(newPos.x - spritePos.x, 2) + 
                                            std::pow(newPos.y - spritePos.y, 2));
                    
                    if (posDiff > 0.1f) {
                        std::cout << "Warning: Manual binding changed sprite position!" << std::endl;
                        // Correct the position
                        Transform correctedTransform = m_selectedSprite->getLocalTransform();
                        correctedTransform.position.x = spritePos.x;
                        correctedTransform.position.y = spritePos.y;
                        m_selectedSprite->setTransform(correctedTransform);
                    }
                    
                    std::cout << "Manually bound sprite to bone: " << bone->getName() << std::endl;
                    
                } catch (const std::exception& e) {
                    std::cout << "Error during manual binding: " << e.what() << std::endl;
                }
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to bind sprite to this bone");
            }
        }
    }
    ImGui::EndChild();
}

void PropertyPanel::renderHelpDialog() {
    if (!m_showHelp) return;
    
    ImGui::OpenPopup("Properties Help");
    
    if (ImGui::BeginPopupModal("Properties Help", &m_showHelp, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (m_currentType == PropertyType::Sprite) {
            ImGui::Text("Sprite Properties Help");
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::Text("Mouse Controls:");
            ImGui::BulletText("Use directional buttons (Up, Down, Left, Right) for precise movement");
            ImGui::BulletText("Reset button resets sprite to center (0, 0)");
            ImGui::BulletText("Adjust 'Move Step' slider to change movement distance");
            
            ImGui::Spacing();
            ImGui::Text("Keyboard Shortcuts (for unbound sprites):");
            ImGui::BulletText("Arrow Keys or WASD: Move sprite continuously");
            ImGui::BulletText("Hold Shift + Movement: Move 5x faster");
            
            ImGui::Spacing();
            ImGui::Text("Transform Controls:");
            ImGui::BulletText("Drag fields to adjust position, rotation, and scale");
            ImGui::BulletText("Use Reset buttons to quickly restore default values");
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f)); // Yellow
            ImGui::TextWrapped("Note: Sprites bound to bones cannot be moved manually. Their transform is controlled by the bone system.");
            ImGui::PopStyleColor();
        } else if (m_currentType == PropertyType::Bone) {
            ImGui::Text("Bone Properties Help");
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::Text("Bone Controls:");
            ImGui::BulletText("Length: Adjust the bone's length");
            ImGui::BulletText("View hierarchy and sprite bindings");
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f)); // Green
            ImGui::TextWrapped("More bone editing features will be added later.");
            ImGui::PopStyleColor();
        }
        
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            m_showHelp = false;
        }
        
        ImGui::EndPopup();
    }
}

void PropertyPanel::moveSprite(float deltaX, float deltaY) {
    if (!m_selectedSprite) return;
    
    Transform transform = m_selectedSprite->getLocalTransform();
    transform.position.x += deltaX;
    transform.position.y += deltaY;
    m_selectedSprite->setTransform(transform);
}

void PropertyPanel::setMultiSelection(Sprite* sprite, std::shared_ptr<Bone> bone) {
    m_selectedSprite = sprite;
    m_selectedBone = bone;
    m_currentType = (sprite && bone) ? PropertyType::MultiSelection : 
                   (sprite ? PropertyType::Sprite : 
                   (bone ? PropertyType::Bone : PropertyType::None));
}

void PropertyPanel::renderMultiSelectionProperties() {
    if (!m_selectedSprite || !m_selectedBone) {
        renderEmptyState();
        return;
    }
    
    ImGui::Text("Multi-Selection Properties");
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 50);
    if (ImGui::Button("Help##Multi")) {
        m_showHelp = true;
    }
    ImGui::Separator();
    
    // Sprite info (read-only)
    ImGui::Text("Selected Sprite:");
    ImGui::Indent();
    ImGui::Text("Name: %s", m_selectedSprite->getName().c_str());
    ImGui::Text("Texture: %s", m_selectedSprite->getTexturePath().c_str());
    Transform spriteTransform = m_selectedSprite->getLocalTransform();
    ImGui::Text("Position: (%.1f, %.1f)", spriteTransform.position.x, spriteTransform.position.y);
    ImGui::Text("Bound: %s", m_selectedSprite->isBoundToBone() ? "Yes" : "No");
    ImGui::Unindent();
    
    ImGui::Spacing();
    
    // Bone info (read-only)
    ImGui::Text("Selected Bone:");
    ImGui::Indent();
    ImGui::Text("Name: %s", m_selectedBone->getName().c_str());
    ImGui::Text("Length: %.1f", m_selectedBone->getLength());
    if (m_selectedBone->getParent()) {
        ImGui::Text("Parent: %s", m_selectedBone->getParent()->getName().c_str());
    } else {
        ImGui::Text("Parent: Root");
    }
    ImGui::Text("Bound Sprites: %zu", m_selectedBone->getSpriteCount());
    ImGui::Unindent();
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Binding controls
    renderBindingControls();
    
    ImGui::Spacing();
    ImGui::Separator();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    ImGui::TextWrapped("Transform controls are disabled during multi-selection. "
                      "Select individual sprite or bone to edit transforms.");
    ImGui::PopStyleColor();
}

void PropertyPanel::renderBindingControls() {
    ImGui::Text("Binding Controls");
    
    if (!m_selectedSprite || !m_selectedBone) return;
    
    bool isAlreadyBound = (m_selectedSprite->isBoundToBone() && 
                          m_selectedSprite->getBoundBone() == m_selectedBone);
    
    if (isAlreadyBound) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        ImGui::Text("✓ Sprite is bound to this bone");
        ImGui::PopStyleColor();
        
        const auto& binding = m_selectedSprite->getBoneBinding();
        ImGui::Text("Offset: (%.1f, %.1f)", binding.bindOffset.x, binding.bindOffset.y);
        ImGui::Text("Rotation: %.1f°", binding.bindRotation * 180.0f / 3.14159f);
        
        if (ImGui::Button("Unbind Sprite")) {
            m_selectedSprite->unbindFromBone();
            std::cout << "Unbound sprite '" << m_selectedSprite->getName() 
                      << "' from bone '" << m_selectedBone->getName() << "'" << std::endl;
        }
    } else {
        if (m_selectedSprite->isBoundToBone()) {
            auto currentBone = m_selectedSprite->getBoundBone();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.3f, 1.0f));
            ImGui::Text("Sprite is bound to: %s", currentBone->getName().c_str());
            ImGui::PopStyleColor();
            ImGui::Text("Binding to new bone will replace existing binding.");
        }
        
        if (ImGui::Button("Bind Sprite to Bone")) {
            // FIXED: Approach 3 - Manual binding with position preservation
            try {
                // Store original sprite world position
                Transform originalSpriteWorld = m_selectedSprite->getWorldTransform();
                Vector2 originalPos = originalSpriteWorld.position;
                float originalRot = originalSpriteWorld.rotation;
                
                // Get bone world transform
                Transform boneWorld = m_selectedBone->getWorldTransform();
                
                // Calculate binding offset to maintain current sprite position
                Vector2 bindOffset;
                bindOffset.x = originalPos.x - boneWorld.position.x;
                bindOffset.y = originalPos.y - boneWorld.position.y;
                
                float bindRotation = originalRot - boneWorld.rotation;
                
                // Perform binding
                m_selectedSprite->bindToBone(m_selectedBone, bindOffset, bindRotation);
                
                // Verify position is maintained
                Transform newSpriteWorld = m_selectedSprite->getWorldTransform();
                float posDiff = std::sqrt(std::pow(newSpriteWorld.position.x - originalPos.x, 2) + 
                                        std::pow(newSpriteWorld.position.y - originalPos.y, 2));
                
                if (posDiff > 0.1f) {
                    std::cout << "Warning: Manual binding changed sprite position by " << posDiff << " units" << std::endl;
                }
                
                std::cout << "Approach 3: Manually bound sprite '" << m_selectedSprite->getName() 
                          << "' to bone '" << m_selectedBone->getName() << "'" << std::endl;
                
            } catch (const std::exception& e) {
                std::cout << "Error during manual binding: " << e.what() << std::endl;
            }
        }
    }
}

} // namespace Riggle