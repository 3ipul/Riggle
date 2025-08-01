#pragma once

#include "Math.h"
#include <vector>
#include <string>
#include <memory>

namespace Riggle {

class Sprite;

class Bone : public std::enable_shared_from_this<Bone> {
public:
    Bone(const std::string& name, float length);
    ~Bone() = default;

    // Basic properties
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    float getLength() const { return m_length; }
    void setLength(float length);

    // Transform
    const Transform& getLocalTransform() const { return m_localTransform; }
    void setLocalTransform(const Transform& transform);
    Transform getWorldTransform() const;
    void markWorldTransformDirty() { m_worldTransformDirty = true; }

    // Hierarchy
    std::shared_ptr<Bone> getParent() const { return m_parent.lock(); }
    void setParent(std::shared_ptr<Bone> parent) { m_parent = parent; }
    
    const std::vector<std::shared_ptr<Bone>>& getChildren() const { return m_children; }
    void addChild(std::shared_ptr<Bone> child);
    void removeChild(std::shared_ptr<Bone> child);
    
    // Multiple sprite binding support
    const std::vector<std::weak_ptr<Sprite>>& getBoundSprites() const { return m_boundSprites; }
    void addBoundSprite(std::weak_ptr<Sprite> sprite);
    void removeBoundSprite(std::weak_ptr<Sprite> sprite);
    bool hasSprites() const;
    size_t getSpriteCount() const;
    
    // Utility
    void getWorldEndpoints(float& startX, float& startY, float& endX, float& endY) const;
    bool isRoot() const { return m_parent.expired(); }
    std::vector<std::shared_ptr<Bone>> getAllDescendants() const;

private:
    std::string m_name;
    float m_length;
    Transform m_localTransform;
    
    // Hierarchy
    std::weak_ptr<Bone> m_parent;
    std::vector<std::shared_ptr<Bone>> m_children;
    
    // Multiple sprite bindings
    std::vector<std::weak_ptr<Sprite>> m_boundSprites;
    
    // Cached world transform
    mutable Transform m_worldTransform;
    mutable bool m_worldTransformDirty;
    
    void updateWorldTransform() const;
};

} // namespace Riggle