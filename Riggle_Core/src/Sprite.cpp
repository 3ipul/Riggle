#include "Riggle/Sprite.h"
#include "Riggle/Bone.h"
#include <iostream>
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
        
        // FIXED: Position calculation using Vector2
        float cosRot = std::cos(boneWorld.rotation);
        float sinRot = std::sin(boneWorld.rotation);
        
        result.position.x = boneWorld.position.x + 
            (m_binding.bindOffset.x * cosRot - m_binding.bindOffset.y * sinRot) * boneWorld.scale.x;
        result.position.y = boneWorld.position.y + 
            (m_binding.bindOffset.x * sinRot + m_binding.bindOffset.y * cosRot) * boneWorld.scale.y;
        
        // Rotation: bone rotation + binding rotation
        result.rotation = boneWorld.rotation + m_binding.bindRotation;
        
        // Scale: combine bone and sprite scale
        result.scale.x = boneWorld.scale.x * m_localTransform.scale.x;
        result.scale.y = boneWorld.scale.y * m_localTransform.scale.y;
        
        return result;
    } else {
        // Use local transform directly
        return m_localTransform;
    }
}

void Sprite::bindToBone(std::shared_ptr<Bone> bone, const Vector2& offset, float rotation) {
    if (!bone) {
        std::cout << "Warning: Attempting to bind sprite to null bone" << std::endl;
        return;
    }
    
    // Unbind from previous bone if any
    if (m_binding.bone) {
        unbindFromBone();
    }
    
    // Set new binding
    m_binding.bone = bone;
    m_binding.weight = 1.0f;  // Always full weight for single binding
    m_binding.bindOffset = offset;
    m_binding.bindRotation = rotation;
    
    // Add this sprite to the bone's sprite list
    bone->addBoundSprite(shared_from_this());
    
    std::cout << "Bound sprite '" << m_name << "' to bone '" << bone->getName() << "'" << std::endl;
}

void Sprite::unbindFromBone() {
    if (!m_binding.bone) return;
    
    // Remove this sprite from the bone's list
    m_binding.bone->removeBoundSprite(shared_from_this());
    
    std::cout << "Unbound sprite '" << m_name << "' from bone '" << m_binding.bone->getName() << "'" << std::endl;
    
    // Clear binding
    m_binding.bone = nullptr;
    m_binding.weight = 0.0f;
    m_binding.bindOffset = {0, 0};
    m_binding.bindRotation = 0.0f;
}

void Sprite::updateFromBone() {
    if (!isBoundToBone()) return;
    
    // The world transform is automatically calculated in getWorldTransform()
    // This method can be used for additional update logic if needed
}

} // namespace Riggle