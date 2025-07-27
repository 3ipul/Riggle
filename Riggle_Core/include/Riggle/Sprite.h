#pragma once

#include "Math.h"
#include <vector>
#include <string>
#include <memory>

namespace Riggle {

class Bone;

struct BoneBinding {
    std::shared_ptr<Bone> bone;
    float weight;           // 0.0 to 1.0
    Vector2 bindOffset;     // Offset from bone when binding
    float bindRotation;     // Bone's rotation when bound (to calculate relative rotation)
    
    BoneBinding(std::shared_ptr<Bone> b, float w = 1.0f) 
        : bone(b), weight(w), bindOffset(0.0f, 0.0f), bindRotation(0.0f) {}
};

class Sprite {
public:
    Sprite(const std::string& name, const std::string& texturePath);
    ~Sprite() = default;

    // Basic properties
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    const std::string& getTexturePath() const { return m_texturePath; }
    void setTexturePath(const std::string& path) { m_texturePath = path; }

    // Transform
    Transform getLocalTransform() const { return m_localTransform; }
    void setLocalTransform(const Transform& transform) { m_localTransform = transform; }
    void setTransform(const Transform& transform) { setLocalTransform(transform); }
    
    // Vertices
    const std::vector<Vertex>& getOriginalVertices() const { return m_originalVertices; }
    const std::vector<Vertex>& getDeformedVertices() const { return m_deformedVertices; }
    void setVertices(const std::vector<Vertex>& vertices);
    void setupAsQuad(float width, float height, const Vector2& pivot = Vector2(0.5f, 0.5f));

    // CLEAN BONE BINDING SYSTEM (no legacy)
    void bindToBone(std::shared_ptr<Bone> bone, float weight = 1.0f);
    void unbindFromBone(std::shared_ptr<Bone> bone);
    void clearAllBindings();
    
    // Check bindings
    bool isBoundToBones() const { return !m_boneBindings.empty(); }
    const std::vector<BoneBinding>& getBoneBindings() const { return m_boneBindings; }
    std::shared_ptr<Bone> getPrimaryBone() const; // Highest weight bone
    
    // Deformation
    Transform getWorldTransform() const;
    void updateDeformation();
    
    // Visibility
    bool isVisible() const { return m_isVisible; }
    void setVisible(bool visible) { m_isVisible = visible; }

private:
    std::string m_name;
    std::string m_texturePath;
    
    Transform m_localTransform;
    
    std::vector<Vertex> m_originalVertices;
    std::vector<Vertex> m_deformedVertices;
    
    // ONLY multi-bone binding system
    std::vector<BoneBinding> m_boneBindings;
    
    bool m_isVisible;
    
    // Helper functions
    void applyBoneDeformation();
    void normalizeWeights();
    Vector2 getCurrentCenterPosition() const;
    void calculateBindOffset(BoneBinding& binding);
};
// ...existing code...

} // namespace Riggle