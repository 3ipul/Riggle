#pragma once
#include "BasePanel.h"
#include <Riggle/Character.h>
#include "../Render/SpriteRenderer.h"
#include "../Render/BoneRenderer.h"
#include "../Tools/BoneCreationTool.h"
#include "../Tools/SpriteManipulationTool.h"
#include "../Tools/IKSolverTool.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <functional>

namespace Riggle {

enum class ViewportTool {
    SpriteTool,
    BoneTool
};

enum class BoneSubTool {
    CreateBone,
    BoneTransform,
    IKSolver
};

class ViewportPanel : public BasePanel {
public:
    ViewportPanel();
    ~ViewportPanel() = default;

    void render() override;
    void update(sf::RenderWindow& window) override;
    void handleEvent(const sf::Event& event) override;

    // Character management
    void setCharacter(Character* character);
    Character* getCharacter() const { return m_character; }

    // Tool management
    ViewportTool getCurrentTool() const { return m_currentTool; }
    BoneSubTool getCurrentBoneSubTool() const { return m_currentBoneSubTool; }
    void setTool(ViewportTool tool);
    void setBoneSubTool(BoneSubTool subTool);
    const char* getToolName() const;
    IKSolverTool* getIKTool() const { return m_ikTool.get(); }

    // Selection
    Sprite* getSelectedSprite() const { return m_selectedSprite; }
    std::shared_ptr<Bone> getSelectedBone() const { return m_selectedBone; }
    void setSelectedSprite(Sprite* sprite) { m_selectedSprite = sprite; }
    void setSelectedBone(std::shared_ptr<Bone> bone);

    // View controls
    void resetView();
    sf::View& getView() { return m_view; }
    float getZoomLevel() const;
    bool isInCtrlHoverMode() const { return m_ctrlHoverMode; }

    // Callbacks
    void setOnSpriteSelected(std::function<void(Sprite*)> callback) {
        m_onSpriteSelected = callback;
    }
    
    void setOnBoneSelected(std::function<void(std::shared_ptr<Bone>)> callback) {
        m_onBoneSelected = callback;
    }
    
    void setOnBoneCreated(const std::function<void(std::shared_ptr<Bone>)>& callback) {
        m_onBoneCreated = callback;
    }

    void setOnBoneRotated(std::function<void(std::shared_ptr<Bone>, float)> callback) {
        m_onBoneRotated = callback;
    }

private:
    Character* m_character;
    std::unique_ptr<SpriteRenderer> m_spriteRenderer;
    std::unique_ptr<BoneRenderer> m_boneRenderer;
    std::unique_ptr<BoneCreationTool> m_boneTool;           
    std::unique_ptr<SpriteManipulationTool> m_spriteTool;
    std::unique_ptr<IKSolverTool> m_ikTool;

    // Viewport
    sf::RenderTexture m_renderTexture;
    sf::View m_view;
    sf::Vector2u m_viewportSize;
    bool m_viewportInitialized;

    // Current tool and interaction
    ViewportTool m_currentTool;
    bool m_isDragging;
    sf::Vector2f m_dragOffset;
    bool m_isPanning;
    sf::Vector2f m_panStartPos;

    BoneSubTool m_currentBoneSubTool;
    bool m_showBoneSubTools;

    // Selection
    Sprite* m_selectedSprite;
    std::shared_ptr<Bone> m_selectedBone;

     // For Ctrl+hover functionality
    bool m_ctrlHoverMode = false;
    Sprite* m_previouslySelectedSprite = nullptr; // To restore selection when Ctrl released

    // Display options
    bool m_showGrid;
    bool m_showBones;
    bool m_showSprites;

    // Callbacks
    std::function<void(Sprite*)> m_onSpriteSelected;
    std::function<void(std::shared_ptr<Bone>)> m_onBoneSelected;
    std::function<void(std::shared_ptr<Bone>)> m_onBoneCreated;
    std::function<void(std::shared_ptr<Bone>, float)> m_onBoneRotated;
    std::function<void(const std::string&)> m_onBoneTransformed;
    
    // Rendering
    void renderToolButtons();
    void renderViewport();
    void renderScene(sf::RenderTarget& target);
    void renderGrid(sf::RenderTarget& target);
    void renderToolOverlays(sf::RenderTarget& target);
    void drawBoneToolOverlay();

    // Interaction
    void handleViewportInteraction(const sf::Vector2f& worldPos, const sf::Vector2f& screenPos);
    sf::Vector2f screenToWorld(const sf::Vector2f& screenPos) const;
    std::shared_ptr<Bone> getBoneAtPosition(const sf::Vector2f& worldPos);
    Sprite* getSpriteAtPosition(const sf::Vector2f& worldPos);
    void handleSpriteManipulation(const sf::Vector2f& worldPos);
    void handleBoneCreation(const sf::Vector2f& worldPos);
    void handleBoneTransform(const sf::Vector2f& worldPos);
    void handleBoneRotation(const sf::Vector2f& worldPos);
    void handleIKSolver(const sf::Vector2f& worldPos);
    
    // Initialization
    void initializeViewport(const sf::Vector2u& size);
    void setupTools();
};

} // namespace Riggle