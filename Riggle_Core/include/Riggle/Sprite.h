#pragma once

#include "Math.h"
#include <vector>
#include <string>
#include <memory>

namespace Riggle {

class Bone;

// Simplified binding - one sprite = one bone only
struct BoneBinding {
    std::shared_ptr<Bone> bone;
    float weight;           // Always 1.0 for single bone binding
    Vector2 bindOffset;     // Offset from bone origin when bound
    float bindRotation;     // Rotation relative to bone when bound
};

class Sprite : public std::enable_shared_from_this<Sprite> {
public:
    Sprite(const std::string& name, const std::string& texturePath);
    ~Sprite() = default;

    // Basic properties
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    const std::string& getTexturePath() const { return m_texturePath; }
    
    bool isVisible() const { return m_isVisible; }
    void setVisible(bool visible) { m_isVisible = visible; }

    // Transform
    const Transform& getLocalTransform() const { return m_localTransform; }
    void setLocalTransform(const Transform& transform) { m_localTransform = transform; }
    void setTransform(const Transform& transform) { m_localTransform = transform; }
    Transform getWorldTransform() const;

    // SIMPLIFIED: Single bone binding only
    bool isBoundToBone() const { return m_binding.bone != nullptr; }
    std::shared_ptr<Bone> getBoundBone() const { return m_binding.bone; }
    const BoneBinding& getBoneBinding() const { return m_binding; }
    
    // Binding operations
    void bindToBone(std::shared_ptr<Bone> bone, const Vector2& offset = {0, 0}, float rotation = 0.0f);
    void unbindFromBone();
    void clearBinding() { unbindFromBone(); }
    
    // Update transform based on bone (called during animation)
    void updateFromBone();

private:
    std::string m_name;
    std::string m_texturePath;
    bool m_isVisible;
    Transform m_localTransform;
    
    // SIMPLIFIED: Single bone binding
    BoneBinding m_binding;  // Only one binding per sprite
};

} // namespace Riggle