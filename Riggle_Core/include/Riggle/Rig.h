#pragma once
#include "Bone.h"
#include "Math.h"
#include <vector>
#include <memory>
#include <string>

namespace Riggle {

class Rig {
public:
    Rig(const std::string& name = "Untitled Rig");
    ~Rig() = default;

    // Bone management
    std::shared_ptr<Bone> createBone(const std::string& name, float length = 50.0f);
    std::shared_ptr<Bone> createChildBone(std::shared_ptr<Bone> parent, const std::string& name, float length = 50.0f);
    void addRootBone(std::shared_ptr<Bone> bone);
    
    // Getters
    const std::string& getName() const { return m_name; }
    const std::vector<std::shared_ptr<Bone>>& getRootBones() const { return m_rootBones; }
    const std::vector<std::shared_ptr<Bone>>& getAllBones() const { return m_allBones; }
    
    // Find bones
    std::shared_ptr<Bone> findBone(const std::string& name) const;
    
    // Animation
    void updateTransforms();
    
    // Utility
    void clear();
    size_t getBoneCount() const { return m_allBones.size(); }

private:
    std::string m_name;
    std::vector<std::shared_ptr<Bone>> m_rootBones;
    std::vector<std::shared_ptr<Bone>> m_allBones;
};

} // namespace Riggle