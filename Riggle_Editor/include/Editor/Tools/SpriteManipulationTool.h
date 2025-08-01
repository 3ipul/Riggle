#pragma once
#include <SFML/Graphics.hpp>
#include <Riggle/Character.h>
#include <Riggle/Sprite.h>
#include <functional>
#include <memory>

namespace Riggle {

class SpriteManipulationTool {
public:
    SpriteManipulationTool();
    ~SpriteManipulationTool() = default;

    // Core functionality
    void setCharacter(Character* character);
    void setActive(bool active) { m_isActive = active; }
    bool isActive() const { return m_isActive; }

    // Event handling
    void handleMousePressed(const sf::Vector2f& worldPos);
    void handleMouseMoved(const sf::Vector2f& worldPos);
    void handleMouseReleased();

    // State queries
    bool isDragging() const { return m_isDragging; }
    Sprite* getSelectedSprite() const { return m_selectedSprite; }
    Sprite* getDraggedSprite() const { return m_draggedSprite; }

    // Selection management
    void setSelectedSprite(Sprite* sprite);
    void clearSelection();

    // Callbacks
    void setOnSpriteSelected(std::function<void(Sprite*)> callback) {
        m_onSpriteSelected = callback;
    }

    void setOnSpriteDragStarted(std::function<void(Sprite*)> callback) {
        m_onSpriteDragStarted = callback;
    }

    void setOnSpriteDragEnded(std::function<void(Sprite*)> callback) {
        m_onSpriteDragEnded = callback;
    }

    // Rendering
    void renderOverlay(sf::RenderTarget& target);

private:
    Character* m_character;
    bool m_isActive;

    // Selection and dragging state
    Sprite* m_selectedSprite;
    Sprite* m_draggedSprite;
    bool m_isDragging;
    sf::Vector2f m_dragOffset;
    sf::Vector2f m_dragStartPos;

    // Callbacks
    std::function<void(Sprite*)> m_onSpriteSelected;
    std::function<void(Sprite*)> m_onSpriteDragStarted;
    std::function<void(Sprite*)> m_onSpriteDragEnded;

    // Helper methods
    Sprite* getSpriteAtPosition(const sf::Vector2f& worldPos);
    bool isPointInSprite(const sf::Vector2f& point, Sprite* sprite);
    void startDragging(Sprite* sprite, const sf::Vector2f& worldPos);
    void updateDragging(const sf::Vector2f& worldPos);
    void endDragging();
};

} // namespace Riggle