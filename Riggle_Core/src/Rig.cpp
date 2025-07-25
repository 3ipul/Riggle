#include "Riggle/Rig.h"

namespace Riggle {

Rig::Rig(const std::string& name) : m_name(name) {
}

std::shared_ptr<Bone> Rig::createBone(const std::string& name, float length) {
    auto bone = std::make_shared<Bone>(name, length);
    m_allBones.push_back(bone);
    m_rootBones.push_back(bone);
    return bone;
}

std::shared_ptr<Bone> Rig::createChildBone(std::shared_ptr<Bone> parent, const std::string& name, float length) {
    if (!parent) return nullptr;
    
    auto bone = std::make_shared<Bone>(name, length);
    m_allBones.push_back(bone);
    parent->addChild(bone);
    return bone;
}

void Rig::addRootBone(std::shared_ptr<Bone> bone) {
    if (!bone) return;
    
    m_rootBones.push_back(bone);
    m_allBones.push_back(bone);
}

std::shared_ptr<Bone> Rig::findBone(const std::string& name) const {
    for (const auto& bone : m_allBones) {
        if (bone->getName() == name) {
            return bone;
        }
    }
    return nullptr;
}

void Rig::clear() {
    m_rootBones.clear();
    m_allBones.clear();
}

// Add method to update all bone transforms
void Rig::updateTransforms() {
    // Force update of all root bones (children will update automatically)
    for (auto& bone : m_rootBones) {
        bone->getWorldTransform(); // This triggers update if dirty
    }
}

} // namespace Riggle