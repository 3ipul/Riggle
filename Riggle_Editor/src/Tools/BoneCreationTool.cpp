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
    
    if (isValidBoneLength()) {
        createBone();
    } else {
        std::cout << "Bone too short, minimum length: " << m_minBoneLength << std::endl;
    }
    
    m_state = BoneCreationState::Idle;
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

    // Show auto-binding preview when Ctrl is held
    if (m_autoBindingEnabled && m_state == BoneCreationState::Creating) {
        bool ctrlPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || 
                           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
        
        if (ctrlPressed) {
            Sprite* targetSprite = findSpriteAtPosition(m_startPosition);
            if (targetSprite) {
                // Highlight the sprite that would be bound
                Transform spriteTransform = targetSprite->getWorldTransform();
                sf::Vector2f spritePos(spriteTransform.position.x, spriteTransform.position.y);
                
                // Draw binding preview
                sf::CircleShape bindingIndicator(20.0f / zoomLevel);
                bindingIndicator.setFillColor(sf::Color::Transparent);
                bindingIndicator.setOutlineThickness(3.0f / zoomLevel);
                bindingIndicator.setOutlineColor(sf::Color(0, 255, 255, 200)); // Cyan
                bindingIndicator.setOrigin(sf::Vector2f(20.0f / zoomLevel, 20.0f / zoomLevel));
                bindingIndicator.setPosition(spritePos);
                
                target.draw(bindingIndicator);
            }
        }
    }
}

void BoneCreationTool::createBone() {
    if (!m_character) {
        std::cout << "Error: No character set for bone creation" << std::endl;
        return;
    }
    
    // Ensure character has a rig
    if (!m_character->getRig()) {
        auto newRig = std::make_unique<Rig>("CharacterRig");
        m_character->setRig(std::move(newRig));
        std::cout << "Created new rig for character" << std::endl;
    }
    
    float length = calculateBoneLength();
    Sprite* targetSprite = nullptr;
    std::string boneName;
    
    // Check for auto-binding with Ctrl key
    bool ctrlPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || 
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
    
    if (m_autoBindingEnabled && ctrlPressed) {
        // Approach 1: Auto-detect sprite behind bone start position
        targetSprite = findSpriteAtPosition(m_startPosition);
        if (targetSprite) {
            boneName = generateBoneNameForSprite(targetSprite);
            std::cout << "Auto-detected sprite for binding: " << targetSprite->getName() << std::endl;
        }
    }
    
    // If no auto-binding name was set, use normal naming
    if (boneName.empty()) {
        boneName = generateBoneName();
    }
    
    // Create bone with calculated name
    std::shared_ptr<Bone> newBone;
    if (!hasRootBone()) {
        newBone = m_character->getRig()->createBone(boneName, length);
    } else if (m_selectedBone) {
        newBone = m_character->getRig()->createChildBone(m_selectedBone, boneName, length);
    } else {
        newBone = m_character->getRig()->createBone(boneName, length);
    }
    
    if (!newBone) {
        std::cout << "Failed to create bone!" << std::endl;
        return;
    }
    
    // Set bone transform
    Transform boneTransform;
    
    if (m_selectedBone && newBone->getParent()) {
        // Child bone - calculate local transform relative to parent
        Transform parentWorld = m_selectedBone->getWorldTransform();
        
        // Calculate world direction vector for the new bone
        sf::Vector2f worldDirection = m_endPosition - m_startPosition;
        float worldRotation = std::atan2(worldDirection.y, worldDirection.x);
        
        // Convert world position to local position relative to parent
        sf::Vector2f worldOffset = m_startPosition - sf::Vector2f(parentWorld.position.x, parentWorld.position.y);
        
        // Rotate offset by negative parent rotation to get local coordinates
        float cosParentRot = std::cos(-parentWorld.rotation);
        float sinParentRot = std::sin(-parentWorld.rotation);
        
        boneTransform.position.x = (worldOffset.x * cosParentRot - worldOffset.y * sinParentRot) / parentWorld.scale.x;
        boneTransform.position.y = (worldOffset.x * sinParentRot + worldOffset.y * cosParentRot) / parentWorld.scale.y;
        
        // Local rotation = world rotation - parent rotation
        boneTransform.rotation = worldRotation - parentWorld.rotation;
        
        // Normalize rotation to [-PI, PI]
        while (boneTransform.rotation > 3.14159f) boneTransform.rotation -= 2.0f * 3.14159f;
        while (boneTransform.rotation < -3.14159f) boneTransform.rotation += 2.0f * 3.14159f;
        
    } else {
        // Root or independent bone - use world coordinates directly
        boneTransform.position.x = m_startPosition.x;
        boneTransform.position.y = m_startPosition.y;
        
        // Calculate rotation from start to end
        sf::Vector2f direction = m_endPosition - m_startPosition;
        boneTransform.rotation = std::atan2(direction.y, direction.x);
    }
    
    // Set scale (always 1.0 for now)
    boneTransform.scale.x = 1.0f;
    boneTransform.scale.y = 1.0f;
    
    // Set length in transform (for consistency)
    boneTransform.length = length;
    
    newBone->setLocalTransform(boneTransform);
    
    // Force update transforms
    m_character->getRig()->forceUpdateWorldTransforms();

    // Perform auto-binding if sprite was detected 
    if (targetSprite && ctrlPressed) {
        bindBoneToSprite(newBone, targetSprite);
    }
    
    // Auto-select the newly created bone
    m_selectedBone = newBone;
    
    std::cout << "Created bone: " << boneName 
              << " (length: " << length 
              << ", local rotation: " << (boneTransform.rotation * 180.0f / 3.14159f) << "°)" << std::endl;

    // Notify callbacks
    if (m_onBoneCreated) {
        m_onBoneCreated(newBone);
    }
    
    if (m_onBoneSelected) {
        m_onBoneSelected(newBone);
    }
}

Sprite* BoneCreationTool::findSpriteAtPosition(const sf::Vector2f& position) {
    if (!m_character) return nullptr;
    
    const auto& sprites = m_character->getSprites();
    for (auto it = sprites.rbegin(); it != sprites.rend(); ++it) {
        const auto& sprite = *it;
        if (!sprite->isVisible() || sprite->isBoundToBone()) continue;
        
        // Simple bounding box check - reuse viewport logic
        Transform worldTransform = sprite->getWorldTransform();
        
        // Use default sprite size for now (you can enhance this later)
        float halfWidth = 32.0f * worldTransform.scale.x;
        float halfHeight = 32.0f * worldTransform.scale.y;
        
        float left = worldTransform.position.x - halfWidth;
        float right = worldTransform.position.x + halfWidth;
        float top = worldTransform.position.y - halfHeight;
        float bottom = worldTransform.position.y + halfHeight;
        
        if (position.x >= left && position.x <= right &&
            position.y >= top && position.y <= bottom) {
            return sprite.get();
        }
    }
    return nullptr;
}

void BoneCreationTool::bindBoneToSprite(std::shared_ptr<Bone> bone, Sprite* sprite) {
    if (!bone || !sprite) return;
    
    // Calculate binding offset
    Transform boneWorld = bone->getWorldTransform();
    Transform spriteWorld = sprite->getWorldTransform();
    
    Vector2 offset;
    offset.x = spriteWorld.position.x - boneWorld.position.x;
    offset.y = spriteWorld.position.y - boneWorld.position.y;
    
    float rotationOffset = spriteWorld.rotation - boneWorld.rotation;
    
    sprite->bindToBone(bone, offset, rotationOffset);
    
    std::cout << "Auto-bound sprite '" << sprite->getName() 
              << "' to bone '" << bone->getName() << "'" << std::endl;
    
    if (m_onBoneSpriteBound) {
        m_onBoneSpriteBound(bone, sprite);
    }
}

std::string BoneCreationTool::generateBoneNameForSprite(Sprite* sprite) {
    if (!sprite) return generateBoneName();
    
    std::string spriteName = sprite->getName();
    if (spriteName.empty()) {
        spriteName = "Sprite";
    }
    
    return spriteName + "_bone";
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