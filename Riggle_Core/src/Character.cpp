#include "Riggle/Character.h"
#include <algorithm>
#include <iostream>

namespace Riggle {

Character::Character(const std::string& name) 
    : m_name(name), m_autoUpdate(true) {
}

void Character::addSprite(std::unique_ptr<Sprite> sprite) {
    if (sprite) {
        m_sprites.push_back(std::move(sprite));
        if (m_autoUpdate) {
            updateDeformations();
        }
    }
}

void Character::removeSprite(const std::string& name) {
    auto it = std::remove_if(m_sprites.begin(), m_sprites.end(),
        [&name](const std::unique_ptr<Sprite>& sprite) {
            return sprite->getName() == name;
        });
    
    if (it != m_sprites.end()) {
        m_sprites.erase(it, m_sprites.end());
        if (m_autoUpdate) {
            updateDeformations();
        }
    }
}

Sprite* Character::findSprite(const std::string& name) {
    auto it = std::find_if(m_sprites.begin(), m_sprites.end(),
        [&name](const std::unique_ptr<Sprite>& sprite) {
            return sprite->getName() == name;
        });
    
    return (it != m_sprites.end()) ? it->get() : nullptr;
}

void Character::setRig(std::unique_ptr<Rig> rig) {
    m_rig = std::move(rig);
    if (m_autoUpdate) {
        updateDeformations();
    }
}

void Character::updateDeformations() {
    if (!m_rig) return;
    
    // First, update the rig hierarchy (recalculate world transforms)
    m_rig->updateWorldTransforms();
    
    // Then, update all sprite deformations
    for (auto& sprite : m_sprites) {
        if (sprite) {
            sprite->updateDeformation();
        }
    }
}

void Character::forceUpdateDeformations() {
    updateDeformations(); // Force immediate update regardless of auto-update setting
}

} // namespace Riggle