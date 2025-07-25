#include <Riggle/Sprite.h>
#include <Riggle/Bone.h>
#include <cmath>

namespace Riggle {

Sprite::Sprite(const std::string& name, const std::string& texturePath)
    : m_name(name)
    , m_texturePath(texturePath)
    , m_transform()
    , m_attachedBone(nullptr)
{
}

Transform Sprite::getWorldTransform() const {
    if (m_attachedBone) {
        // Combine local transform with bone transform
        Transform boneTransform = m_attachedBone->getWorldTransform();
        
        Transform worldTransform;
        worldTransform.x = boneTransform.x + m_transform.x;
        worldTransform.y = boneTransform.y + m_transform.y;
        worldTransform.rotation = boneTransform.rotation + m_transform.rotation;
        worldTransform.scaleX = boneTransform.scaleX * m_transform.scaleX;
        worldTransform.scaleY = boneTransform.scaleY * m_transform.scaleY;
        worldTransform.length = m_transform.length; // Sprites don't use bone length
        
        return worldTransform;
    }
    
    return m_transform;
}

std::vector<Vertex> Sprite::getDeformedVertices() const {
    std::vector<Vertex> deformedVertices = m_vertices;
    Transform worldTransform = getWorldTransform();
    
    // Apply world transform to all vertices
    for (auto& vertex : deformedVertices) {
        vertex = transformVertex(vertex, worldTransform);
    }
    
    return deformedVertices;
}

void Sprite::setupAsQuad(float width, float height) {
    m_vertices.clear();
    m_vertices.resize(4);
    
    // Create a quad centered at origin
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    
    m_vertices[0] = Vertex(-halfWidth, -halfHeight); // Top-left
    m_vertices[1] = Vertex( halfWidth, -halfHeight); // Top-right
    m_vertices[2] = Vertex( halfWidth,  halfHeight); // Bottom-right
    m_vertices[3] = Vertex(-halfWidth,  halfHeight); // Bottom-left
}

Vertex Sprite::transformVertex(const Vertex& vertex, const Transform& transform) const {
    // Apply scale
    float scaledX = vertex.x * transform.scaleX;
    float scaledY = vertex.y * transform.scaleY;
    
    // Apply rotation
    float cosR = std::cos(transform.rotation);
    float sinR = std::sin(transform.rotation);
    
    float rotatedX = scaledX * cosR - scaledY * sinR;
    float rotatedY = scaledX * sinR + scaledY * cosR;
    
    // Apply translation
    return Vertex(rotatedX + transform.x, rotatedY + transform.y);
}

} // namespace Riggle