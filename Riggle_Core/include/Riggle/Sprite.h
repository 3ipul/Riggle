#pragma once
#include "Math.h"  // Include the common Transform and Vertex
#include <string>
#include <vector>

namespace Riggle {

class Bone;

// Simple Vertex struct for sprite deformation
struct Vertex {
    float x = 0.0f;
    float y = 0.0f;
    
    Vertex() = default;
    Vertex(float x, float y) : x(x), y(y) {}
};

class Sprite {
public:
    Sprite(const std::string& name, const std::string& texturePath);
    ~Sprite() = default;

    // Basic properties
    const std::string& getName() const { return m_name; }
    const std::string& getTexturePath() const { return m_texturePath; }
    
    // Transform management
    void setTransform(const Transform& transform) { m_transform = transform; }
    const Transform& getLocalTransform() const { return m_transform; }
    Transform getWorldTransform() const;
    
    // Vertex management (for deformation)
    void setVertices(const std::vector<Vertex>& vertices) { m_vertices = vertices; }
    const std::vector<Vertex>& getVertices() const { return m_vertices; }
    std::vector<Vertex> getDeformedVertices() const;
    
    // Setup helpers
    void setupAsQuad(float width, float height);
    
    // Bone attachment (for animation)
    void attachToBone(Bone* bone) { m_attachedBone = bone; }
    void detachFromBone() { m_attachedBone = nullptr; }
    Bone* getAttachedBone() const { return m_attachedBone; }
    bool isAttachedToBone() const { return m_attachedBone != nullptr; }

private:
    std::string m_name;
    std::string m_texturePath;
    Transform m_transform;
    std::vector<Vertex> m_vertices;
    Bone* m_attachedBone;
    
    // Helper functions
    Vertex transformVertex(const Vertex& vertex, const Transform& transform) const;
};

} // namespace Riggle