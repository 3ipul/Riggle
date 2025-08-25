#pragma once

#include "BasePanel.h"
#include <Riggle/Character.h>
#include <Riggle/Sprite.h>
#include <functional>
#include <string>

namespace Riggle {

class AssetBrowserPanel; // Forward declaration

class AssetPanel : public BasePanel {
public:
    AssetPanel();
    ~AssetPanel() = default;

    void render() override;
    void update(sf::RenderWindow& window) override;

    // Character management
    void setCharacter(Character* character) { m_character = character; }
    
    // Selection management
    void setSelectedSprite(Sprite* sprite) { m_selectedSprite = sprite; }
    Sprite* getSelectedSprite() const { return m_selectedSprite; }

    // Clear selection method
    void clearSelection() { 
        m_selectedSprite = nullptr; 
        if (m_onSpriteSelected) {
            m_onSpriteSelected(nullptr);
        }
    }

    // Asset browser integration
    void setAssetBrowserPanel(AssetBrowserPanel* assetBrowser) { m_assetBrowserPanel = assetBrowser; }

    // Callbacks
    void setOnSpriteSelected(std::function<void(Sprite*)> callback) {
        m_onSpriteSelected = callback;
    }

    void setOnSpriteDeleted(std::function<void(Sprite*)> callback) {
        m_onSpriteDeleted = callback;
    }

private:
    Character* m_character;
    Sprite* m_selectedSprite;
    AssetBrowserPanel* m_assetBrowserPanel;

    // Drag and drop state
    int m_draggedIndex;
    bool m_isDragging;
    
    // Callbacks
    std::function<void(Sprite*)> m_onSpriteSelected;
    std::function<void(Sprite*)> m_onSpriteDeleted;
    
    // Helper functions
    void renderEmptyState();
    void renderSpriteList();
    void renderSpriteItem(Sprite* sprite, size_t index);
    void moveSpriteUp(size_t index);
    void moveSpriteDown(size_t index);
    void deleteSprite(size_t index);
    void toggleSpriteVisibility(size_t index);
    void handleDragAndDrop(size_t index);
    bool isAssetBrowserVisible() const;
};

} // namespace Riggle