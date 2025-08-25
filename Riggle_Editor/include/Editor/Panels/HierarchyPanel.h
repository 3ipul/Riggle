#pragma once
#include "Editor/Panels/BasePanel.h"
#include <Riggle/Character.h>
#include <Riggle/Bone.h>
#include <functional>
#include <memory>
#include <string>

namespace Riggle {

class HierarchyPanel : public BasePanel {
public:
    HierarchyPanel();
    ~HierarchyPanel() = default;

    void render() override;
    void update(sf::RenderWindow& window) override;

    // Character management
    void setCharacter(Character* character) { m_character = character; }
    
    // Selection management
    void setSelectedBone(std::shared_ptr<Bone> bone);
    std::shared_ptr<Bone> getSelectedBone() const { return m_selectedBone; }
    void clearSelection() { setSelectedBone(nullptr); }
    
    // Context menu management
    void setContextMenuBone(std::shared_ptr<Bone> bone) { m_contextMenuBone = bone; m_showContextMenu = true; }
    std::shared_ptr<Bone> getContextMenuBone() const { return m_contextMenuBone; }
    
    // Callbacks
    void setOnBoneSelected(std::function<void(std::shared_ptr<Bone>)> callback) {
        m_onBoneSelected = callback;
    }
    
    void setOnBoneDeleted(std::function<void(std::shared_ptr<Bone>)> callback) {
        m_onBoneDeleted = callback;
    }
    
    void setOnBoneRenamed(std::function<void(std::shared_ptr<Bone>, const std::string&)> callback) {
        m_onBoneRenamed = callback;
    }

private:
    Character* m_character;
    std::shared_ptr<Bone> m_selectedBone;
    
    // Renaming state
    bool m_isRenaming;
    std::shared_ptr<Bone> m_renamingBone;
    std::string m_renameBuffer;
    std::string m_contextMenuPopupId;
    bool m_shouldOpenContextMenu = false;
    
    // Callbacks
    std::function<void(std::shared_ptr<Bone>)> m_onBoneSelected;
    std::function<void(std::shared_ptr<Bone>)> m_onBoneDeleted;
    std::function<void(std::shared_ptr<Bone>, const std::string&)> m_onBoneRenamed;
    
    // UI state
    bool m_showContextMenu;
    std::shared_ptr<Bone> m_contextMenuBone;
    
    // Helper methods
    void renderBoneHierarchy();
    void renderBoneNode(std::shared_ptr<Bone> bone, int depth = 0);
    void renderContextMenu(const std::string& popupId);
    void renderRenameModal();
    void deleteBone(std::shared_ptr<Bone> bone);
    void renameBone(std::shared_ptr<Bone> bone, const std::string& newName);
    std::string getBoneDisplayName(std::shared_ptr<Bone> bone) const;
    bool isValidBoneName(const std::string& name) const;
};

} // namespace Riggle