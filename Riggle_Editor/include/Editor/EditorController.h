#pragma once

#include <SFML/Graphics.hpp>
#include "Panels/ViewportPanel.h"
#include "Panels/AssetBrowserPanel.h"
#include "Panels/AssetPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/PropertyPanel.h"
#include <Riggle/Character.h>
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
    ViewportPanel* getViewportPanel() const { return m_viewportPanel; }
    AssetPanel* getAssetPanel() const { return m_assetPanel; }
    AssetBrowserPanel* getAssetBrowserPanel() const { return m_assetBrowserPanel; }
    HierarchyPanel* getHierarchyPanel() const { return m_hierarchyPanel; }
    PropertyPanel* getPropertyPanel() const { return m_propertyPanel; }
    
    // Character access
    Character* getCharacter() const { return m_character.get(); }

private:
    // Panel management
    std::vector<std::unique_ptr<BasePanel>> m_panels;
    ViewportPanel* m_viewportPanel;
    AssetPanel* m_assetPanel;
    AssetBrowserPanel* m_assetBrowserPanel;
    HierarchyPanel* m_hierarchyPanel;
    PropertyPanel* m_propertyPanel;
    
    // Character
    std::unique_ptr<Character> m_character;
    
    // State
    sf::RenderWindow* m_currentWindow;
    bool m_showExitConfirmation = false;
    bool m_shouldExit = false;          
    bool m_hasUnsavedChanges = false;
    
    // Initialization
    void initializePanels();
    void createDefaultCharacter();
    void setupPanelCallbacks();
    
    // Asset management
    void onAssetSelected(const AssetInfo& asset);
    void onMultipleAssetsSelected(const std::vector<AssetInfo>& assets);
    void onSpriteSelected(Sprite* sprite);
    void onSpriteDeleted(Sprite* sprite);
    void onBoneSelected(std::shared_ptr<Bone> bone);
    void onBoneCreated(std::shared_ptr<Bone> bone);
    void onBoneDeleted(std::shared_ptr<Bone> bone);

     // Multi-selection state
    Sprite* m_selectedSprite = nullptr;
    std::shared_ptr<Bone> m_selectedBone = nullptr;
    bool m_hasMultiSelection = false;
    
    // Selection management
    void updateSelectionState(Sprite* sprite, std::shared_ptr<Bone> bone);
    void clearAllSelections();
    
    // Rendering
    void renderMainMenuBar();
    void renderAssetHierarchyTabs();
    void renderExitConfirmation();
    
    // Project management
    void newProject();             
    void saveProject();             
    void loadProject();            
    bool hasUnsavedChanges() const; 
};

} // namespace Riggle