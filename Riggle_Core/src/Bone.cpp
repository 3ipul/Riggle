#include "Riggle/Bone.h"
#include <algorithm>
#include <cmath>

namespace Riggle {

Bone::Bone(const std::string& name, float length)
    : m_name(name)
{
    m_localTransform.length = length;
}

void Bone::addChild(std::shared_ptr<Bone> child) {
    if (child && child.get() != this) {
        // Remove from previous parent if any
        auto currentParent = child->getParent();
        if (currentParent) {
            auto& siblings = currentParent->m_children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
        }
        
        // Add to this bone
        child->m_parent = shared_from_this();
        m_children.push_back(child);
        child->m_worldTransformDirty = true;
    }
}

void Bone::setParent(std::shared_ptr<Bone> parent) {
    if (parent) {
        parent->addChild(shared_from_this());
    } else {
        // Remove from current parent
        auto currentParent = getParent();
        if (currentParent) {
            auto& siblings = currentParent->m_children;
            auto self = shared_from_this();
            siblings.erase(std::remove(siblings.begin(), siblings.end(), self), siblings.end());
        }
        m_parent.reset();
        m_worldTransformDirty = true;
    }
}

void Bone::setLocalTransform(const Transform& transform) {
    m_localTransform = transform;
    m_worldTransformDirty = true;
    
    // Mark all children as dirty too
    for (auto& child : m_children) {
        child->m_worldTransformDirty = true;
    }
}

void Bone::setLocalRotation(float rotation) {
    m_localTransform.rotation = rotation;
    m_worldTransformDirty = true;
    
    // Mark all children as dirty too
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

void Bone::getWorldEndpoints(float& startX, float& startY, float& endX, float& endY) const {
    Transform world = getWorldTransform();
    
    startX = world.x;
    startY = world.y;
    
    // Calculate end position based on world transform
    float cosRot = std::cos(world.rotation);
    float sinRot = std::sin(world.rotation);
    
    endX = startX + world.length * cosRot * world.scaleX;
    endY = startY + world.length * sinRot * world.scaleY;
}

void Bone::updateWorldTransform() const {
    auto parent = m_parent.lock();
    
    if (!parent) {
        // Root bone - world transform equals local transform
        m_worldTransform = m_localTransform;
    } else {
        // Get parent's world transform
        Transform parentWorld = parent->getWorldTransform();
        
        // Apply parent's transform to this bone's local transform
        float cosRot = std::cos(parentWorld.rotation);
        float sinRot = std::sin(parentWorld.rotation);
        
        // Transform position
        m_worldTransform.x = parentWorld.x + 
            (m_localTransform.x * cosRot - m_localTransform.y * sinRot) * parentWorld.scaleX;
        m_worldTransform.y = parentWorld.y + 
            (m_localTransform.x * sinRot + m_localTransform.y * cosRot) * parentWorld.scaleY;
        
        // Transform rotation
        m_worldTransform.rotation = parentWorld.rotation + m_localTransform.rotation;
        
        // Transform scale
        m_worldTransform.scaleX = parentWorld.scaleX * m_localTransform.scaleX;
        m_worldTransform.scaleY = parentWorld.scaleY * m_localTransform.scaleY;
        
        // Length stays the same
        m_worldTransform.length = m_localTransform.length;
    }
    
    m_worldTransformDirty = false;
}

void Bone::markWorldTransformDirty() {
    m_worldTransformDirty = true;
    
    // Mark all children as dirty too
    for (auto& child : m_children) {
        if (child) {
            child->markWorldTransformDirty();
        }
    }
}

} // namespace Riggle