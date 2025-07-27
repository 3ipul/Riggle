#include "Riggle/Rig.h"
#include <algorithm>
#include <functional>

namespace Riggle {

Rig::Rig(const std::string& name) : m_name(name) {
}

std::shared_ptr<Bone> Rig::createBone(const std::string& name, float length) {
    auto bone = std::make_shared<Bone>(name, length);
    m_rootBones.push_back(bone);
    return bone;
}

std::shared_ptr<Bone> Rig::createChildBone(std::shared_ptr<Bone> parent, const std::string& name, float length) {
    if (!parent) return nullptr;
    
    auto child = std::make_shared<Bone>(name, length);
    parent->addChild(child);
    return child;
}

void Rig::removeBone(const std::string& name) {
    // Remove from root bones
    auto it = std::remove_if(m_rootBones.begin(), m_rootBones.end(),
        [&name](const std::shared_ptr<Bone>& bone) {
            return bone->getName() == name;
        });
    
    if (it != m_rootBones.end()) {
        m_rootBones.erase(it, m_rootBones.end());
        return;
    }
    
    // TODO: Remove from child bones (recursive search)
}

std::shared_ptr<Bone> Rig::findBone(const std::string& name) {
    // Search in all bones (including children)
    auto allBones = getAllBones();
    auto it = std::find_if(allBones.begin(), allBones.end(),
        [&name](const std::shared_ptr<Bone>& bone) {
            return bone->getName() == name;
        });
    
    return (it != allBones.end()) ? *it : nullptr;
}

std::vector<std::shared_ptr<Bone>> Rig::getAllBones() const {
    std::vector<std::shared_ptr<Bone>> allBones;
    
    // Recursively collect all bones
    std::function<void(std::shared_ptr<Bone>)> collectBones = [&](std::shared_ptr<Bone> bone) {
        if (!bone) return;
        allBones.push_back(bone);
        for (const auto& child : bone->getChildren()) {
            collectBones(child);
        }
    };
    
    for (const auto& root : m_rootBones) {
        collectBones(root);
    }
    
    return allBones;
}

void Rig::updateWorldTransforms() {
    // Update world transforms for all root bones (and their children recursively)
    for (auto& rootBone : m_rootBones) {
        updateBoneHierarchy(rootBone);
    }
}

void Rig::forceUpdateWorldTransforms() {
    updateWorldTransforms(); // Force immediate update
}

void Rig::updateBoneHierarchy(std::shared_ptr<Bone> bone) {
    if (!bone) return;
    
    // The Bone class should automatically calculate world transform when requested
    bone->getWorldTransform();
    
    // Recursively update all children
    for (auto& child : bone->getChildren()) {
        updateBoneHierarchy(child);
    }
}

} // namespace Riggle