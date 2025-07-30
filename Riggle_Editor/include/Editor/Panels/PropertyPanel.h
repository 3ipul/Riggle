#pragma once

#include "BasePanel.h"
#include <Riggle/Character.h>
#include <Riggle/Sprite.h>
#include <Riggle/Bone.h>
#include <functional>

namespace Riggle {

enum class PropertyType {
    None,
    Sprite,
    Bone  // For future Hierarchy panel integration
};

class PropertyPanel : public BasePanel {
public:
    PropertyPanel();
    ~PropertyPanel() = default;

    void render() override;
    void update(sf::RenderWindow& window) override;

    // Selection management
    void setSelectedSprite(Sprite* sprite);
    void setSelectedBone(Bone* bone);  // For future use
    void clearSelection();

    Sprite* getSelectedSprite() const { return m_selectedSprite; }

    // Character management
    void setCharacter(Character* character) { m_character = character; }

private:
    Character* m_character;
    PropertyType m_currentType;
    
    // Selected objects
    Sprite* m_selectedSprite;
    Bone* m_selectedBone;  // For future use
    
    // Movement controls
    float m_moveStep;
    int m_moveStepInt;
    
    // Helper functions
    void renderEmptyState();
    void renderSpriteProperties();
    void renderBoneProperties();  // For future use
    void renderTransformControls();
    void renderMovementButtons();
    void renderSpriteBindings();
    void moveSprite(float deltaX, float deltaY);
};

} // namespace Riggle