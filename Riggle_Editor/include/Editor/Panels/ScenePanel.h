#pragma once
#include "BasePanel.h"
#include <Riggle/Character.h>
#include "../Render/SpriteRenderer.h"
#include "../Render/BoneRenderer.h"
#include "../AssetManager.h"
#include "../Tools/BoneCreationTool.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <functional>

namespace Riggle {

enum class SceneToolMode {
    SpriteManipulation = 0,
    BoneCreation = 1,
    BoneSelection = 2,
    BoneRotation = 3,
    SpriteBinding = 4,
    IKSolver = 5 
};

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

    // Tool management
    SceneToolMode getToolMode() const { return m_toolMode; }
    void setToolMode(SceneToolMode mode);

    // Selection callbacks
    void setOnSpriteSelected(std::function<void(Sprite*)> callback) {
        m_onSpriteSelected = callback;
    }
    
    void setOnBoneSelected(std::function<void(std::shared_ptr<Bone>)> callback) {
        m_onBoneSelected = callback;
    }

    void setOnBoneRotated(std::function<void(std::shared_ptr<Bone>, float)> callback) {
        m_onBoneRotated = callback;
    }

    // NEW: Binding callbacks
    void setOnSpriteBindingChanged(std::function<void(Sprite*, std::shared_ptr<Bone>)> callback) {
        m_onSpriteBindingChanged = callback;
    }

    // Rendering
    void renderScene(sf::RenderTarget& target);

private:
    std::unique_ptr<Character> m_character;
    std::unique_ptr<SpriteRenderer> m_spriteRenderer;
    std::unique_ptr<BoneRenderer> m_boneRenderer;
    std::unique_ptr<BoneCreationTool> m_boneCreationTool;

    // View and interaction
    sf::View m_view;
    sf::RenderWindow* m_window;
    
    // Tool mode
    SceneToolMode m_toolMode;
    
    // Mouse interaction
    bool m_isDragging;
    bool m_isPanning;
    sf::Vector2f m_lastMousePos;
    sf::Vector2f m_dragOffset;
    Sprite* m_draggedSprite;
    Sprite* m_selectedSprite;
    std::shared_ptr<Bone> m_selectedBone;

    // NEW: Binding state
    bool m_showBindingUI;
    Sprite* m_spriteToBindFrom;
    std::shared_ptr<Bone> m_boneToBindFrom;

    // Viewport
    sf::RenderTexture m_renderTexture;
    bool m_viewportInitialized;
    sf::Vector2u m_viewportSize;

    // Callbacks
    std::function<void(Sprite*)> m_onSpriteSelected;
    std::function<void(std::shared_ptr<Bone>)> m_onBoneSelected;
    std::function<void(std::shared_ptr<Bone>, float)> m_onBoneRotated;
    std::function<void(Sprite*, std::shared_ptr<Bone>)> m_onSpriteBindingChanged;

    // Display options
    bool m_showBones;
    bool m_showSprites;
    bool m_showGrid;
    bool m_showBindings;

    // IK-specific members
    bool m_ikMode;
    std::shared_ptr<Bone> m_ikTarget;
    sf::Vector2f m_ikTargetPos;
    bool m_ikDragging;

    // Helper functions
    sf::Vector2f screenToWorld(const sf::Vector2f& screenPos, const sf::Vector2f& viewportSize) const;
    void handleMouseInput(const sf::Vector2f& worldPos, const sf::Vector2f& screenPos, const sf::Vector2f& viewportSize);
    void handleSpriteInteraction(const sf::Vector2f& worldPos);
    void handleBoneInteraction(const sf::Vector2f& worldPos);
    void handleBindingInteraction(const sf::Vector2f& worldPos);  // NEW
    Sprite* getSpriteAtPosition(const sf::Vector2f& worldPos);
    bool isPointInQuad(const sf::Vector2f& point, const std::vector<Vertex>& vertices) const;
    void updateSpriteDragging();
    void renderGrid(sf::RenderTarget& target);
    void renderSceneControls();
    void renderBoneCreationPreview(sf::RenderTarget& target);
    void renderBoneRotationPreview(sf::RenderTarget& target);
    void renderBoneHandles(sf::RenderTarget& target);
    void renderBindingConnections(sf::RenderTarget& target);  // NEW
    void renderBindingUI();  // NEW
    void initializeViewport(const sf::Vector2u& size);
    void handleEscapeKey();

    // NEW: Binding helper functions
    void fitViewToContent();
    void bindSpriteToSelectedBone();
    void unbindSelectedSprite();
    void showSpriteContextMenu(Sprite* sprite, const sf::Vector2f& mousePos);
    void showBoneContextMenu(std::shared_ptr<Bone> bone, const sf::Vector2f& mousePos);

    // NEW IK methods
    void handleIKInteraction(const sf::Vector2f& worldPos);
    void renderIKTargets(sf::RenderTarget& target);
    void renderIKUI();
};

} // namespace Riggle