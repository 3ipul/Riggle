#pragma once

#include <SFML/Graphics.hpp>
#include "Panels/ViewportPanel.h"
#include "Panels/AssetBrowserPanel.h"
#include "Panels/AssetPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/PropertyPanel.h"
#include "Panels/AnimationPanel.h"
#include "Export/ExportManager.h"
#include "Editor/Utils/FileDialogManager.h"
#include "Editor/Utils/DialogManager.h"
#include "Editor/Project/ProjectManager.h"
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
    bool shouldExit() const;
    
    // Panel access
    ViewportPanel* getViewportPanel() const { return m_viewportPanel; }
    AssetPanel* getAssetPanel() const { return m_assetPanel; }
    AssetBrowserPanel* getAssetBrowserPanel() const { return m_assetBrowserPanel; }
    HierarchyPanel* getHierarchyPanel() const { return m_hierarchyPanel; }
    PropertyPanel* getPropertyPanel() const { return m_propertyPanel; }
    AnimationPanel* getAnimationPanel() const { return m_animationPanel; }

    // Character access
    Character* getCharacter() const { return m_character.get(); }

    // Layout management
    void setupInitialDockLayout(ImGuiID dockspace_id);
    bool isLayoutResetRequested() const;
    void requestLayoutReset() { m_resetLayoutRequested = true; }
    void clearLayoutResetRequest() { m_resetLayoutRequested = false; }

private:
    // Panel management
    std::vector<std::unique_ptr<BasePanel>> m_panels;
    ViewportPanel* m_viewportPanel;
    AssetPanel* m_assetPanel;
    AssetBrowserPanel* m_assetBrowserPanel;
    HierarchyPanel* m_hierarchyPanel;
    PropertyPanel* m_propertyPanel;
    AnimationPanel* m_animationPanel;
    
    // Project management
    std::unique_ptr<ProjectManager> m_projectManager;
    std::string m_currentProjectPath;
    ProjectMetadata m_currentProjectMetadata;
    
    // Character
    std::unique_ptr<Character> m_character;
    
    // State
    sf::RenderWindow* m_currentWindow;
    bool m_shouldExit = false;          
    bool m_hasUnsavedChanges = false;

    // Export and dialog management
    std::unique_ptr<ExportManager> m_exportManager;
    std::unique_ptr<DialogManager> m_dialogManager;
    
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

    // Layout management
    bool m_resetLayoutRequested = false;

    // Project management
    void initializeProjectSystem();
    void newProject();             
    void saveProject();             
    void loadProject(); 
    void updateAllPanelsWithCharacter();           
    bool hasUnsavedChanges() const;
    
    // Export methods
    void initializeExportSystem();
    
    // Dialog callbacks
    void setupDialogCallbacks();
};

} // namespace Riggle