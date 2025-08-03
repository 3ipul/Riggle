#include "Riggle/Character.h"
#include <algorithm>
#include <iostream>

namespace Riggle {

Character::Character(const std::string& name) 
    : m_name(name), m_autoUpdate(true), m_manualBoneEditMode(false) {
}

void Character::notifyTransformChanged(const std::string& boneName, const Transform& oldTransform, const Transform& newTransform) {
    // Only notify during manual edit mode (when Editor is manipulating bones)
    if (!m_manualBoneEditMode) {
        return;
    }
    
    TransformEvent event{
        boneName, 
        oldTransform, 
        newTransform, 
        getCurrentTime()
    };
    
    for (auto& handler : m_transformHandlers) {
        handler(event);
    }
}

float Character::getCurrentTime() const {
    static auto startTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
    return duration.count() / 1000.0f;
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

void Character::removeSprite(Sprite* sprite) {
    if (!sprite) return;
    
    auto it = std::remove_if(m_sprites.begin(), m_sprites.end(),
        [sprite](const std::unique_ptr<Sprite>& spr) {
            return spr.get() == sprite;
        });
    
    if (it != m_sprites.end()) {
        m_sprites.erase(it, m_sprites.end());
        if (m_autoUpdate) {
            updateDeformations();
        }
        std::cout << "Removed sprite: " << sprite->getName() << std::endl;
    }
}

void Character::removeSpriteAt(size_t index) {
    if (index < m_sprites.size()) {
        std::string name = m_sprites[index]->getName();
        m_sprites.erase(m_sprites.begin() + index);
        if (m_autoUpdate) {
            updateDeformations();
        }
        std::cout << "Removed sprite at index " << index << ": " << name << std::endl;
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

    // Set character reference in rig
    if (m_rig) {
        m_rig->setCharacter(this);
    }

    if (m_autoUpdate) {
        updateDeformations();
    }
}

void Character::addAnimation(std::unique_ptr<Animation> animation) {
    if (animation) {
        m_animations.push_back(std::move(animation));
    }
}

void Character::removeAnimation(const std::string& name) {
    m_animations.erase(
        std::remove_if(m_animations.begin(), m_animations.end(),
            [&name](const std::unique_ptr<Animation>& anim) {
                return anim && anim->getName() == name;
            }),
        m_animations.end()
    );
}

Animation* Character::findAnimation(const std::string& name) {
    for (auto& animation : m_animations) {
        if (animation && animation->getName() == name) {
            return animation.get();
        }
    }
    return nullptr;
}

void Character::update(float deltaTime) {
    // Don't update animations during manual bone editing
    if (m_manualBoneEditMode) {
        return;
    }

    // Update animation player
    m_animationPlayer.update(deltaTime);
    
    // Apply animation to rig
    if (m_rig) {
        m_animationPlayer.applyToRig(m_rig.get());
    }
    
    // Update deformations if auto-update is enabled
    if (m_autoUpdate) {
        updateDeformations();
    }
}

void Character::updateDeformations() {
    if (!m_rig) return;
    
   // Update all bone world transforms first
    m_rig->forceUpdateWorldTransforms();
    
    // Update all sprites based on their bone bindings
    for (auto& sprite : m_sprites) {
        if (sprite->isBoundToBone()) {
            sprite->updateFromBone();  // CHANGED: was updateDeformation()
        }
    }
}

void Character::forceUpdateDeformations() {
    updateDeformations(); // Force immediate update regardless of auto-update setting
}

} // namespace Riggle