#pragma once
#include "Bone.h"
#include <vector>
#include <memory>
#include <string>

namespace Riggle {

class Character; // Forward declaration

class Rig {
public:
    Rig(const std::string& name);
    ~Rig() = default;

    // Basic properties
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    // Bone management
    std::shared_ptr<Bone> createBone(const std::string& name, float length = 50.0f);
    std::shared_ptr<Bone> createChildBone(std::shared_ptr<Bone> parent, const std::string& name, float length = 50.0f);
    void removeBone(const std::string& name);
    std::shared_ptr<Bone> findBone(const std::string& name);
    
    // Bone hierarchy
    void addRootBone(std::shared_ptr<Bone> bone) { m_rootBones.push_back(bone); }
    const std::vector<std::shared_ptr<Bone>>& getRootBones() const { return m_rootBones; }
    std::vector<std::shared_ptr<Bone>> getAllBones() const;
    
    // CRITICAL: Update all bone world transforms
    void updateWorldTransforms();
    void forceUpdateWorldTransforms(); // Force immediate update

    // Character reference management
    void setCharacter(Character* character);

private:
    std::string m_name;
    std::vector<std::shared_ptr<Bone>> m_rootBones;
    Character* m_character = nullptr; // Non-owning pointer
    
    void updateBoneHierarchy(std::shared_ptr<Bone> bone); // Recursive helper
};

} // namespace Riggle