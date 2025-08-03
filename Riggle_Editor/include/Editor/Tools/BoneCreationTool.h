#pragma once
#include <Riggle/Character.h>
#include <Riggle/Bone.h>
#include <Riggle/Rig.h>
#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>

namespace Riggle {

enum class BoneCreationState {
    Idle,           // No bone creation in progress
    Creating,       // Mouse pressed, dragging to set bone length and direction
    Complete        // Bone created
};

class BoneCreationTool {
public:
    BoneCreationTool();
    ~BoneCreationTool() = default;

    // Setup
    void setCharacter(Character* character) { m_character = character; }
    void setActive(bool active);
    bool isActive() const { return m_isActive; }

    // Bone creation
    void handleMousePressed(const sf::Vector2f& worldPos);
    void handleMouseMoved(const sf::Vector2f& worldPos);
    void handleMouseReleased(const sf::Vector2f& worldPos);
    
    // Selection management
    void setSelectedBone(std::shared_ptr<Bone> bone) { m_selectedBone = bone; }
    std::shared_ptr<Bone> getSelectedBone() const { return m_selectedBone; }
    void clearSelection() { m_selectedBone = nullptr; }
    void clearInternalSelection() {m_selectedBone = nullptr; }

    // Bone picking
    std::shared_ptr<Bone> findBoneAtPosition(const sf::Vector2f& worldPos);
    
    // Rendering
    void renderOverlay(sf::RenderTarget& target, float zoomLevel = 1.0f);
    
    // State
    bool isCreating() const { return m_state == BoneCreationState::Creating; }
    sf::Vector2f getCreationStart() const { return m_startPosition; }
    sf::Vector2f getCreationEnd() const { return m_endPosition; }
    
    // Callbacks
    void setOnBoneCreated(std::function<void(std::shared_ptr<Bone>)> callback) {
        m_onBoneCreated = callback;
    }
    
    void setOnBoneSelected(std::function<void(std::shared_ptr<Bone>)> callback) {
        m_onBoneSelected = callback;
    }

    // Settings
    void setMinBoneLength(float length) { m_minBoneLength = length; }
    void setSnapToGrid(bool snap) { m_snapToGrid = snap; }
    void setGridSize(float size) { m_gridSize = size; }

private:
    Character* m_character;
    bool m_isActive;
    
    // Creation state
    BoneCreationState m_state;
    sf::Vector2f m_startPosition;
    sf::Vector2f m_endPosition;
    
    // Selected bone (parent for next creation)
    std::shared_ptr<Bone> m_selectedBone;
    
    // Settings
    float m_minBoneLength;
    bool m_snapToGrid;
    float m_gridSize;
    
    // Colors
    sf::Color m_previewColor;
    sf::Color m_validColor;
    sf::Color m_invalidColor;
    sf::Color m_selectedColor;
    
    // Callbacks
    std::function<void(std::shared_ptr<Bone>)> m_onBoneCreated;
    std::function<void(std::shared_ptr<Bone>)> m_onBoneSelected;
    
    // Helper methods
    void createBone();
    float calculateBoneLength() const;
    bool isValidBoneLength() const;
    sf::Vector2f snapToGrid(const sf::Vector2f& position) const;
    std::string generateBoneName() const;
    bool hasRootBone() const;
    
    void renderBonePreview(sf::RenderTarget& target, float zoomLevel);
    void renderSelectedBoneHighlight(sf::RenderTarget& target, float zoomLevel);
};

} // namespace Riggle