#include "Editor/Panels/HierarchyPanel.h"
#include <imgui.h>
#include <iostream>
#include <algorithm>

namespace Riggle {

HierarchyPanel::HierarchyPanel()
    : BasePanel("Hierarchy")
    , m_character(nullptr)
    , m_selectedBone(nullptr)
    , m_isRenaming(false)
    , m_renamingBone(nullptr)
    , m_showContextMenu(false)
    , m_contextMenuBone(nullptr)
{
}

void HierarchyPanel::render() {
    if (!isVisible()) return;

    if (ImGui::Begin(getName().c_str(), &m_isVisible)) {
        if (m_character && m_character->getRig()) {
            renderBoneHierarchy();
        } else {
            ImGui::Text("No character loaded");
            ImGui::Spacing();
            ImGui::TextWrapped("Load a character or create bones to see the hierarchy.");
        }
        
        // Handle context menu
        if (m_showContextMenu && !m_contextMenuPopupId.empty()) {
            if (m_shouldOpenContextMenu) {
                ImGui::OpenPopup(m_contextMenuPopupId.c_str());
                m_shouldOpenContextMenu = false;
            }
            renderContextMenu(m_contextMenuPopupId);
        }
    }
    ImGui::End();
}

void HierarchyPanel::setSelectedBone(std::shared_ptr<Bone> bone) {
    if (m_selectedBone != bone) {
        m_selectedBone = bone;
        std::cout << "HierarchyPanel: Bone selection changed to: " 
                  << (bone ? bone->getName() : "none") << std::endl;
    }
}

void HierarchyPanel::renderContent() {
    // Render just the content without the ImGui::Begin/End wrapper
    if (m_character && m_character->getRig()) {
        renderBoneHierarchy();
    } else {
        ImGui::Text("No character loaded");
        ImGui::Spacing();
        ImGui::TextWrapped("Load a character or create bones to see the hierarchy.");
    }
    
    // Handle context menu
    if (m_showContextMenu && !m_contextMenuPopupId.empty()) {
        if (m_shouldOpenContextMenu) {
                ImGui::OpenPopup(m_contextMenuPopupId.c_str());
                m_shouldOpenContextMenu = false;
        }
        renderContextMenu(m_contextMenuPopupId);
    }
    // Handle rename modal
    if (m_isRenaming) {
        renderRenameModal();
    }
}

void HierarchyPanel::update(sf::RenderWindow& window) {
    // Handle F2 key for renaming selected bone
    if (ImGui::IsKeyPressed(ImGuiKey_F2) && m_selectedBone && !m_isRenaming) {
        m_renamingBone = m_selectedBone;
        m_renameBuffer = m_selectedBone->getName();
        m_isRenaming = true;
    }
}

void HierarchyPanel::renderBoneHierarchy() {
    const auto& rootBones = m_character->getRig()->getRootBones();
    
    if (rootBones.empty()) {
        ImGui::Text("No bones in hierarchy");
        ImGui::Spacing();
        ImGui::TextWrapped("Use the Bone Creation tool to create bones.");
        return;
    }
    
    ImGui::Text("Bone Hierarchy:");
    ImGui::SameLine();
    ImGui::TextDisabled("(Double-click to rename)");
    ImGui::Separator();
    
    // Render each root bone and its children
    for (const auto& rootBone : rootBones) {
        renderBoneNode(rootBone, 0);
    }
}

void HierarchyPanel::renderBoneNode(std::shared_ptr<Bone> bone, int depth) {
    if (!bone) return;
    
    // Create unique ID for this bone
    std::string nodeId = bone->getName() + "##" + std::to_string(reinterpret_cast<uintptr_t>(bone.get()));
    std::string popupId = "BoneContextMenu##" + std::to_string(reinterpret_cast<uintptr_t>(bone.get()));
    
    // Node flags
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    
    // Highlight selected bone
    bool isSelected = (bone == m_selectedBone);
    if (isSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    // If bone has no children, make it a leaf
    if (bone->getChildren().empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    
    // Get display name with additional info
    std::string displayName = getBoneDisplayName(bone);
    
    // Render tree node
    bool nodeOpen = ImGui::TreeNodeEx(nodeId.c_str(), flags, "%s", displayName.c_str());
    
    // Handle selection
    if (ImGui::IsItemClicked()) {
        setSelectedBone(bone);  // Use the setter to ensure proper updates
        if (m_onBoneSelected) {
            m_onBoneSelected(bone);
        }
        std::cout << "Selected bone from hierarchy: " << bone->getName() << std::endl;
    }

    // Handle double-click for renaming
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        m_renamingBone = bone;
        m_renameBuffer = bone->getName();
        m_isRenaming = true;
    }
    
    // Handle right-click context menu
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        m_contextMenuBone = bone;
        m_showContextMenu = true;
        m_contextMenuPopupId = popupId;
        m_shouldOpenContextMenu = true;
    }
    
    // Render children if node is open
    if (nodeOpen && !bone->getChildren().empty()) {
        for (const auto& child : bone->getChildren()) {
            renderBoneNode(child, depth + 1);
        }
        ImGui::TreePop();
    }
    
}

void HierarchyPanel::renderContextMenu(const std::string& popupId) {
    if (ImGui::BeginPopup(popupId.c_str())) {
        if (m_contextMenuBone) {
            ImGui::Text("Bone: %s", m_contextMenuBone->getName().c_str());
            ImGui::Separator();
            
            if (ImGui::MenuItem("Select")) {
                m_selectedBone = m_contextMenuBone;
                if (m_onBoneSelected) {
                    m_onBoneSelected(m_selectedBone);
                }
            }

            if (ImGui::MenuItem("Rename", "F2")) {
                m_renamingBone = m_contextMenuBone;
                m_renameBuffer = m_contextMenuBone->getName();
                m_isRenaming = true;
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Delete", "Del", false, !m_contextMenuBone->isRoot())) {
                deleteBone(m_contextMenuBone);
            }
            
            if(!m_contextMenuBone->getChildren().empty()){
                ImGui::TextDisabled("Cannot delete bone with children");
            }
            
            if (m_contextMenuBone->isRoot()) {
                ImGui::TextDisabled("Root bones cannot be deleted");
            }
        }
        
        ImGui::EndPopup();
    } else {
        m_showContextMenu = false;
        m_contextMenuBone = nullptr;
        m_contextMenuPopupId.clear();
    }
}

void HierarchyPanel::renderRenameModal() {
    if (!m_renamingBone) {
        m_isRenaming = false;
        return;
    }
    
    ImGui::OpenPopup("Rename Bone");
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 120), ImGuiCond_Appearing);
    
    if (ImGui::BeginPopupModal("Rename Bone", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Rename bone: %s", m_renamingBone->getName().c_str());
        ImGui::Spacing();
        
        // Auto-focus the input field
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        
        // Input field with buffer
        char nameBuffer[64];
        strncpy_s(nameBuffer, m_renameBuffer.c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        
        bool enterPressed = ImGui::InputText("##NewName", nameBuffer, sizeof(nameBuffer), 
                                           ImGuiInputTextFlags_EnterReturnsTrue);
        m_renameBuffer = nameBuffer;
        
        ImGui::Spacing();
        
        // Validation
        bool isValid = isValidBoneName(m_renameBuffer);
        if (!isValid) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            if (m_renameBuffer.empty()) {
                ImGui::Text("Name cannot be empty");
            } else {
                ImGui::Text("Name already exists");
            }
            ImGui::PopStyleColor();
        }
        
        ImGui::Spacing();
        
        // Buttons
        if ((ImGui::Button("Rename") || enterPressed) && isValid) {
            renameBone(m_renamingBone, m_renameBuffer);
            m_isRenaming = false;
            m_renamingBone = nullptr;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_isRenaming = false;
            m_renamingBone = nullptr;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void HierarchyPanel::renameBone(std::shared_ptr<Bone> bone, const std::string& newName) {
    if (!bone || newName.empty() || bone->getName() == newName) return;
    
    std::string oldName = bone->getName();
    bone->setName(newName);
    
    std::cout << "Renamed bone from '" << oldName << "' to '" << newName << "'" << std::endl;
    
    // Notify callback
    if (m_onBoneRenamed) {
        m_onBoneRenamed(bone, oldName);
    }
}

bool HierarchyPanel::isValidBoneName(const std::string& name) const {
    if (name.empty()) return false;
    if (!m_character || !m_character->getRig()) return true;
    
    // Check if name already exists (but allow current bone's name)
    const auto& allBones = m_character->getRig()->getAllBones();
    for (const auto& bone : allBones) {
        if (bone != m_renamingBone && bone->getName() == name) {
            return false;
        }
    }
    
    return true;
}

void HierarchyPanel::deleteBone(std::shared_ptr<Bone> bone) {
    if (!bone || !m_character || !m_character->getRig()) return;
    
    // Root bone can only be deleted if it has no children
    if (!bone->getChildren().empty()) {
        std::cout << "Cannot delete bone with children: " << bone->getName() << std::endl;
        return;
    }
    
    std::cout << "Deleting bone: " << bone->getName() << std::endl;
    
    // Clear selection if deleting selected bone
    if (bone == m_selectedBone) {
        setSelectedBone(nullptr);
        if (m_onBoneSelected) {
            m_onBoneSelected(nullptr);
        }
    }
    
    // Notify callback
    if (m_onBoneDeleted) {
        m_onBoneDeleted(bone);
    }
    
    m_contextMenuBone = nullptr;
    m_showContextMenu = false;
    m_contextMenuPopupId.clear();
}

std::string HierarchyPanel::getBoneDisplayName(std::shared_ptr<Bone> bone) const {
    if (!bone) return "Unknown";
    
    std::string name = bone->getName();
    
    // Add sprite count
    size_t spriteCount = bone->getSpriteCount();
    if (spriteCount > 0) {
        name += " [" + std::to_string(spriteCount) + " sprites]";
    }
    
    // Add root indicator
    if (bone->isRoot()) {
        name += " (Root)";
    }
    
    return name;
}

} // namespace Riggle