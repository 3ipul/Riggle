#include "Editor/Tools/BoneCreationTool.h"
#include <iostream>
#include <cmath>
#include <sstream>

namespace Riggle {

BoneCreationTool::BoneCreationTool()
    : m_character(nullptr)
    , m_isActive(false)
    , m_state(BoneCreationState::Idle)
    , m_selectedBone(nullptr)
    , m_minBoneLength(20.0f)
    , m_snapToGrid(false)
    , m_gridSize(10.0f)
    , m_previewColor(255, 255, 0, 150)        // Yellow preview
    , m_validColor(0, 255, 0, 200)            // Green when valid
    , m_invalidColor(255, 0, 0, 200)          // Red when invalid
    , m_selectedColor(255, 100, 100, 255)     // Red for selected
{
}

void BoneCreationTool::setActive(bool active) {
    if (m_isActive == active) return;
    
    m_isActive = active;
    
    if (!active) {
        m_state = BoneCreationState::Idle;
    }
    
    std::cout << "BoneCreationTool " << (active ? "activated" : "deactivated") << std::endl;
}

void BoneCreationTool::handleMousePressed(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_character) return;
    
    // Check if clicking on existing bone to select it
    auto clickedBone = findBoneAtPosition(worldPos);
    if (clickedBone) {
        m_selectedBone = clickedBone;
        if (m_onBoneSelected) {
            m_onBoneSelected(m_selectedBone);
        }
        std::cout << "Selected bone: " << clickedBone->getName() << std::endl;
        return;
    }
    
    // Start creating new bone
    m_startPosition = m_snapToGrid ? snapToGrid(worldPos) : worldPos;
    m_endPosition = m_startPosition;
    m_state = BoneCreationState::Creating;
    
    std::cout << "Started bone creation at (" << m_startPosition.x << ", " << m_startPosition.y << ")" << std::endl;
}

void BoneCreationTool::handleMouseMoved(const sf::Vector2f& worldPos) {
    if (!m_isActive || m_state != BoneCreationState::Creating) return;
    
    m_endPosition = m_snapToGrid ? snapToGrid(worldPos) : worldPos;
}

void BoneCreationTool::handleMouseReleased(const sf::Vector2f& worldPos) {
    if (!m_isActive || m_state != BoneCreationState::Creating) return;
    
    m_endPosition = m_snapToGrid ? snapToGrid(worldPos) : worldPos;
    
    // CRITICAL: Reset state FIRST to prevent re-entry
    BoneCreationState oldState = m_state;
    m_state = BoneCreationState::Idle;
    
    // Only create bone if we were actually creating one
    if (oldState == BoneCreationState::Creating && isValidBoneLength()) {
        createBone();
    }
}

std::shared_ptr<Bone> BoneCreationTool::findBoneAtPosition(const sf::Vector2f& worldPos) {
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

void BoneCreationTool::renderOverlay(sf::RenderTarget& target, float zoomLevel) {
    if (!m_isActive || !m_character) return;
    
    // Render selected bone highlight
    if (m_selectedBone) {
        renderSelectedBoneHighlight(target, zoomLevel);
    }
    
    // Render bone creation preview
    if (m_state == BoneCreationState::Creating) {
        renderBonePreview(target, zoomLevel);
    }
}

void BoneCreationTool::createBone() {
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

float BoneCreationTool::calculateBoneLength() const {
    sf::Vector2f diff = m_endPosition - m_startPosition;
    return std::sqrt(diff.x * diff.x + diff.y * diff.y);
}

bool BoneCreationTool::isValidBoneLength() const {
    return calculateBoneLength() >= m_minBoneLength;
}

sf::Vector2f BoneCreationTool::snapToGrid(const sf::Vector2f& position) const {
    return sf::Vector2f(
        std::round(position.x / m_gridSize) * m_gridSize,
        std::round(position.y / m_gridSize) * m_gridSize
    );
}

std::string BoneCreationTool::generateBoneName() const {
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

bool BoneCreationTool::hasRootBone() const {
    if (!m_character || !m_character->getRig()) return false;
    return !m_character->getRig()->getRootBones().empty();
}

void BoneCreationTool::renderBonePreview(sf::RenderTarget& target, float zoomLevel) {
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

void BoneCreationTool::renderSelectedBoneHighlight(sf::RenderTarget& target, float zoomLevel) {
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
    startJoint.setFillColor(m_selectedColor);
    startJoint.setOrigin(sf::Vector2f(startJointSize, startJointSize));
    startJoint.setPosition(start);
    target.draw(startJoint);
    
    float endJointSize = 6.0f / zoomLevel;
    sf::CircleShape endJoint(endJointSize);
    endJoint.setFillColor(m_selectedColor);
    endJoint.setOrigin(sf::Vector2f(endJointSize, endJointSize));
    endJoint.setPosition(end);
    target.draw(endJoint);
}

} // namespace Riggle