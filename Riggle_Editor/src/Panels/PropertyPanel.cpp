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
        case PropertyType::None:
        default:
            renderEmptyState();
            break;
    }

    ImGui::End();
}

void PropertyPanel::update(sf::RenderWindow& window) {
    // Handle continuous keyboard input for smooth movement
    if (m_selectedSprite && m_currentType == PropertyType::Sprite) {
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

void PropertyPanel::setSelectedBone(Bone* bone) {
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

    ImGui::Text("Sprite Properties");
    ImGui::Separator();

    // Basic info
    ImGui::Text("Name: %s", m_selectedSprite->getName().c_str());
    ImGui::Text("Texture: %s", m_selectedSprite->getTexturePath().c_str());
    
    ImGui::Spacing();
    
    // Transform controls
    renderTransformControls();
    
    ImGui::Separator();
    
    // Movement controls
    renderMovementButtons();
    
    ImGui::Separator();
    
    // Bone bindings
    renderSpriteBindings();
}

void PropertyPanel::renderBoneProperties() {
    // TODO: Implement bone properties for future Hierarchy panel
    ImGui::Text("Bone Properties");
    ImGui::Separator();
    ImGui::Text("Bone properties will be implemented");
    ImGui::Text("when the Hierarchy panel is added.");
}

void PropertyPanel::renderTransformControls() {
    ImGui::Text("Transform");
    
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

void PropertyPanel::renderMovementButtons() {
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
}

void PropertyPanel::renderSpriteBindings() {
    ImGui::Text("Bone Bindings");
    
    if (!m_selectedSprite) return;
    
    if (m_selectedSprite->isBoundToBones()) {
        const auto& bindings = m_selectedSprite->getBoneBindings();
        ImGui::Text("Current bindings (%zu):", bindings.size());
        
        for (size_t i = 0; i < bindings.size(); ++i) {
            const auto& binding = bindings[i];
            ImGui::Text("- %s (weight: %.2f)", 
                       binding.bone->getName().c_str(), 
                       binding.weight);
            
            ImGui::SameLine();
            std::string unbindLabel = "Unbind##" + std::to_string(i);
            if (ImGui::Button(unbindLabel.c_str())) {
                m_selectedSprite->unbindFromBone(binding.bone);
                std::cout << "Unbound sprite from bone: " << binding.bone->getName() << std::endl;
            }
        }
        
        if (ImGui::Button("Clear All Bindings")) {
            m_selectedSprite->clearAllBindings();
            std::cout << "Cleared all bindings from sprite" << std::endl;
        }
    } else {
        ImGui::Text("No bone bindings");
        ImGui::Text("Use Binding Mode to bind to bones");
    }
    
    // Quick binding section
    ImGui::Separator();
    ImGui::Text("Quick Binding:");
    
    if (m_character && m_character->getRig()) {
        const auto& bones = m_character->getRig()->getAllBones();
        if (!bones.empty()) {
            static int selectedBoneIndex = -1;
            std::vector<const char*> boneNames;
            boneNames.push_back("Select bone...");
            
            for (const auto& bone : bones) {
                boneNames.push_back(bone->getName().c_str());
            }
            
            if (ImGui::Combo("##BoneSelect", &selectedBoneIndex, boneNames.data(), boneNames.size())) {
                if (selectedBoneIndex > 0 && selectedBoneIndex <= (int)bones.size()) {
                    auto selectedBone = bones[selectedBoneIndex - 1];
                    m_selectedSprite->bindToBone(selectedBone, 1.0f);
                    std::cout << "Bound sprite to bone: " << selectedBone->getName() << std::endl;
                    selectedBoneIndex = -1;
                }
            }
        } else {
            ImGui::Text("No bones available");
        }
    }
}

void PropertyPanel::moveSprite(float deltaX, float deltaY) {
    if (!m_selectedSprite) return;
    
    Transform transform = m_selectedSprite->getLocalTransform();
    transform.x += deltaX;
    transform.y += deltaY;
    m_selectedSprite->setTransform(transform);
}

} // namespace Riggle