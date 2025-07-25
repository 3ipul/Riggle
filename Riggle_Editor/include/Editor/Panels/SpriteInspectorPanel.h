#pragma once
#include "BasePanel.h"
#include <Riggle/Character.h>
#include <Riggle/Sprite.h>
#include <functional>
#include <string>

namespace Riggle {

class SpriteInspectorPanel : public BasePanel {
public:
    SpriteInspectorPanel();
    ~SpriteInspectorPanel() = default;

    void render() override;
    void update(sf::RenderWindow& window) override;
    void handleEvent(const sf::Event& event) override;

    // Character management
    void setCharacter(Character* character) { m_character = character; }
    
    // Selection management
    void setSelectedSprite(Sprite* sprite) { m_selectedSprite = sprite; }
    Sprite* getSelectedSprite() const { return m_selectedSprite; }

    // Callbacks
    void setOnSpriteSelected(std::function<void(Sprite*)> callback) {
        m_onSpriteSelected = callback;
    }

private:
    Character* m_character;
    Sprite* m_selectedSprite;
    
    // Movement controls
    float m_moveStep;
    int m_moveStepInt;
    
    // Callbacks
    std::function<void(Sprite*)> m_onSpriteSelected;
    
    // Helper functions
    void renderSpriteList();
    void renderTransformControls();
    void renderMovementButtons();
    void moveSprite(float deltaX, float deltaY);
    void renderSpriteProperties();
};

} // namespace Riggle