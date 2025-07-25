#pragma once
#include "BasePanel.h"
#include <Riggle/Character.h>
#include "../Render/SpriteRenderer.h"
#include "../Render/BoneRenderer.h"
#include "../AssetManager.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <functional>

namespace Riggle {

class ScenePanel : public BasePanel {
public:
    ScenePanel();
    ~ScenePanel() = default;

    void render() override;
    void update(sf::RenderWindow& window) override;
    void handleEvent(const sf::Event& event) override;

    // Character management
    void setCharacter(std::unique_ptr<Character> character);
    Character* getCharacter() const { return m_character.get(); }

    // Sprite management
    void addSpriteFromAsset(const AssetInfo& asset, const sf::Vector2f& position);
    
    // View management
    void resetView();
    sf::View& getView() { return m_view; }

    // Selection callbacks
    void setOnSpriteSelected(std::function<void(Sprite*)> callback) {
        m_onSpriteSelected = callback;
    }

    // Rendering
    void renderScene(sf::RenderTarget& target);

private:
    std::unique_ptr<Character> m_character;
    std::unique_ptr<SpriteRenderer> m_spriteRenderer;
    std::unique_ptr<BoneRenderer> m_boneRenderer;

    // View and interaction
    sf::View m_view;
    sf::RenderWindow* m_window;
    
    // Mouse interaction
    bool m_isDragging;
    bool m_isPanning;
    sf::Vector2f m_lastMousePos;
    sf::Vector2f m_dragOffset;
    Sprite* m_draggedSprite;
    Sprite* m_selectedSprite;

    // Viewport
    sf::RenderTexture m_renderTexture;
    bool m_viewportInitialized;
    sf::Vector2u m_viewportSize;

    // Callbacks
    std::function<void(Sprite*)> m_onSpriteSelected;

    // Display options
    bool m_showBones;
    bool m_showSprites;
    bool m_showGrid;

    // Helper functions
    sf::Vector2f screenToWorld(const sf::Vector2f& screenPos, const sf::Vector2f& viewportSize) const;
    void handleMouseInput(const sf::Vector2f& worldPos, const sf::Vector2f& screenPos, const sf::Vector2f& viewportSize);
    Sprite* getSpriteAtPosition(const sf::Vector2f& worldPos);
    bool isPointInQuad(const sf::Vector2f& point, const std::vector<Vertex>& vertices) const;
    void updateSpriteDragging();
    void renderGrid(sf::RenderTarget& target);
    void renderSceneControls();
    void initializeViewport(const sf::Vector2u& size);
};

} // namespace Riggle