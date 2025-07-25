#pragma once
#include "Rig.h"
#include "Sprite.h"
#include <memory>
#include <vector>
#include <string>

namespace Riggle {

class Character {
public:
    Character(const std::string& name);
    ~Character() = default;

    // Basic properties
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    // Rig management
    void setRig(std::unique_ptr<Rig> rig) { m_rig = std::move(rig); }
    Rig* getRig() const { return m_rig.get(); }

    // Sprite management
    void addSprite(std::unique_ptr<Sprite> sprite);
    void removeSprite(const std::string& name);
    Sprite* getSprite(const std::string& name) const;
    const std::vector<std::unique_ptr<Sprite>>& getSprites() const { return m_sprites; }

    // Animation update
    void updateDeformations();

private:
    std::string m_name;
    std::unique_ptr<Rig> m_rig;
    std::vector<std::unique_ptr<Sprite>> m_sprites;
};

} // namespace Riggle