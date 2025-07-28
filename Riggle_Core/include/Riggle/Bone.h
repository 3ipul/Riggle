#pragma once
#include "Math.h"  // Include the common Transform
#include <vector>
#include <memory>
#include <string>

namespace Riggle {

class Bone : public std::enable_shared_from_this<Bone> {
public:
    Bone(const std::string& name, float length = 50.0f);
    ~Bone() = default;

    // Hierarchy
    void addChild(std::shared_ptr<Bone> child);
    void setParent(std::shared_ptr<Bone> parent);
    
    // Transform
    void setLocalTransform(const Transform& transform);
    void setLocalRotation(float rotation);
    Transform getLocalTransform() const { return m_localTransform; }
    Transform getWorldTransform() const;
    
    // Getters
    const std::string& getName() const { return m_name; }
    const std::vector<std::shared_ptr<Bone>>& getChildren() const { return m_children; }
    std::shared_ptr<Bone> getParent() const { return m_parent.lock(); }
    
    // World positions for rendering
    void getWorldEndpoints(float& startX, float& startY, float& endX, float& endY) const;

    // Force mark world transform as dirty
    void markWorldTransformDirty();

private:
    void updateWorldTransform() const;
    
    std::string m_name;
    Transform m_localTransform;
    mutable Transform m_worldTransform;
    mutable bool m_worldTransformDirty = true;
    
    std::vector<std::shared_ptr<Bone>> m_children;
    std::weak_ptr<Bone> m_parent;
};

} // namespace Riggle