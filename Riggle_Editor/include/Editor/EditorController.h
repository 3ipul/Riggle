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

    // Exit handling
    void requestExit();
    bool shouldExit() const { return m_shouldExit; }
    
    // Panel access
    ScenePanel* getScenePanel() const { return m_scenePanel; }
    SpriteInspectorPanel* getSpriteInspectorPanel() const { return m_spriteInspectorPanel; }
    AssetBrowserPanel* getAssetBrowserPanel() const { return m_assetBrowserPanel; }

private:
    // Panel management
    std::vector<std::unique_ptr<BasePanel>> m_panels;
    AssetBrowserPanel* m_assetBrowserPanel;
    ScenePanel* m_scenePanel;
    SpriteInspectorPanel* m_spriteInspectorPanel;
    
    // State
    sf::RenderWindow* m_currentWindow;
    bool m_showExitConfirmation = false;
    bool m_shouldExit = false;          
    bool m_hasUnsavedChanges = false;
    
    // Initialization
    void initializePanels();
    void setupPanelCallbacks();
    
    // Rendering
    void renderMainMenuBar();
    void renderViewport();
    void renderExitConfirmation();
    
    // Event callbacks
    void onAssetSelected(const AssetInfo& asset);
    void onSpriteSelected(Sprite* sprite);
    void onBoneSelected(std::shared_ptr<Bone> bone);    
    void onSpriteBindingChanged(Sprite* sprite, std::shared_ptr<Bone> bone);  
    
    // Project management
    void newProject();             
    void saveProject();             
    void loadProject();            
    bool hasUnsavedChanges() const; 
};

} // namespace Riggle
