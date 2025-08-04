#include "Editor/Tools/IKSolverTool.h"
#include <Riggle/Rig.h>
#include <cmath>

namespace Riggle {

IKSolverTool::IKSolverTool()
    : m_character(nullptr)
    , m_isActive(false)
    , m_state(IKToolState::WaitingForEndEffector)
    , m_chainLength(3)
    , m_targetPosition(0, 0)
    , m_isDragging(false)
    , m_chainColor(0, 255, 255, 180)      // Cyan
    , m_endEffectorColor(255, 255, 0, 255) // Yellow
    , m_targetColor(255, 0, 0, 255)        // Red
{
}

void IKSolverTool::setActive(bool active) {
    m_isActive = active;
    if (!active) {
        clearEndEffector();
        m_isDragging = false;
    }
}

void IKSolverTool::handleMousePressed(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_character) return;
    
    // First, always check if the user is clicking on a new bone.
    // This allows changing the end-effector at any time.
    auto boneUnderMouse = findBoneAtPosition(worldPos);
    
    if (boneUnderMouse && boneUnderMouse != m_endEffector) {
        // User clicked a new bone, select it as the new end-effector.
        setEndEffector(boneUnderMouse);
        m_isDragging = false; // Reset dragging state
        return; // Done for this click
    }
    
    // If we are configured and the user didn't click a *new* bone, start solving.
    if (m_state == IKToolState::Configured) {
        m_isDragging = true;
        m_lastMousePos = worldPos;
        m_targetPosition = Vector2(worldPos.x, worldPos.y);
        m_state = IKToolState::Solving;
    }
}


void IKSolverTool::handleMouseMoved(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_character) return;
    
    if (m_state == IKToolState::Solving && m_isDragging) {
        m_targetPosition = Vector2(worldPos.x, worldPos.y);
        solveIK(m_targetPosition);
        m_lastMousePos = worldPos;
    }
}

void IKSolverTool::handleMouseReleased(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_character) return;
    
    if (m_state == IKToolState::Solving && m_isDragging) {
        m_isDragging = false;
        m_state = IKToolState::Configured; // Back to configured state
    }
}

void IKSolverTool::setEndEffector(std::shared_ptr<Bone> bone) {
    m_endEffector = bone;
    updateState();
    
    if (m_onEndEffectorSelected) {
        m_onEndEffectorSelected(bone);
    }
}

void IKSolverTool::clearEndEffector() {
    m_endEffector = nullptr;
    m_state = IKToolState::WaitingForEndEffector;
}

void IKSolverTool::setChainLength(int length) {
    if (length > 0) {
        m_chainLength = length;
        updateState();
    }
}

void IKSolverTool::updateState() {
    if (!m_endEffector) {
        m_state = IKToolState::WaitingForEndEffector;
        return;
    }
    
    auto validation = validateCurrentChain();
    if (validation.isValid) {
        m_state = IKToolState::Configured;
    } else {
        m_state = IKToolState::WaitingForEndEffector;
    }
}

IKChainValidation IKSolverTool::validateCurrentChain() const {
    if (!m_character || !m_endEffector) {
        return {false, "No end effector selected", 0, {}};
    }
    
    return m_character->getIKSolver().validateChain(m_endEffector, m_chainLength);
}

std::string IKSolverTool::getStatusMessage() const {
    switch (m_state) {
        case IKToolState::WaitingForEndEffector:
            return "Select an end-effector bone to begin IK setup";
        case IKToolState::Configured: {
            auto validation = validateCurrentChain();
            if (validation.isValid) {
                return "Ready to solve - drag end-effector to target";
            } else {
                return validation.message;
            }
        }
        case IKToolState::Solving:
            return "Solving IK...";
        default:
            return "";
    }
}

void IKSolverTool::solveIK(const Vector2& targetPos) {
    if (!m_character || !m_endEffector) return;
    
    m_character->solveIK(m_endEffector, targetPos, m_chainLength);
}

std::shared_ptr<Bone> IKSolverTool::findBoneAtPosition(const sf::Vector2f& worldPos) {
    if (!m_character || !m_character->getRig()) return nullptr;
    
    const float BONE_PICK_TOLERANCE = 10.0f;
    std::shared_ptr<Bone> closestBone = nullptr;
    float closestDistance = BONE_PICK_TOLERANCE;
    
    auto allBones = m_character->getRig()->getAllBones();
    for (const auto& bone : allBones) {
        float startX, startY, endX, endY;
        bone->getWorldEndpoints(startX, startY, endX, endY);
        
        // Check distance to bone line
        sf::Vector2f start(startX, startY);
        sf::Vector2f end(endX, endY);
        sf::Vector2f mousePos(worldPos.x, worldPos.y);
        
        // Simple distance to line check
        sf::Vector2f lineVec = end - start;
        sf::Vector2f mouseVec = mousePos - start;
        
        float lineLength = std::sqrt(lineVec.x * lineVec.x + lineVec.y * lineVec.y);
        if (lineLength < 0.001f) continue;
        
        float t = (mouseVec.x * lineVec.x + mouseVec.y * lineVec.y) / (lineLength * lineLength);
        t = std::max(0.0f, std::min(1.0f, t));
        
        sf::Vector2f closestPoint = start + lineVec * t;
        sf::Vector2f diff = mousePos - closestPoint;
        float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        
        if (distance < closestDistance) {
            closestDistance = distance;
            closestBone = bone;
        }
    }
    
    return closestBone;
}

void IKSolverTool::renderOverlay(sf::RenderTarget& target, float zoomLevel) {
    if (!m_isActive || !m_character) return;
    
    renderChainHighlight(target, zoomLevel);
    renderEndEffectorMarker(target, zoomLevel);
    
    if (m_state == IKToolState::Solving) {
        renderTargetMarker(target, zoomLevel);
    }
}

void IKSolverTool::renderChainHighlight(sf::RenderTarget& target, float zoomLevel) {
    if (!m_endEffector) return;
    
    auto validation = validateCurrentChain();
    if (!validation.isValid) return;
    
    // Render chain bones with highlight
    for (const auto& bone : validation.chain) {
        float startX, startY, endX, endY;
        bone->getWorldEndpoints(startX, startY, endX, endY);
        
        // Create thicker line for chain using RectangleShape
        sf::Vector2f start(startX, startY);
        sf::Vector2f end(endX, endY);
        sf::Vector2f direction = end - start;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        
        if (length > 0.001f) {
            sf::RectangleShape chainLine;
            chainLine.setSize(sf::Vector2f(length, 4.0f / zoomLevel)); // Thickness of 4 pixels
            chainLine.setFillColor(m_chainColor);
            chainLine.setPosition(start);
            
            // Calculate rotation angle and convert to sf::Angle
            float angleRadians = std::atan2(direction.y, direction.x);
            chainLine.setRotation(sf::radians(angleRadians));
            
            target.draw(chainLine);
        }
    }
}


void IKSolverTool::renderEndEffectorMarker(sf::RenderTarget& target, float zoomLevel) {
    if (!m_endEffector) return;
    
    float startX, startY, endX, endY;
    m_endEffector->getWorldEndpoints(startX, startY, endX, endY);
    
    // Draw circle at end effector tip
    sf::CircleShape marker(8.0f / zoomLevel);
    marker.setFillColor(sf::Color::Transparent);
    marker.setOutlineColor(m_endEffectorColor);
    marker.setOutlineThickness(2.0f / zoomLevel);
    marker.setOrigin(sf::Vector2f(marker.getRadius(), marker.getRadius()));
    marker.setPosition(sf::Vector2f(endX, endY));
    
    target.draw(marker);
}

void IKSolverTool::renderTargetMarker(sf::RenderTarget& target, float zoomLevel) {
    // Draw circle at target position
    sf::CircleShape targetMarker(6.0f / zoomLevel);
    targetMarker.setFillColor(sf::Color::Transparent);
    targetMarker.setOutlineColor(m_targetColor);
    targetMarker.setOutlineThickness(2.0f / zoomLevel);
    targetMarker.setOrigin(sf::Vector2f(targetMarker.getRadius(), targetMarker.getRadius()));
    targetMarker.setPosition(sf::Vector2f(m_targetPosition.x, m_targetPosition.y));
    
    target.draw(targetMarker);
    
    // Draw crosshair lines using RectangleShape
    float crossSize = 10.0f / zoomLevel;
    float lineThickness = 2.0f / zoomLevel;
    
    // Horizontal line
    sf::RectangleShape hLine;
    hLine.setSize(sf::Vector2f(crossSize * 2, lineThickness));
    hLine.setFillColor(m_targetColor);
    hLine.setOrigin(sf::Vector2f(crossSize, lineThickness / 2));
    hLine.setPosition(sf::Vector2f(m_targetPosition.x, m_targetPosition.y));
    target.draw(hLine);
    
    // Vertical line
    sf::RectangleShape vLine;
    vLine.setSize(sf::Vector2f(lineThickness, crossSize * 2));
    vLine.setFillColor(m_targetColor);
    vLine.setOrigin(sf::Vector2f(lineThickness / 2, crossSize));
    vLine.setPosition(sf::Vector2f(m_targetPosition.x, m_targetPosition.y));
    target.draw(vLine);
}

}