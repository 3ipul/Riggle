#pragma once
#include <SFML/Graphics.hpp>
#include <Riggle/Rig.h>
#include <Riggle/Bone.h>
#include <memory>
#include <functional>

namespace Riggle {

enum class BoneToolMode {
    Creating,
    Selecting,
    Rotating,    // NEW: For FK rotation
    Moving
};

enum class BoneHandle {
    None,
    Start,
    End,
    Shaft
};

class BoneCreationTool {
public:
    BoneCreationTool();
    ~BoneCreationTool() = default;

    void setRig(Rig* rig) { m_rig = rig; }
    
    // Tool state
    void setActive(bool active) { m_isActive = active; }
    bool isActive() const { return m_isActive; }
    
    BoneToolMode getMode() const { return m_mode; }
    void setMode(BoneToolMode mode) { m_mode = mode; }

    // Mouse handling
    void handleMousePress(const sf::Vector2f& worldPos);
    void handleMouseMove(const sf::Vector2f& worldPos);
    void handleMouseRelease(const sf::Vector2f& worldPos);

    // Selection
    std::shared_ptr<Bone> getSelectedBone() const { return m_selectedBone; }
    void setSelectedBone(std::shared_ptr<Bone> bone) { m_selectedBone = bone; }

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

    // Utility
    std::shared_ptr<Bone> findBoneAtPosition(const sf::Vector2f& worldPos, float tolerance = 10.0f);
    BoneHandle getBoneHandleAtPosition(std::shared_ptr<Bone> bone, const sf::Vector2f& worldPos, float tolerance = 15.0f);
    void cancelCurrentOperation();

    // Get creation preview for rendering
    bool isCreating() const { return m_isCreating; }
    sf::Vector2f getCreationStart() const { return m_creationStart; }
    sf::Vector2f getCreationEnd() const { return m_creationEnd; }
    
    // Get rotation preview for rendering
    bool isRotating() const { return m_isRotating; }
    std::shared_ptr<Bone> getRotatingBone() const { return m_rotatingBone; }
    float getRotationPreview() const { return m_rotationPreview; }

private:
    Rig* m_rig;
    bool m_isActive;
    BoneToolMode m_mode;
    
    // Creation state
    bool m_isCreating;
    sf::Vector2f m_creationStart;
    sf::Vector2f m_creationEnd;
    
    // Selection state
    std::shared_ptr<Bone> m_selectedBone;
    sf::Vector2f m_dragOffset;
    
    // Rotation state (NEW)
    bool m_isRotating;
    std::shared_ptr<Bone> m_rotatingBone;
    sf::Vector2f m_rotationStart;
    float m_initialRotation;
    float m_rotationPreview;
    BoneHandle m_grabbedHandle;
    
    // Callbacks
    std::function<void(std::shared_ptr<Bone>)> m_onBoneCreated;
    std::function<void(std::shared_ptr<Bone>)> m_onBoneSelected;
    std::function<void(std::shared_ptr<Bone>, float)> m_onBoneRotated;
    
    // Helper functions
    std::string generateBoneName();
    float distanceToLine(const sf::Vector2f& point, const sf::Vector2f& lineStart, const sf::Vector2f& lineEnd);
    float calculateAngle(const sf::Vector2f& center, const sf::Vector2f& point);
    float angleDifference(float angle1, float angle2);
};

} // namespace Riggle