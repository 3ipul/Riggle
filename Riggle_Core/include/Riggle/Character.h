#pragma once
#include "Sprite.h"
#include "Rig.h"
#include <vector>
#include <memory>
#include <string>

namespace Riggle {

class Character {
public:
    Character(const std::string& name);
    ~Character() = default;

    // Basic properties
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    // Sprite management
    void addSprite(std::unique_ptr<Sprite> sprite);
    void removeSprite(const std::string& name);
    Sprite* findSprite(const std::string& name);
    const std::vector<std::unique_ptr<Sprite>>& getSprites() const { return m_sprites; }

    // Rig management
    void setRig(std::unique_ptr<Rig> rig);
    Rig* getRig() const { return m_rig.get(); }

    // Deformation - CRITICAL for FK/IK
    void updateDeformations();
    void forceUpdateDeformations(); // Force immediate update
    
    // Auto-update toggle
    void setAutoUpdate(bool autoUpdate) { m_autoUpdate = autoUpdate; }
    bool getAutoUpdate() const { return m_autoUpdate; }

private:
    std::string m_name;
    std::vector<std::unique_ptr<Sprite>> m_sprites;
    std::unique_ptr<Rig> m_rig;
    bool m_autoUpdate = true; // NEW: Auto-update deformations
};

} // namespace Riggle