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
    // Since SFML 3.0 event handling varies, let's use a simpler approach
    // We'll handle keyboard input primarily in the update() method using isKeyPressed()
    // This is more reliable across different SFML versions
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
            
            if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                m_selectedSprite = sprite.get();
                if (m_onSpriteSelected) {
                    m_onSpriteSelected(m_selectedSprite);
                }
                std::cout << "Selected sprite from list: " << sprite->getName() << std::endl;
            }
            
            // Tooltip with more info
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Name: %s", sprite->getName().c_str());
                ImGui::Text("Path: %s", sprite->getTexturePath().c_str());
                ImGui::Text("Position: (%.1f, %.1f)", transform.x, transform.y);
                ImGui::Text("Rotation: %.1f°", transform.rotation * 180.0f / 3.14159f);
                ImGui::Text("Scale: (%.2f, %.2f)", transform.scaleX, transform.scaleY);
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
    
    // Bone attachment info
    if (m_selectedSprite->isAttachedToBone()) {
        ImGui::Text("Attached to Bone: %s", m_selectedSprite->getAttachedBone()->getName().c_str());
        if (ImGui::Button("Detach from Bone")) {
            m_selectedSprite->detachFromBone();
            std::cout << "Detached sprite from bone" << std::endl;
        }
    } else {
        ImGui::Text("Not attached to any bone");
    }
    
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