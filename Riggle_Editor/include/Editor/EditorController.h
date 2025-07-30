#pragma once

#include <SFML/Graphics.hpp>
#include "Panels/AssetBrowserPanel.h"
#include "Panels/AssetPanel.h"
#include "Panels/PropertyPanel.h"
#include "Panels/ScenePanel.h"
#include "Panels/AnimationPanel.h"
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
    AssetPanel* getAssetPanel() const { return m_assetPanel; }
    PropertyPanel* getPropertyPanel() const { return m_propertyPanel; }
    AssetBrowserPanel* getAssetBrowserPanel() const { return m_assetBrowserPanel; }
    AnimationPanel* getAnimationPanel() const { return m_animationPanel; }

private:
    // Panel management
    std::vector<std::unique_ptr<BasePanel>> m_panels;
    AssetBrowserPanel* m_assetBrowserPanel;
    AssetPanel* m_assetPanel;
    PropertyPanel* m_propertyPanel;
    ScenePanel* m_scenePanel;
    AnimationPanel* m_animationPanel;
    
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
    void onMultipleAssetsSelected(const std::vector<AssetInfo>& assets);
    void onSpriteSelected(Sprite* sprite);
    void onSpriteDeleted(Sprite* sprite);
    void onBoneSelected(std::shared_ptr<Bone> bone);    
    void onSpriteBindingChanged(Sprite* sprite, std::shared_ptr<Bone> bone);  
    
    // Project management
    void newProject();             
    void saveProject();             
    void loadProject();            
    bool hasUnsavedChanges() const; 
};

} // namespace Riggle
