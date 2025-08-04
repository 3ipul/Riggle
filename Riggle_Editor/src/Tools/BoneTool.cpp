#include "Editor/Tools/BoneTool.h"
#include "Editor/Tools/IKSolverTool.h"
#include <iostream>
#include <cmath>
#include <sstream>

namespace Riggle {

BoneTool::BoneTool()
    : m_character(nullptr)
    , m_isActive(false)
    , m_currentSubTool(BoneSubTool::BoneTransform)
    , m_state(BoneCreationState::Idle)
    , m_selectedBone(nullptr)
    , m_minBoneLength(20.0f)
    , m_snapToGrid(false)
    , m_gridSize(10.0f)
    , m_previewColor(255, 255, 0, 150)
    , m_validColor(0, 255, 0, 200)
    , m_invalidColor(255, 0, 0, 200)
    , m_selectedColor(255, 100, 100, 255)
    , m_jointColor(0, 255, 0, 255)
{
}

void BoneTool::setActive(bool active) {
    if (m_isActive == active) return;
    
    m_isActive = active;
    
    if (!active) {
        m_state = BoneCreationState::Idle;
        // Deactivate IK tool if it was active
        if (m_ikTool) {
            m_ikTool->setActive(false);
        }
    } else {
        // Activate sub-tool when bone tool becomes active
        setSubTool(m_currentSubTool);
    }
    
    std::cout << "BoneTool " << (active ? "activated" : "deactivated") << std::endl;
}

void BoneTool::setSubTool(BoneSubTool subTool) {
    if (m_currentSubTool == subTool) return;
    
    m_currentSubTool = subTool;
    
    // Reset state when switching sub-tools
    m_state = BoneCreationState::Idle;
    
    // Handle IK tool activation/deactivation
    if (m_ikTool) {
        if (subTool == BoneSubTool::IKSolver) {
            m_ikTool->setActive(true);
        } else {
            m_ikTool->setActive(false);
        }
    }
    
    std::cout << "BoneTool sub-tool changed to: " << getSubToolName() << std::endl;
}

const char* BoneTool::getSubToolName() const {
    switch (m_currentSubTool) {
        case BoneSubTool::CreateBone: return "Create Bone";
        case BoneSubTool::BoneTransform: return "Transform";
        case BoneSubTool::IKSolver: return "IK Solver";
        default: return "Unknown";
    }
}

void BoneTool::handleMousePressed(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_character) return;
    
    switch (m_currentSubTool) {
        case BoneSubTool::CreateBone:
            handleBoneCreation(worldPos);
            break;
        case BoneSubTool::BoneTransform:
            handleBoneTransform(worldPos);
            break;
        case BoneSubTool::IKSolver:
            if (m_ikTool) {
                m_ikTool->handleMousePressed(worldPos);
            }
            break;
    }
}

void BoneTool::handleMouseMoved(const sf::Vector2f& worldPos) {
    if (!m_isActive) return;
    
    switch (m_currentSubTool) {
        case BoneSubTool::CreateBone:
            if (m_state == BoneCreationState::Creating) {
                // Check for snap points during drag
                sf::Vector2f snapPos;
                std::shared_ptr<Bone> snapBone;
                bool snapToEnd;
                
                if (findNearbyBoneEndpoint(worldPos, snapPos, snapBone, snapToEnd)) {
                    m_endPosition = snapPos;
                } else {
                    m_endPosition = m_snapToGrid ? snapToGrid(worldPos) : worldPos;
                }
            }
            break;
        case BoneSubTool::BoneTransform:
            // Handle bone rotation during drag
            if (m_selectedBone) {
                handleBoneRotation(worldPos);
            }
            break;
        case BoneSubTool::IKSolver:
            if (m_ikTool) {
                m_ikTool->handleMouseMoved(worldPos);
            }
            break;
    }
}

void BoneTool::handleMouseReleased(const sf::Vector2f& worldPos) {
    if (!m_isActive) return;
    
    switch (m_currentSubTool) {
        case BoneSubTool::CreateBone:

            if (m_state == BoneCreationState::Creating) {
                // NEW: Final snap check
                sf::Vector2f snapPos;
                std::shared_ptr<Bone> snapBone;
                bool snapToEnd;
                
                if (findNearbyBoneEndpoint(worldPos, snapPos, snapBone, snapToEnd)) {
                    m_endPosition = snapPos;
                    // If snapping to end of bone, make that bone the parent
                    if (snapToEnd) {
                        m_selectedBone = snapBone;
                    }
                } else {
                    m_endPosition = m_snapToGrid ? snapToGrid(worldPos) : worldPos;
                }
                
                BoneCreationState oldState = m_state;
                m_state = BoneCreationState::Idle;
                
                if (oldState == BoneCreationState::Creating && isValidBoneLength()) {
                    createBone();
                }
            }
            break;
        case BoneSubTool::BoneTransform:
            // Nothing special needed on release for transform
            break;
        case BoneSubTool::IKSolver:
            if (m_ikTool) {
                m_ikTool->handleMouseReleased(worldPos);
            }
            break;
    }
}

void BoneTool::handleBoneCreation(const sf::Vector2f& worldPos) {
    // First check for nearby bone endpoints before checking bone selection
    sf::Vector2f snapPosition = worldPos;
    std::shared_ptr<Bone> snapTargetBone = nullptr;
    bool snapToEnd = false;
    
    bool hasSnapPoint = findNearbyBoneEndpoint(worldPos, snapPosition, snapTargetBone, snapToEnd);
    
    if (hasSnapPoint) {
        std::cout << "Starting bone creation from " << (snapToEnd ? "end" : "start") << " of bone: " << snapTargetBone->getName() << std::endl;
        
        // If snapping to end of bone, make that bone the parent for the new bone
        if (snapToEnd) {
            m_selectedBone = snapTargetBone;
        }
        
        // Start creating new bone from the snapped position
        m_startPosition = snapPosition;
        m_endPosition = m_startPosition;
        m_state = BoneCreationState::Creating;
        
        std::cout << "Started bone creation at snap point (" << m_startPosition.x << ", " << m_startPosition.y << ")" << std::endl;
        return;
    }
    
    // If no snap point, then check if clicking on existing bone to select it
    auto clickedBone = findBoneAtPosition(worldPos);
    if (clickedBone) {
        m_selectedBone = clickedBone;
        if (m_onBoneSelected) {
            m_onBoneSelected(m_selectedBone);
        }
        std::cout << "Selected bone: " << clickedBone->getName() << std::endl;
        return;
    }
    
    // Start creating new bone from clicked position (no snap)
    m_startPosition = m_snapToGrid ? snapToGrid(worldPos) : worldPos;
    m_endPosition = m_startPosition;
    m_state = BoneCreationState::Creating;
    
    std::cout << "Started bone creation at (" << m_startPosition.x << ", " << m_startPosition.y << ")" << std::endl;
}

bool BoneTool::findNearbyBoneEndpoint(const sf::Vector2f& worldPos, sf::Vector2f& snapPosition, 
                                     std::shared_ptr<Bone>& snapBone, bool& snapToEnd) {
    if (!m_character || !m_character->getRig()) return false;
    
    const auto& allBones = m_character->getRig()->getAllBones();
    const float snapRadius = 15.0f; // Snap distance threshold
    
    float closestDistance = snapRadius + 1.0f;
    bool found = false;
    
    for (const auto& bone : allBones) {
        float startX, startY, endX, endY;
        bone->getWorldEndpoints(startX, startY, endX, endY);
        
        sf::Vector2f boneStart(startX, startY);
        sf::Vector2f boneEnd(endX, endY);
        
        // Check distance to start point
        sf::Vector2f toStart = worldPos - boneStart;
        float distanceToStart = std::sqrt(toStart.x * toStart.x + toStart.y * toStart.y);
        
        if (distanceToStart < closestDistance) {
            closestDistance = distanceToStart;
            snapPosition = boneStart;
            snapBone = bone;
            snapToEnd = false;
            found = true;
        }
        
        // Check distance to end point
        sf::Vector2f toEnd = worldPos - boneEnd;
        float distanceToEnd = std::sqrt(toEnd.x * toEnd.x + toEnd.y * toEnd.y);
        
        if (distanceToEnd < closestDistance) {
            closestDistance = distanceToEnd;
            snapPosition = boneEnd;
            snapBone = bone;
            snapToEnd = true;
            found = true;
        }
    }
    
    return found;
}

void BoneTool::handleBoneTransform(const sf::Vector2f& worldPos) {
    auto bone = findBoneAtPosition(worldPos);
    if (bone != m_selectedBone) {
        m_selectedBone = bone;
        if (m_onBoneSelected) {
            m_onBoneSelected(bone);
        }
    }
}

void BoneTool::handleBoneRotation(const sf::Vector2f& worldPos) {
    if (!m_selectedBone) return;
    
    // Get bone's start position (world coordinates)
    float startX, startY, endX, endY;
    m_selectedBone->getWorldEndpoints(startX, startY, endX, endY);
    sf::Vector2f boneStart(startX, startY);
    
    // Calculate angle from bone start to mouse position
    sf::Vector2f mouseOffset = worldPos - boneStart;
    float targetAngle = std::atan2(mouseOffset.y, mouseOffset.x);
    
    // Get current transform
    Transform currentTransform = m_selectedBone->getLocalTransform();
    
    if (m_selectedBone->getParent()) {
        // Child bone - calculate relative to parent's rotation
        Transform parentWorld = m_selectedBone->getParent()->getWorldTransform();
        float localAngle = targetAngle - parentWorld.rotation;
        
        // Normalize angle to [-PI, PI]
        while (localAngle > 3.14159f) localAngle -= 2.0f * 3.14159f;
        while (localAngle < -3.14159f) localAngle += 2.0f * 3.14159f;
        
        currentTransform.rotation = localAngle;
    } else {
        // Root bone - use world angle directly
        currentTransform.rotation = targetAngle;
    }
    
    m_selectedBone->setLocalTransform(currentTransform);
    
    // Force update world transforms
    if (m_character && m_character->getRig()) {
        m_character->getRig()->forceUpdateWorldTransforms();
    }

    // Notify callbacks
    if (m_onBoneRotated) {
        m_onBoneRotated(m_selectedBone, currentTransform.rotation);
    }
    if (m_onBoneTransformed) {
        m_onBoneTransformed(m_selectedBone->getName());
    }
}

void BoneTool::renderOverlay(sf::RenderTarget& target, float zoomLevel) {
    if (!m_isActive || !m_character) return;
    
    // Render selected bone highlight (common to all sub-tools)
    if (m_selectedBone) {
        renderSelectedBoneHighlight(target, zoomLevel);
    }
    
    // Render sub-tool specific overlays
    switch (m_currentSubTool) {
        case BoneSubTool::CreateBone:
            if (m_state == BoneCreationState::Creating) {
                renderBonePreview(target, zoomLevel);
            }
            break;
        case BoneSubTool::BoneTransform:
            // No additional overlay for transform (selection highlight is enough)
            break;
        case BoneSubTool::IKSolver:
            renderIKOverlay(target, zoomLevel);
            break;
    }
}

void BoneTool::renderIKOverlay(sf::RenderTarget& target, float zoomLevel) {
    if (m_ikTool && m_ikTool->isActive()) {
        m_ikTool->renderOverlay(target, zoomLevel);
    }
}


std::shared_ptr<Bone> BoneTool::findBoneAtPosition(const sf::Vector2f& worldPos) {
    if (!m_character || !m_character->getRig()) return nullptr;
    
    const auto& allBones = m_character->getRig()->getAllBones();
    const float hitRadius = 10.0f; // Hit detection radius
    
    for (const auto& bone : allBones) {
        float startX, startY, endX, endY;
        bone->getWorldEndpoints(startX, startY, endX, endY);
        
        // Check if click is near bone line
        sf::Vector2f boneStart(startX, startY);
        sf::Vector2f boneEnd(endX, endY);
        sf::Vector2f mousePos(worldPos.x, worldPos.y);
        
        // Distance from point to line segment
        sf::Vector2f line = boneEnd - boneStart;
        sf::Vector2f toMouse = mousePos - boneStart;
        
        float lineLength = std::sqrt(line.x * line.x + line.y * line.y);
        if (lineLength < 0.1f) continue; // Avoid division by zero
        
        float t = std::max(0.0f, std::min(1.0f, 
            (toMouse.x * line.x + toMouse.y * line.y) / (lineLength * lineLength)));
        
        sf::Vector2f closestPoint = boneStart + t * line;
        sf::Vector2f diff = mousePos - closestPoint;
        float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        
        if (distance <= hitRadius) {
            return bone;
        }
    }
    
    return nullptr;
}

void BoneTool::createBone() {
    if (!m_character) return;
    
    if (!m_character->getRig()) {
        auto newRig = std::make_unique<Rig>("CharacterRig");
        m_character->setRig(std::move(newRig));
    }
    
    float length = calculateBoneLength();
    std::string boneName = generateBoneName();
    
    // Create bone
    std::shared_ptr<Bone> newBone;
    if (!hasRootBone()) {
        newBone = m_character->getRig()->createBone(boneName, length);
    } else if (m_selectedBone) {
        newBone = m_character->getRig()->createChildBone(m_selectedBone, boneName, length);
    } else {
        newBone = m_character->getRig()->createBone(boneName, length);
    }
    
    if (!newBone) return;
    
    // Set bone transform
    Transform boneTransform;
    
    if (m_selectedBone && newBone->getParent()) {
        Transform parentWorld = m_selectedBone->getWorldTransform();
        sf::Vector2f worldDirection = m_endPosition - m_startPosition;
        float worldRotation = std::atan2(worldDirection.y, worldDirection.x);
        sf::Vector2f worldOffset = m_startPosition - sf::Vector2f(parentWorld.position.x, parentWorld.position.y);
        
        float cosParentRot = std::cos(-parentWorld.rotation);
        float sinParentRot = std::sin(-parentWorld.rotation);
        
        boneTransform.position.x = (worldOffset.x * cosParentRot - worldOffset.y * sinParentRot) / parentWorld.scale.x;
        boneTransform.position.y = (worldOffset.x * sinParentRot + worldOffset.y * cosParentRot) / parentWorld.scale.y;
        boneTransform.rotation = worldRotation - parentWorld.rotation;
        
        while (boneTransform.rotation > 3.14159f) boneTransform.rotation -= 2.0f * 3.14159f;
        while (boneTransform.rotation < -3.14159f) boneTransform.rotation += 2.0f * 3.14159f;
    } else {
        boneTransform.position.x = m_startPosition.x;
        boneTransform.position.y = m_startPosition.y;
        sf::Vector2f direction = m_endPosition - m_startPosition;
        boneTransform.rotation = std::atan2(direction.y, direction.x);
    }
    
    boneTransform.scale.x = 1.0f;
    boneTransform.scale.y = 1.0f;
    boneTransform.length = length;

    newBone->setLocalTransform(boneTransform);
    m_character->getRig()->forceUpdateWorldTransforms();
    
    // Set as selected
    m_selectedBone = newBone;
    
    // Call callbacks
    if (m_onBoneCreated) {
        m_onBoneCreated(newBone);
    }
    if (m_onBoneSelected) {
        m_onBoneSelected(newBone);
    }
}

float BoneTool::calculateBoneLength() const {
    sf::Vector2f diff = m_endPosition - m_startPosition;
    return std::sqrt(diff.x * diff.x + diff.y * diff.y);
}

bool BoneTool::isValidBoneLength() const {
    return calculateBoneLength() >= m_minBoneLength;
}

sf::Vector2f BoneTool::snapToGrid(const sf::Vector2f& position) const {
    return sf::Vector2f(
        std::round(position.x / m_gridSize) * m_gridSize,
        std::round(position.y / m_gridSize) * m_gridSize
    );
}

std::string BoneTool::generateBoneName() const {
    if (!m_character || !m_character->getRig()) return "Bone_1";
    
    int boneCount = static_cast<int>(m_character->getRig()->getAllBones().size());
    std::ostringstream oss;
    
    if (!hasRootBone()) {
        oss << "Root";
    } else {
        oss << "Bone_" << (boneCount + 1);
    }
    
    return oss.str();
}

bool BoneTool::hasRootBone() const {
    if (!m_character || !m_character->getRig()) return false;
    return !m_character->getRig()->getRootBones().empty();
}

void BoneTool::renderBonePreview(sf::RenderTarget& target, float zoomLevel) {
    sf::Color color = isValidBoneLength() ? m_validColor : m_invalidColor;
    
    // Calculate direction and create triangle preview
    sf::Vector2f direction = m_endPosition - m_startPosition;
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    if (length > 0.1f) {
        direction /= length;
        sf::Vector2f perpendicular(-direction.y, direction.x);
        
        // Adjust thickness for zoom level
        float thickness = 8.0f / zoomLevel;
        float baseWidth = thickness;
        float tipWidth = thickness * 0.2f;
        
        sf::Vector2f baseLeft = m_startPosition + perpendicular * (baseWidth * 0.5f);
        sf::Vector2f baseRight = m_startPosition - perpendicular * (baseWidth * 0.5f);
        sf::Vector2f tipLeft = m_endPosition + perpendicular * (tipWidth * 0.5f);
        sf::Vector2f tipRight = m_endPosition - perpendicular * (tipWidth * 0.5f);
        
        // Draw preview triangle
        std::array<sf::Vertex, 6> vertices;
        vertices[0].position = baseLeft;
        vertices[0].color = color;
        vertices[1].position = baseRight;
        vertices[1].color = color;
        vertices[2].position = tipLeft;
        vertices[2].color = color;
        vertices[3].position = baseRight;
        vertices[3].color = color;
        vertices[4].position = tipRight;
        vertices[4].color = color;
        vertices[5].position = tipLeft;
        vertices[5].color = color;
        
        target.draw(vertices.data(), 6, sf::PrimitiveType::Triangles);
    }
    
    // Draw start joint (larger) with zoom adjustment
    float startJointSize = 8.0f / zoomLevel;
    sf::CircleShape startJoint(startJointSize);
    startJoint.setFillColor(color);
    startJoint.setOrigin(sf::Vector2f(startJointSize, startJointSize));
    startJoint.setPosition(m_startPosition);
    target.draw(startJoint);
    
    // Draw end joint (smaller) with zoom adjustment
    float endJointSize = 4.0f / zoomLevel;
    sf::CircleShape endJoint(endJointSize);
    endJoint.setFillColor(color);
    endJoint.setOrigin(sf::Vector2f(endJointSize, endJointSize));
    endJoint.setPosition(m_endPosition);
    target.draw(endJoint);
}

void BoneTool::renderSelectedBoneHighlight(sf::RenderTarget& target, float zoomLevel) {
    if (!m_selectedBone) return;
    
    float startX, startY, endX, endY;
    m_selectedBone->getWorldEndpoints(startX, startY, endX, endY);
    
    sf::Vector2f start(startX, startY);
    sf::Vector2f end(endX, endY);
    
    // Create highlighted triangle with zoom adjustment
    sf::Vector2f direction = end - start;
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    if (length > 0.1f) {
        direction /= length;
        sf::Vector2f perpendicular(-direction.y, direction.x);
        
        // Adjust thickness for zoom level
        float thickness = 12.0f / zoomLevel; // Thicker for highlight
        float baseWidth = thickness;
        float tipWidth = thickness * 0.2f;
        
        sf::Vector2f baseLeft = start + perpendicular * (baseWidth * 0.5f);
        sf::Vector2f baseRight = start - perpendicular * (baseWidth * 0.5f);
        sf::Vector2f tipLeft = end + perpendicular * (tipWidth * 0.5f);
        sf::Vector2f tipRight = end - perpendicular * (tipWidth * 0.5f);
        
        std::array<sf::Vertex, 6> vertices;
        vertices[0].position = baseLeft;
        vertices[0].color = m_selectedColor;
        vertices[1].position = baseRight;
        vertices[1].color = m_selectedColor;
        vertices[2].position = tipLeft;
        vertices[2].color = m_selectedColor;
        vertices[3].position = baseRight;
        vertices[3].color = m_selectedColor;
        vertices[4].position = tipRight;
        vertices[4].color = m_selectedColor;
        vertices[5].position = tipLeft;
        vertices[5].color = m_selectedColor;
        
        target.draw(vertices.data(), 6, sf::PrimitiveType::Triangles);
    }
    
    // Draw larger joints for selected bone with zoom adjustment
    float startJointSize = 10.0f / zoomLevel;
    sf::CircleShape startJoint(startJointSize);
    startJoint.setFillColor(m_jointColor);
    startJoint.setOrigin(sf::Vector2f(startJointSize, startJointSize));
    startJoint.setPosition(start);
    target.draw(startJoint);
    
    float endJointSize = 6.0f / zoomLevel;
    sf::CircleShape endJoint(endJointSize);
    endJoint.setFillColor(m_jointColor);
    endJoint.setOrigin(sf::Vector2f(endJointSize, endJointSize));
    endJoint.setPosition(end);
    target.draw(endJoint);
}

} // namespace Riggle