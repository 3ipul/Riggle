#pragma once

#include <SFML/Graphics.hpp>
#include "Panels/AssetBrowserPanel.h"
#include "Panels/ScenePanel.h"
#include "Panels/SpriteInspectorPanel.h"
#include <memory>
#include <vector>

namespace Riggle {

class EditorController {
public:
    EditorController();
    ~EditorController() = default;

    void update(sf::RenderWindow& window);
    void render(sf::RenderWindow& window);
    void handleEvent(const sf::Event& event);

private:
    // Panels
    std::vector<std::unique_ptr<BasePanel>> m_panels;
    AssetBrowserPanel* m_assetBrowserPanel;
    ScenePanel* m_scenePanel;
    SpriteInspectorPanel* m_spriteInspectorPanel;
    
    // Current render window for viewport rendering
    sf::RenderWindow* m_currentWindow;
    
    // UI functions
    void renderMainMenuBar();
    void renderViewport();
    
    // Panel management
    void initializePanels();
    void setupPanelCallbacks();
    
    // Callbacks
    void onAssetSelected(const AssetInfo& asset);
    void onSpriteSelected(Sprite* sprite);
};

} // namespace Riggle
