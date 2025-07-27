#include "Riggle/Sprite.h"
#include "Riggle/Bone.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Riggle {

Sprite::Sprite(const std::string& name, const std::string& texturePath)
    : m_name(name)
    , m_texturePath(texturePath)
    , m_isVisible(true)
{
    // Initialize with identity transform
    m_localTransform = Transform();
}

void Sprite::setupAsQuad(float width, float height, const Vector2& pivot) {
    m_originalVertices.clear();
    
    // Use pivot to determine anchor point
    float left = -width * pivot.x;
    float right = left + width;
    float top = -height * pivot.y;
    float bottom = top + height;
    
    // Create quad vertices (counter-clockwise)
    m_originalVertices.emplace_back(left, top);     // Top-left
    m_originalVertices.emplace_back(right, top);    // Top-right
    m_originalVertices.emplace_back(right, bottom); // Bottom-right
    m_originalVertices.emplace_back(left, bottom);  // Bottom-left
    
    // Initialize deformed vertices
    m_deformedVertices = m_originalVertices;
}

Vector2 Sprite::getCurrentCenterPosition() const {
    if (m_deformedVertices.empty()) {
        return Vector2(m_localTransform.x, m_localTransform.y);
    }
    
    Vector2 center(0.0f, 0.0f);
    for (const auto& vertex : m_deformedVertices) {
        center.x += vertex.x;
        center.y += vertex.y;
    }
    center.x /= m_deformedVertices.size();
    center.y /= m_deformedVertices.size();
    
    return center;
}

void Sprite::calculateBindOffset(BoneBinding& binding) {
    if (!binding.bone) return;
    
    // Get current sprite center position
    Vector2 spriteCenter = getCurrentCenterPosition();
    
    // Get bone's start position (where it connects)
    float boneStartX, boneStartY, boneEndX, boneEndY;
    binding.bone->getWorldEndpoints(boneStartX, boneStartY, boneEndX, boneEndY);
    
    // Store bone's current rotation when binding
    Transform boneWorld = binding.bone->getWorldTransform();
    binding.bindRotation = boneWorld.rotation;
    
    // Calculate offset from bone start to sprite center (in bone's local space)
    float dx = spriteCenter.x - boneStartX;
    float dy = spriteCenter.y - boneStartY;
    
    // Store offset in bone's local coordinate system (unrotated)
    float cosR = std::cos(-binding.bindRotation); // Inverse rotation
    float sinR = std::sin(-binding.bindRotation);
    binding.bindOffset.x = dx * cosR - dy * sinR;
    binding.bindOffset.y = dx * sinR + dy * cosR;
    
    std::cout << "Calculated bind offset: (" << binding.bindOffset.x << ", " << binding.bindOffset.y 
              << ") at rotation: " << binding.bindRotation << std::endl;
}

void Sprite::bindToBone(std::shared_ptr<Bone> bone, float weight) {
    if (!bone) return;
    
    // Check if already bound to this bone
    for (auto& binding : m_boneBindings) {
        if (binding.bone == bone) {
            binding.weight = weight;
            normalizeWeights();
            return;
        }
    }
    
    // Create new binding
    BoneBinding newBinding(bone, weight);
    
    // Calculate offset to maintain current position
    calculateBindOffset(newBinding);
    
    // Add new binding
    m_boneBindings.push_back(newBinding);
    normalizeWeights();
    
    std::cout << "Bound sprite '" << m_name << "' to bone '" << bone->getName() << "' with weight " << weight << std::endl;
}

void Sprite::unbindFromBone(std::shared_ptr<Bone> bone) {
    if (!bone) return;
    
    auto it = std::remove_if(m_boneBindings.begin(), m_boneBindings.end(),
        [bone](const BoneBinding& binding) { return binding.bone == bone; });
    
    if (it != m_boneBindings.end()) {
        m_boneBindings.erase(it, m_boneBindings.end());
        normalizeWeights();
        std::cout << "Unbound sprite '" << m_name << "' from bone '" << bone->getName() << "'" << std::endl;
    }
}

void Sprite::clearAllBindings() {
    // Store current world position before clearing
    Vector2 currentPos = getCurrentCenterPosition();
    
    m_boneBindings.clear();
    
    // Set local transform to current world position to maintain visual position
    m_localTransform.x = currentPos.x;
    m_localTransform.y = currentPos.y;
    
    std::cout << "Cleared all bindings from sprite '" << m_name << "'" << std::endl;
}

std::shared_ptr<Bone> Sprite::getPrimaryBone() const {
    if (m_boneBindings.empty()) {
        return nullptr;
    }
    
    // Return bone with highest weight
    auto maxBinding = std::max_element(m_boneBindings.begin(), m_boneBindings.end(),
        [](const BoneBinding& a, const BoneBinding& b) {
            return a.weight < b.weight;
        });
    
    return maxBinding->bone;
}

void Sprite::setVertices(const std::vector<Vertex>& vertices) {
    m_originalVertices = vertices;
    m_deformedVertices = vertices;
}

Transform Sprite::getWorldTransform() const {
    if (!isBoundToBones()) {
        // No bone bindings - return local transform
        return m_localTransform;
    }
    
    // Calculate weighted average of bone transforms
    Transform result;
    result.x = 0.0f;
    result.y = 0.0f;
    result.rotation = 0.0f;
    result.scaleX = 0.0f;
    result.scaleY = 0.0f;
    result.length = m_localTransform.length;
    
    float totalWeight = 0.0f;
    
    for (const auto& binding : m_boneBindings) {
        if (!binding.bone) continue;
        
        Transform boneWorld = binding.bone->getWorldTransform();
        float boneStartX, boneStartY, boneEndX, boneEndY;
        binding.bone->getWorldEndpoints(boneStartX, boneStartY, boneEndX, boneEndY);
        
        // Calculate relative rotation (how much the bone has rotated since binding)
        float relativeRotation = boneWorld.rotation - binding.bindRotation;
        
        // Apply current bone rotation to the stored offset
        float cosR = std::cos(boneWorld.rotation);
        float sinR = std::sin(boneWorld.rotation);
        float rotatedOffsetX = binding.bindOffset.x * cosR - binding.bindOffset.y * sinR;
        float rotatedOffsetY = binding.bindOffset.x * sinR + binding.bindOffset.y * cosR;
        
        // Calculate position for this bone
        float worldX = boneStartX + rotatedOffsetX;
        float worldY = boneStartY + rotatedOffsetY;
        
        // Accumulate weighted transform
        result.x += worldX * binding.weight;
        result.y += worldY * binding.weight;
        result.rotation += (m_localTransform.rotation + relativeRotation) * binding.weight;
        result.scaleX += boneWorld.scaleX * binding.weight;
        result.scaleY += boneWorld.scaleY * binding.weight;
        
        totalWeight += binding.weight;
    }
    
    // Normalize by total weight
    if (totalWeight > 0.0f) {
        result.x /= totalWeight;
        result.y /= totalWeight;
        result.rotation /= totalWeight;
        result.scaleX /= totalWeight;
        result.scaleY /= totalWeight;
    }
    
    return result;
}

void Sprite::applyBoneDeformation() {
    if (!isBoundToBones()) {
        // No bone bindings - apply local transform to original vertices
        for (size_t i = 0; i < m_deformedVertices.size(); ++i) {
            Vertex& vertex = m_deformedVertices[i];
            const Vertex& original = m_originalVertices[i];
            
            // Apply sprite's local transform
            float x = original.x;
            float y = original.y;
            
            // Apply local rotation
            float cosR = std::cos(m_localTransform.rotation);
            float sinR = std::sin(m_localTransform.rotation);
            float rx = x * cosR - y * sinR;
            float ry = x * sinR + y * cosR;
            
            // Apply scale and translation
            vertex.x = rx * m_localTransform.scaleX + m_localTransform.x;
            vertex.y = ry * m_localTransform.scaleY + m_localTransform.y;
        }
        return;
    }
    
    // Apply bone deformation to each vertex
    for (size_t i = 0; i < m_deformedVertices.size(); ++i) {
        Vertex& vertex = m_deformedVertices[i];
        const Vertex& original = m_originalVertices[i];
        
        // Reset vertex position
        vertex.x = 0.0f;
        vertex.y = 0.0f;
        
        float totalWeight = 0.0f;
        
        // Apply weighted bone transformations
        for (const auto& binding : m_boneBindings) {
            if (!binding.bone) continue;
            
            // Get bone world position
            float boneStartX, boneStartY, boneEndX, boneEndY;
            binding.bone->getWorldEndpoints(boneStartX, boneStartY, boneEndX, boneEndY);
            
            Transform boneWorld = binding.bone->getWorldTransform();
            
            // Calculate relative rotation (how much the bone has rotated since binding)
            float relativeRotation = boneWorld.rotation - binding.bindRotation;
            
            // Transform the original vertex
            float localX = original.x;
            float localY = original.y;
            
            // Apply sprite's local transform first
            float cosLocal = std::cos(m_localTransform.rotation);
            float sinLocal = std::sin(m_localTransform.rotation);
            float transformedX = localX * cosLocal - localY * sinLocal;
            float transformedY = localX * sinLocal + localY * cosLocal;
            
            transformedX *= m_localTransform.scaleX;
            transformedY *= m_localTransform.scaleY;
            
            // Apply relative bone rotation
            float cosRel = std::cos(relativeRotation);
            float sinRel = std::sin(relativeRotation);
            float rotatedX = transformedX * cosRel - transformedY * sinRel;
            float rotatedY = transformedX * sinRel + transformedY * cosRel;
            
            // Apply current bone rotation to the stored offset
            float cosR = std::cos(boneWorld.rotation);
            float sinR = std::sin(boneWorld.rotation);
            float rotatedOffsetX = binding.bindOffset.x * cosR - binding.bindOffset.y * sinR;
            float rotatedOffsetY = binding.bindOffset.x * sinR + binding.bindOffset.y * cosR;
            
            // Final position: bone start + rotated offset + rotated vertex
            float finalX = boneStartX + rotatedOffsetX + rotatedX;
            float finalY = boneStartY + rotatedOffsetY + rotatedY;
            
            // Apply weight
            vertex.x += finalX * binding.weight;
            vertex.y += finalY * binding.weight;
            totalWeight += binding.weight;
        }
        
        // Normalize by total weight
        if (totalWeight > 0.0f) {
            vertex.x /= totalWeight;
            vertex.y /= totalWeight;
        }
    }
}

void Sprite::updateDeformation() {
    applyBoneDeformation();
}

void Sprite::normalizeWeights() {
    if (m_boneBindings.empty()) return;
    
    float totalWeight = 0.0f;
    for (const auto& binding : m_boneBindings) {
        totalWeight += binding.weight;
    }
    
    if (totalWeight > 0.0f) {
        for (auto& binding : m_boneBindings) {
            binding.weight /= totalWeight;
        }
    }
}

} // namespace Riggle