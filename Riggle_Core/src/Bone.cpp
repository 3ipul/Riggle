#include "Riggle/Bone.h"
#include <cmath>

namespace Riggle {

Bone::Bone(const std::string& name, float length) 
    : m_name(name) {
    m_localTransform.length = length;
}

void Bone::addChild(std::shared_ptr<Bone> child) {
    if (child) {
        m_children.push_back(child);
        child->m_parent = shared_from_this();
    }
}

void Bone::setParent(std::shared_ptr<Bone> parent) {
    m_parent = parent;
    m_worldTransformDirty = true;
}

void Bone::setLocalTransform(const Transform& transform) {
    m_localTransform = transform;
    m_worldTransformDirty = true;
    
    // Mark children as dirty too
    for (auto& child : m_children) {
        child->m_worldTransformDirty = true;
    }
}

void Bone::setLocalRotation(float rotation) {
    m_localTransform.rotation = rotation;
    m_worldTransformDirty = true;
    
    // Mark children as dirty too
    for (auto& child : m_children) {
        child->m_worldTransformDirty = true;
    }
}

Transform Bone::getWorldTransform() const {
    if (m_worldTransformDirty) {
        updateWorldTransform();
    }
    return m_worldTransform;
}

void Bone::updateWorldTransform() const {
    auto parent = m_parent.lock();
    if (parent) {
        Transform parentWorld = parent->getWorldTransform();
        
        // Calculate world position at end of parent bone
        float parentEndX = parentWorld.x + std::cos(parentWorld.rotation) * parentWorld.length;
        float parentEndY = parentWorld.y + std::sin(parentWorld.rotation) * parentWorld.length;
        
        m_worldTransform.x = parentEndX;
        m_worldTransform.y = parentEndY;
        m_worldTransform.rotation = parentWorld.rotation + m_localTransform.rotation;
        m_worldTransform.length = m_localTransform.length;
    } else {
        // Root bone
        m_worldTransform = m_localTransform;
    }
    
    m_worldTransformDirty = false;
}

void Bone::getWorldEndpoints(float& startX, float& startY, float& endX, float& endY) const {
    Transform world = getWorldTransform();
    
    startX = world.x;
    startY = world.y;
    endX = world.x + std::cos(world.rotation) * world.length;
    endY = world.y + std::sin(world.rotation) * world.length;
}

} // namespace Riggle