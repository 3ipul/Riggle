#include <Riggle/Character.h>

namespace Riggle {

Character::Character(const std::string& name)
    : m_name(name)
{
}

void Character::addSprite(std::unique_ptr<Sprite> sprite) {
    if (sprite) {
        m_sprites.push_back(std::move(sprite));
    }
}

void Character::removeSprite(const std::string& name) {
    m_sprites.erase(
        std::remove_if(m_sprites.begin(), m_sprites.end(),
            [&name](const std::unique_ptr<Sprite>& sprite) {
                return sprite->getName() == name;
            }),
        m_sprites.end()
    );
}

Sprite* Character::getSprite(const std::string& name) const {
    for (const auto& sprite : m_sprites) {
        if (sprite->getName() == name) {
            return sprite.get();
        }
    }
    return nullptr;
}

void Character::updateDeformations() {
    // Update rig if it exists
    if (m_rig) {
        m_rig->updateTransforms();
    }
    
    // Note: Sprite deformations are calculated on-demand via getDeformedVertices()
    // This keeps the core lightweight and lets the renderer handle the heavy lifting
}

} // namespace Riggle