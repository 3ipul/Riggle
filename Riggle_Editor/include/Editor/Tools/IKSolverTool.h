#pragma once
#include <Riggle/Character.h>
#include <Riggle/Bone.h>
#include <Riggle/IK_Solver.h>
#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>

namespace Riggle {

enum class IKToolState {
    WaitingForEndEffector,  // No end-effector selected
    Configured,             // End-effector selected, chain length set
    Solving                 // Mouse dragging, actively solving IK
};

class IKSolverTool {
public:
    IKSolverTool();
    ~IKSolverTool() = default;

    // Setup
    void setCharacter(Character* character) { m_character = character; }
    void setActive(bool active);
    bool isActive() const { return m_isActive; }

    // IK solving
    void handleMousePressed(const sf::Vector2f& worldPos);
    void handleMouseMoved(const sf::Vector2f& worldPos);
    void handleMouseReleased(const sf::Vector2f& worldPos);
    
    // Selection management
    void setEndEffector(std::shared_ptr<Bone> bone);
    std::shared_ptr<Bone> getEndEffector() const { return m_endEffector; }
    void clearEndEffector();

    // Chain configuration
    void setChainLength(int length);
    int getChainLength() const { return m_chainLength; }

    // State queries
    IKToolState getState() const { return m_state; }
    bool isConfigured() const { return m_state == IKToolState::Configured || m_state == IKToolState::Solving; }
    bool isSolving() const { return m_state == IKToolState::Solving; }
    
    // Validation
    IKChainValidation validateCurrentChain() const;
    std::string getStatusMessage() const;
    
    // Target position
    Vector2 getTargetPosition() const { return m_targetPosition; }
    
    // Bone picking
    std::shared_ptr<Bone> findBoneAtPosition(const sf::Vector2f& worldPos);
    
    // Rendering
    void renderOverlay(sf::RenderTarget& target, float zoomLevel = 1.0f);
    
    // Callbacks
    void setOnEndEffectorSelected(std::function<void(std::shared_ptr<Bone>)> callback) {
        m_onEndEffectorSelected = callback;
    }

private:
    Character* m_character;
    bool m_isActive;
    
    // IK state
    IKToolState m_state;
    std::shared_ptr<Bone> m_endEffector;
    int m_chainLength;
    Vector2 m_targetPosition;
    
    // Interaction state
    bool m_isDragging;
    sf::Vector2f m_lastMousePos;
    
    // Visual settings
    sf::Color m_chainColor;
    sf::Color m_endEffectorColor;
    sf::Color m_targetColor;
    
    // Callbacks
    std::function<void(std::shared_ptr<Bone>)> m_onEndEffectorSelected;
    
    // Helper methods
    void updateState();
    void solveIK(const Vector2& targetPos);
    void renderChainHighlight(sf::RenderTarget& target, float zoomLevel);
    void renderTargetMarker(sf::RenderTarget& target, float zoomLevel);
    void renderEndEffectorMarker(sf::RenderTarget& target, float zoomLevel);
};

} // namespace Riggle