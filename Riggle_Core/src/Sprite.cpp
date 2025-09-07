#include "Riggle/Sprite.h"
#include "Riggle/Bone.h"
#include <cmath>

namespace Riggle {

Sprite::Sprite(const std::string& name, const std::string& texturePath)
    : m_name(name)
    , m_texturePath(texturePath)
    , m_isVisible(true)
    , m_localTransform()
    , m_binding{nullptr, 1.0f, {0, 0}, 0.0f}  // Initialize empty binding
{
}

Transform Sprite::getWorldTransform() const {
    if (isBoundToBone()) {
        // Calculate transform based on bone
        Transform boneWorld = m_binding.bone->getWorldTransform();
        
        // Apply binding offset and rotation
        Transform result;
        
        // Position calculation using proper offset transformation
        float cosRot = std::cos(boneWorld.rotation);
        float sinRot = std::sin(boneWorld.rotation);
        
        // Transform binding offset by bone's rotation and scale
        float rotatedOffsetX = m_binding.bindOffset.x * cosRot - m_binding.bindOffset.y * sinRot;
        float rotatedOffsetY = m_binding.bindOffset.x * sinRot + m_binding.bindOffset.y * cosRot;
        
        // Apply bone scale to the rotated offset
        result.position.x = boneWorld.position.x + (rotatedOffsetX * boneWorld.scale.x);
        result.position.y = boneWorld.position.y + (rotatedOffsetY * boneWorld.scale.y);
        
        // Rotation: bone rotation + binding rotation
        result.rotation = boneWorld.rotation + m_binding.bindRotation;
        
        // Scale: combine bone and sprite scale
        result.scale.x = boneWorld.scale.x * m_localTransform.scale.x;
        result.scale.y = boneWorld.scale.y * m_localTransform.scale.y;
        
        // Length (if applicable)
        result.length = m_localTransform.length;
        
        return result;
    } else {
        // Use local transform as world transform
        return m_localTransform;
    }
}

void Sprite::bindToBone(std::shared_ptr<Bone> bone, const Vector2& offset, float rotation) {
    if (!bone)
        return;
    
    // Unbind from previous bone if any
    if (m_binding.bone)
        unbindFromBone();
    
    // Set new binding
    m_binding.bone = bone;
    m_binding.weight = 1.0f;  // Always full weight for single binding

    // The provided offset and rotation are in world space. We need to convert
    // them to be relative to the bone's world transform.
    Transform boneWorld = bone->getWorldTransform();
    float cosRot = std::cos(-boneWorld.rotation);
    float sinRot = std::sin(-boneWorld.rotation);

    // Rotate the world offset to the bone's local space
    m_binding.bindOffset.x = (offset.x * cosRot - offset.y * sinRot) / boneWorld.scale.x;
    m_binding.bindOffset.y = (offset.x * sinRot + offset.y * cosRot) / boneWorld.scale.y;
    
    // Rotation is relative to the bone's rotation
    m_binding.bindRotation = rotation;
    
    // Add this sprite to the bone's sprite list
    bone->addBoundSprite(shared_from_this());
}

void Sprite::unbindFromBone() {
    if (!m_binding.bone) return;
    
    // Remove this sprite from the bone's list
    m_binding.bone->removeBoundSprite(shared_from_this());
    
    // Clear binding
    m_binding.bone = nullptr;
    m_binding.weight = 0.0f;
    m_binding.bindOffset = {0, 0};
    m_binding.bindRotation = 0.0f;
}

void Sprite::restoreBinding(std::shared_ptr<Bone> bone, const Vector2& localOffset, float localRotation) {
    if (!bone) return;
    if (m_binding.bone) unbindFromBone();

    m_binding.bone = bone;
    m_binding.weight = 1.0f;
    m_binding.bindOffset = localOffset;
    m_binding.bindRotation = localRotation;
    bone->addBoundSprite(shared_from_this());
}

} // namespace Riggle