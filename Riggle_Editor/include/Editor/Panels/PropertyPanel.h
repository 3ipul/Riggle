#pragma once

#include "BasePanel.h"
#include <Riggle/Character.h>
#include <Riggle/Sprite.h>
#include <Riggle/Bone.h>
#include "../Tools/IKSolverTool.h"
#include <functional>
#include <chrono>

namespace Riggle {

enum class PropertyType {
    None,
    Sprite,
    Bone, // For future Hierarchy panel integration
    MultiSelection
};

class PropertyPanel : public BasePanel {
public:
    PropertyPanel();
    ~PropertyPanel() = default;

    void render() override;
    void update(sf::RenderWindow& window) override;

    // Selection management
    void setSelectedSprite(Sprite* sprite);
    void setSelectedBone(std::shared_ptr<Bone> bone);
    void clearSelection();

    // Multi-selection support
    void setMultiSelection(Sprite* sprite, std::shared_ptr<Bone> bone);
    bool hasMultipleTypesSelected() const { return m_selectedSprite && m_selectedBone; }

    Sprite* getSelectedSprite() const { return m_selectedSprite; }
    void setIKTool(IKSolverTool* ikTool) { m_ikTool = ikTool; }

    // Character management
    void setCharacter(Character* character) { m_character = character; }

    bool m_showHelp; // Help dialog state

private:
    Character* m_character;
    PropertyType m_currentType;
    
    // Selected objects
    Sprite* m_selectedSprite;
    std::shared_ptr<Bone> m_selectedBone;
    IKSolverTool* m_ikTool;
    
    // IK error message handling
    std::string m_ikErrorMessage;
    std::chrono::steady_clock::time_point m_ikErrorTime;

    // Movement controls
    float m_moveStep;
    int m_moveStepInt;
    
    // Helper functions
    void renderEmptyState();
    void renderSpriteProperties();
    void renderBoundSpriteInfo();
    void renderHelpDialog();
    void renderTransformControls();
    void renderMovementButtons();
    void renderSpriteBindings();
    void moveSprite(float deltaX, float deltaY);
    void renderIKProperties();

    void renderMultiSelectionProperties();
    void renderBindingControls();

    void renderBoneProperties();
    void renderBoneTransformControls();
    void renderBoneHierarchyInfo();
    void renderBoneSpriteBindings();
};

} // namespace Riggle