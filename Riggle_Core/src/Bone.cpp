#include "Riggle/Bone.h"
#include "Riggle/Sprite.h"
#include "Riggle/Character.h"
#include <algorithm>
#include <functional>
#include <cmath>

namespace Riggle {

Bone::Bone(const std::string& name, float length)
    : m_name(name)
    , m_length(length)
    , m_localTransform()
    , m_worldTransformDirty(true)
{
}

void Bone::setLocalTransform(const Transform& transform) {
    Transform oldTransform = m_localTransform;
    
    m_localTransform = transform;
    m_localTransform.length = m_length;  // Keep length consistent
    markWorldTransformDirty();
    
    // IMPORTANT: Mark ALL descendants as dirty, not just direct children
    std::function<void(std::shared_ptr<Bone>)> markDescendants = [&](std::shared_ptr<Bone> bone) {
        if (!bone) return;
        for (auto& child : bone->getChildren()) {
            child->markWorldTransformDirty();
            markDescendants(child);  // Recursive call
        }
    };
    
    markDescendants(shared_from_this());
    
    // Notify character of transform change
    notifyCharacterOfTransformChange(oldTransform, m_localTransform);
}

Transform Bone::getWorldTransform() const {
    if (m_worldTransformDirty) {
        updateWorldTransform();
    }
    return m_worldTransform;
}

void Bone::updateWorldTransform() const {
    auto parent = m_parent.lock();
    if (!parent) {
        // Root bone - world transform = local transform
        m_worldTransform = m_localTransform;
    } else {
        // Child bone - combine with parent's world transform
        Transform parentWorld = parent->getWorldTransform();
        
        // Apply parent transform to local transform
        float cosRot = std::cos(parentWorld.rotation);
        float sinRot = std::sin(parentWorld.rotation);
        
        // Transform position: rotate local position and add to parent position
        float rotatedX = m_localTransform.position.x * cosRot - m_localTransform.position.y * sinRot;
        float rotatedY = m_localTransform.position.x * sinRot + m_localTransform.position.y * cosRot;
        
        m_worldTransform.position.x = parentWorld.position.x + rotatedX * parentWorld.scale.x;
        m_worldTransform.position.y = parentWorld.position.y + rotatedY * parentWorld.scale.y;
            
        // Combine rotations
        m_worldTransform.rotation = parentWorld.rotation + m_localTransform.rotation;
        
        // Combine scales
        m_worldTransform.scale.x = parentWorld.scale.x * m_localTransform.scale.x;
        m_worldTransform.scale.y = parentWorld.scale.y * m_localTransform.scale.y;
        
        // Keep length from local transform
        m_worldTransform.length = m_localTransform.length;
    }
    
    m_worldTransformDirty = false;
}

void Bone::setLength(float length) {
    if (m_length != length) {
        Transform oldTransform = m_localTransform;
        
        m_length = length;
        m_localTransform.length = length;  // Keep transform in sync
        markWorldTransformDirty();
        
        // Mark all children as needing update
        for (auto& child : m_children) {
            child->markWorldTransformDirty();
        }
        
        // Notify character of transform change
        notifyCharacterOfTransformChange(oldTransform, m_localTransform);
    }
}

void Bone::notifyCharacterOfTransformChange(const Transform& oldTransform, const Transform& newTransform) {
    if (m_character) {
        m_character->notifyTransformChanged(m_name, oldTransform, newTransform);
    }
}

void Bone::addChild(std::shared_ptr<Bone> child) {
    if (!child) return;
    
    // Remove from current parent if any
    if (auto oldParent = child->getParent()) {
        oldParent->removeChild(child);
    }
    
    // Add to this bone
    m_children.push_back(child);
    child->setParent(shared_from_this());
    child->markWorldTransformDirty();
}

void Bone::removeChild(std::shared_ptr<Bone> child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->setParent(nullptr);
        m_children.erase(it);
    }
}

void Bone::addBoundSprite(std::weak_ptr<Sprite> sprite) {
    // Check if sprite is already bound
    auto spritePtr = sprite.lock();
    if (!spritePtr) return;
    
    for (auto it = m_boundSprites.begin(); it != m_boundSprites.end();) {
        if (it->expired()) {
            // Remove expired weak_ptr
            it = m_boundSprites.erase(it);
        } else if (it->lock() == spritePtr) {
            // Already bound
            return;
        } else {
            ++it;
        }
    }
    
    // Add new sprite
    m_boundSprites.push_back(sprite);
}

void Bone::removeBoundSprite(std::weak_ptr<Sprite> sprite) {
    auto spritePtr = sprite.lock();
    if (!spritePtr) return;
    
    for (auto it = m_boundSprites.begin(); it != m_boundSprites.end();) {
        if (it->expired() || it->lock() == spritePtr) {
            it = m_boundSprites.erase(it);
        } else {
            ++it;
        }
    }
}

bool Bone::hasSprites() const {
    size_t count = 0;
    for (const auto& sprite : m_boundSprites) {
        if (!sprite.expired()) {
            count++;
        }
    }
    return count > 0;
}

size_t Bone::getSpriteCount() const {
    size_t count = 0;
    for (const auto& sprite : m_boundSprites) {
        if (!sprite.expired()) {
            count++;
        }
    }
    return count;
}

void Bone::getWorldEndpoints(float& startX, float& startY, float& endX, float& endY) const {
    Transform world = getWorldTransform();
    
    startX = world.position.x;
    startY = world.position.y;
    
    float cosRot = std::cos(world.rotation);
    float sinRot = std::sin(world.rotation);
    
    endX = world.position.x + world.length * cosRot * world.scale.x;
    endY = world.position.y + world.length * sinRot * world.scale.y;
}

std::vector<std::shared_ptr<Bone>> Bone::getAllDescendants() const {
    std::vector<std::shared_ptr<Bone>> descendants;
    
    for (const auto& child : m_children) {
        descendants.push_back(child);
        
        auto childDescendants = child->getAllDescendants();
        descendants.insert(descendants.end(), childDescendants.begin(), childDescendants.end());
    }
    
    return descendants;
}

} // namespace Riggle