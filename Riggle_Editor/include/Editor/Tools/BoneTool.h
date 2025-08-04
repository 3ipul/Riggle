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

enum class BoneSubTool {
    CreateBone,
    BoneTransform,
    IKSolver
};

class BoneTool {
public:
    BoneTool();
    ~BoneTool() = default;

    // Setup
    void setCharacter(Character* character) { m_character = character; }
    void setActive(bool active);
    bool isActive() const { return m_isActive; }

    // Sub-tool management
    void setSubTool(BoneSubTool subTool);
    BoneSubTool getSubTool() const { return m_currentSubTool; }
    const char* getSubToolName() const;

    // Mouse handling (handles all sub-tools)
    void handleMousePressed(const sf::Vector2f& worldPos);
    void handleMouseMoved(const sf::Vector2f& worldPos);
    void handleMouseReleased(const sf::Vector2f& worldPos);
    
    // Selection management
    void setSelectedBone(std::shared_ptr<Bone> bone) { m_selectedBone = bone; }
    std::shared_ptr<Bone> getSelectedBone() const { return m_selectedBone; }
    void clearSelection() { m_selectedBone = nullptr; }
    void clearInternalSelection() { m_selectedBone = nullptr; }

    // Bone picking
    std::shared_ptr<Bone> findBoneAtPosition(const sf::Vector2f& worldPos);
    
    // Rendering (handles all sub-tools)
    void renderOverlay(sf::RenderTarget& target, float zoomLevel = 1.0f);
    void renderSubToolButtons(sf::RenderTarget& target); // For viewport overlay
    
    // State queries
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

    void setOnBoneRotated(std::function<void(std::shared_ptr<Bone>, float)> callback) {
        m_onBoneRotated = callback;
    }

    void setOnBoneTransformed(std::function<void(const std::string&)> callback) {
        m_onBoneTransformed = callback;
    }

    // Settings
    void setMinBoneLength(float length) { m_minBoneLength = length; }
    void setSnapToGrid(bool snap) { m_snapToGrid = snap; }
    void setGridSize(float size) { m_gridSize = size; }

    // IK Tool integration
    void setIKTool(class IKSolverTool* ikTool) { m_ikTool = ikTool; }
    class IKSolverTool* getIKTool() const { return m_ikTool; }

private:
    Character* m_character;
    bool m_isActive;
    BoneSubTool m_currentSubTool;
    
    // Creation state
    BoneCreationState m_state;
    sf::Vector2f m_startPosition;
    sf::Vector2f m_endPosition;
    
    // Selected bone
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
    sf::Color m_jointColor;
    
    // IK Tool reference
    class IKSolverTool* m_ikTool;
    
    // Callbacks
    std::function<void(std::shared_ptr<Bone>)> m_onBoneCreated;
    std::function<void(std::shared_ptr<Bone>)> m_onBoneSelected;
    std::function<void(std::shared_ptr<Bone>, float)> m_onBoneRotated;
    std::function<void(const std::string&)> m_onBoneTransformed;
    
    // Sub-tool specific handlers
    void handleBoneCreation(const sf::Vector2f& worldPos);
    void handleBoneTransform(const sf::Vector2f& worldPos);
     bool findNearbyBoneEndpoint(const sf::Vector2f& worldPos, sf::Vector2f& snapPosition, 
                               std::shared_ptr<Bone>& snapBone, bool& snapToEnd);
    // void handleIKSolver(const sf::Vector2f& worldPos);
    
    // Bone creation methods
    void createBone();
    float calculateBoneLength() const;
    bool isValidBoneLength() const;
    sf::Vector2f snapToGrid(const sf::Vector2f& position) const;
    std::string generateBoneName() const;
    bool hasRootBone() const;
    
    // Bone transform methods
    void handleBoneRotation(const sf::Vector2f& worldPos);
    
    // Rendering methods
    void renderBonePreview(sf::RenderTarget& target, float zoomLevel);
    void renderSelectedBoneHighlight(sf::RenderTarget& target, float zoomLevel);
    void renderIKOverlay(sf::RenderTarget& target, float zoomLevel);
};

} // namespace Riggle