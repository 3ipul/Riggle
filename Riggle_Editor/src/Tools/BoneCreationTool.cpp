#include "Editor/Tools/BoneCreationTool.h"
#include <cmath>
#include <iostream>

namespace Riggle {

BoneCreationTool::BoneCreationTool()
    : m_rig(nullptr)
    , m_isActive(false)
    , m_mode(BoneToolMode::Creating)
    , m_isCreating(false)
    , m_isRotating(false)
    , m_rotatingBone(nullptr)
    , m_initialRotation(0.0f)
    , m_rotationPreview(0.0f)
    , m_grabbedHandle(BoneHandle::None)
{
}

void BoneCreationTool::handleMousePress(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_rig) return;

    switch (m_mode) {
        case BoneToolMode::Creating:
            if (!m_isCreating) {
                m_isCreating = true;
                m_creationStart = worldPos;
                m_creationEnd = worldPos;
                std::cout << "Started bone creation at (" << worldPos.x << ", " << worldPos.y << ")" << std::endl;
            }
            break;
            
        case BoneToolMode::Selecting:
        case BoneToolMode::Moving: {
            auto bone = findBoneAtPosition(worldPos);
            if (bone) {
                m_selectedBone = bone;
                Transform world = bone->getWorldTransform();
                m_dragOffset = sf::Vector2f(worldPos.x - world.x, worldPos.y - world.y);
                
                if (m_onBoneSelected) {
                    m_onBoneSelected(bone);
                }
                std::cout << "Selected bone: " << bone->getName() << std::endl;
            } else {
                m_selectedBone = nullptr;
                if (m_onBoneSelected) {
                    m_onBoneSelected(nullptr);
                }
            }
            break;
        }
        
        case BoneToolMode::Rotating: {
            // Check if clicking on a bone
            auto bone = findBoneAtPosition(worldPos);
            if (bone) {
                m_selectedBone = bone;
                
                // Check which part of the bone was clicked
                BoneHandle handle = getBoneHandleAtPosition(bone, worldPos);
                
                if (handle == BoneHandle::End) {
                    // Start rotating from the end of the bone
                    m_isRotating = true;
                    m_rotatingBone = bone;
                    m_rotationStart = worldPos;
                    m_initialRotation = bone->getLocalTransform().rotation;
                    m_rotationPreview = m_initialRotation;
                    m_grabbedHandle = handle;
                    
                    std::cout << "Started rotating bone: " << bone->getName() << std::endl;
                } else {
                    // Just select the bone if clicking on shaft or start
                    if (m_onBoneSelected) {
                        m_onBoneSelected(bone);
                    }
                }
            } else {
                // Deselect if clicking empty space
                m_selectedBone = nullptr;
                if (m_onBoneSelected) {
                    m_onBoneSelected(nullptr);
                }
            }
            break;
        }
    }
}

void BoneCreationTool::handleMouseMove(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_rig) return;

    switch (m_mode) {
        case BoneToolMode::Creating:
            if (m_isCreating) {
                m_creationEnd = worldPos;
            }
            break;
            
        case BoneToolMode::Moving:
            if (m_selectedBone) {
                // Move the selected bone (existing code)
                sf::Vector2f newPos = sf::Vector2f(worldPos.x - m_dragOffset.x, worldPos.y - m_dragOffset.y);
                
                Transform newTransform = m_selectedBone->getLocalTransform();
                auto parent = m_selectedBone->getParent();
                
                if (!parent) {
                    newTransform.x = newPos.x;
                    newTransform.y = newPos.y;
                } else {
                    Transform parentWorld = parent->getWorldTransform();
                    float dx = newPos.x - parentWorld.x;
                    float dy = newPos.y - parentWorld.y;
                    
                    float cosRot = std::cos(-parentWorld.rotation);
                    float sinRot = std::sin(-parentWorld.rotation);
                    
                    newTransform.x = (dx * cosRot - dy * sinRot) / parentWorld.scaleX;
                    newTransform.y = (dx * sinRot + dy * cosRot) / parentWorld.scaleY;
                }
                
                m_selectedBone->setLocalTransform(newTransform);
            }
            break;
            
        case BoneToolMode::Rotating:
            if (m_isRotating && m_rotatingBone) {
                // Calculate rotation based on mouse movement
                Transform boneWorld = m_rotatingBone->getWorldTransform();
                sf::Vector2f boneStart(boneWorld.x, boneWorld.y);
                
                // Calculate angles
                float startAngle = calculateAngle(boneStart, m_rotationStart);
                float currentAngle = calculateAngle(boneStart, worldPos);
                float deltaAngle = angleDifference(currentAngle, startAngle);
                
                // Update rotation preview
                m_rotationPreview = m_initialRotation + deltaAngle;
                
                // Apply rotation to bone
                Transform newTransform = m_rotatingBone->getLocalTransform();
                newTransform.rotation = m_rotationPreview;
                m_rotatingBone->setLocalTransform(newTransform);

                // Force update all children
                m_rotatingBone->markWorldTransformDirty();
                
                // Force update entire rig
                if (m_rig) {
                    m_rig->forceUpdateWorldTransforms();
                }
                
                // Trigger callback for real-time updates
                if (m_onBoneRotated) {
                    m_onBoneRotated(m_rotatingBone, m_rotationPreview);
                }
            }
            break;
    }
}

void BoneCreationTool::handleMouseRelease(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_rig) return;

    switch (m_mode) {
        case BoneToolMode::Creating:
            if (m_isCreating) {
                // Existing bone creation code
                m_creationEnd = worldPos;
                
                float dx = m_creationEnd.x - m_creationStart.x;
                float dy = m_creationEnd.y - m_creationStart.y;
                float distance = std::sqrt(dx * dx + dy * dy);
                
                if (distance > 5.0f) {
                    std::string boneName = generateBoneName();
                    auto newBone = std::make_shared<Bone>(boneName, distance);
                    Transform boneTransform;
                    boneTransform.x = m_creationStart.x;
                    boneTransform.y = m_creationStart.y;
                    boneTransform.rotation = std::atan2(dy, dx);
                    boneTransform.length = distance;
                    newBone->setLocalTransform(boneTransform);
                    
                    // Auto-parenting logic (existing code)
                    auto nearbyBone = findBoneAtPosition(m_creationStart, 15.0f);
                    if (nearbyBone && nearbyBone != newBone) {
                        float startX, startY, endX, endY;
                        nearbyBone->getWorldEndpoints(startX, startY, endX, endY);
                        
                        float distToEnd = std::sqrt((m_creationStart.x - endX) * (m_creationStart.x - endX) + 
                                                  (m_creationStart.y - endY) * (m_creationStart.y - endY));
                        
                        if (distToEnd < 20.0f) {
                            auto childBone = m_rig->createChildBone(nearbyBone, boneName, distance);
                            Transform newTransform;
                            newTransform.x = nearbyBone->getLocalTransform().length;
                            newTransform.y = 0;
                            newTransform.rotation = std::atan2(dy, dx) - nearbyBone->getWorldTransform().rotation;
                            newTransform.length = distance;
                            childBone->setLocalTransform(newTransform);
                            
                            m_selectedBone = childBone;
                            if (m_onBoneCreated) m_onBoneCreated(childBone);
                            if (m_onBoneSelected) m_onBoneSelected(childBone);
                        } else {
                            auto rootBone = m_rig->createBone(boneName, distance);
                            rootBone->setLocalTransform(boneTransform);
                            m_selectedBone = rootBone;
                            if (m_onBoneCreated) m_onBoneCreated(rootBone);
                            if (m_onBoneSelected) m_onBoneSelected(rootBone);
                        }
                    } else {
                        auto rootBone = m_rig->createBone(boneName, distance);
                        rootBone->setLocalTransform(boneTransform);
                        m_selectedBone = rootBone;
                        if (m_onBoneCreated) m_onBoneCreated(rootBone);
                        if (m_onBoneSelected) m_onBoneSelected(rootBone);
                    }
                }
                
                m_isCreating = false;
            }
            break;
            
        case BoneToolMode::Rotating:
            if (m_isRotating) {
                std::cout << "Finished rotating bone: " << m_rotatingBone->getName() 
                         << " to " << (m_rotationPreview * 180.0f / 3.14159f) << " degrees" << std::endl;
                
                m_isRotating = false;
                m_rotatingBone = nullptr;
                m_grabbedHandle = BoneHandle::None;
            }
            break;
    }
}

BoneHandle BoneCreationTool::getBoneHandleAtPosition(std::shared_ptr<Bone> bone, const sf::Vector2f& worldPos, float tolerance) {
    if (!bone) return BoneHandle::None;
    
    float startX, startY, endX, endY;
    bone->getWorldEndpoints(startX, startY, endX, endY);
    
    sf::Vector2f start(startX, startY);
    sf::Vector2f end(endX, endY);
    
    // Check distance to end point (higher priority for rotation)
    float distToEnd = std::sqrt((worldPos.x - end.x) * (worldPos.x - end.x) + 
                               (worldPos.y - end.y) * (worldPos.y - end.y));
    if (distToEnd <= tolerance) {
        return BoneHandle::End;
    }
    
    // Check distance to start point
    float distToStart = std::sqrt((worldPos.x - start.x) * (worldPos.x - start.x) + 
                                 (worldPos.y - start.y) * (worldPos.y - start.y));
    if (distToStart <= tolerance) {
        return BoneHandle::Start;
    }
    
    // Check distance to bone shaft
    float distToLine = distanceToLine(worldPos, start, end);
    if (distToLine <= tolerance) {
        return BoneHandle::Shaft;
    }
    
    return BoneHandle::None;
}

std::shared_ptr<Bone> BoneCreationTool::findBoneAtPosition(const sf::Vector2f& worldPos, float tolerance) {
    if (!m_rig) return nullptr;

    const auto& allBones = m_rig->getAllBones();
    for (const auto& bone : allBones) {
        BoneHandle handle = getBoneHandleAtPosition(bone, worldPos, tolerance);
        if (handle != BoneHandle::None) {
            return bone;
        }
    }
    
    return nullptr;
}

void BoneCreationTool::cancelCurrentOperation() {
    m_isCreating = false;
    m_isRotating = false;
    m_selectedBone = nullptr;
    m_rotatingBone = nullptr;
}

float BoneCreationTool::calculateAngle(const sf::Vector2f& center, const sf::Vector2f& point) {
    return std::atan2(point.y - center.y, point.x - center.x);
}

float BoneCreationTool::angleDifference(float angle1, float angle2) {
    float diff = angle1 - angle2;
    while (diff > 3.14159f) diff -= 2.0f * 3.14159f;
    while (diff < -3.14159f) diff += 2.0f * 3.14159f;
    return diff;
}

// ... rest of existing helper functions remain the same ...

std::string BoneCreationTool::generateBoneName() {
    if (!m_rig) return "Bone";
    
    int counter = 1;
    std::string baseName = "Bone";
    std::string name = baseName;
    
    while (m_rig->findBone(name)) {
        name = baseName + std::to_string(counter++);
    }
    
    return name;
}

float BoneCreationTool::distanceToLine(const sf::Vector2f& point, const sf::Vector2f& lineStart, const sf::Vector2f& lineEnd) {
    float dx = lineEnd.x - lineStart.x;
    float dy = lineEnd.y - lineStart.y;
    float length = std::sqrt(dx * dx + dy * dy);
    
    if (length < 0.001f) {
        float px = point.x - lineStart.x;
        float py = point.y - lineStart.y;
        return std::sqrt(px * px + py * py);
    }
    
    dx /= length;
    dy /= length;
    
    float px = point.x - lineStart.x;
    float py = point.y - lineStart.y;
    
    float t = px * dx + py * dy;
    t = std::max(0.0f, std::min(length, t));
    
    float closestX = lineStart.x + t * dx;
    float closestY = lineStart.y + t * dy;
    
    float distX = point.x - closestX;
    float distY = point.y - closestY;
    return std::sqrt(distX * distX + distY * distY);
}

} // namespace Riggle