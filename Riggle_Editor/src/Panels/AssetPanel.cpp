#include "Editor/Panels/AssetPanel.h"
#include "Editor/Panels/AssetBrowserPanel.h"
#include <imgui.h>
#include <iostream>

namespace Riggle {

AssetPanel::AssetPanel()
    : BasePanel("Assets")
    , m_character(nullptr)
    , m_selectedSprite(nullptr)
    , m_assetBrowserPanel(nullptr)
    , m_draggedIndex(-1)
    , m_isDragging(false)
{
}

void AssetPanel::render() {
    if (!m_isVisible) return;

    ImGui::Begin(m_name.c_str(), &m_isVisible);

    if (!m_character) {
        ImGui::Text("No character loaded");
        ImGui::End();
        return;
    }

    const auto& sprites = m_character->getSprites();
    
    if (sprites.empty()) {
        renderEmptyState();
    } else {
        renderSpriteList();
    }

    ImGui::End();
}

void AssetPanel::renderContent() {
    // Render just the content without the ImGui::Begin/End wrapper
    if (!m_character) {
        ImGui::Text("No character loaded");
    } else {
        const auto& sprites = m_character->getSprites();
        
        if (sprites.empty()) {
            renderEmptyState();
        } else {
            renderSpriteList();
        }
    }
}

void AssetPanel::update(sf::RenderWindow& window) {
    // No update logic needed for now
}

void AssetPanel::renderEmptyState() {
    ImGui::Spacing();
    ImGui::Text("No sprites loaded");
    ImGui::Spacing();

     // Instructional message
    ImGui::TextWrapped("Use Asset Browser panel to load sprites into your character.");
    ImGui::Spacing();
    
    // Show "Show Asset Browser" button only if Asset Browser is not visible
    if (!isAssetBrowserVisible()) {
        // Center the button
        float buttonWidth = 150.0f;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
        
        if (ImGui::Button("Show Asset Browser", ImVec2(buttonWidth, 40))) {
            if (m_assetBrowserPanel) {
                m_assetBrowserPanel->setVisible(true);
                std::cout << "Asset Browser panel made visible" << std::endl;
            }
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Open the Asset Browser panel to browse and select image files");
        }
    } else {
        // When Asset Browser is visible, show a different message
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f)); // Green text
        ImGui::TextWrapped("Asset Browser is open. Browse and select image files to add them as sprites.");
        ImGui::PopStyleColor();
    }
    
    ImGui::Spacing();
}

void AssetPanel::renderSpriteList() {
    const auto& sprites = m_character->getSprites();
    
    // Header with sprite count and add button
    ImGui::Text("Sprites (%zu)", sprites.size());
    ImGui::SameLine();

    // Show "Show Asset Browser" button only if Asset Browser is not visible
    if (!isAssetBrowserVisible()) {
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 150);
        if (ImGui::Button("Show Asset Browser")) {
            if (m_assetBrowserPanel) {
                m_assetBrowserPanel->setVisible(true);
                std::cout << "Asset Browser panel made visible" << std::endl;
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Open the Asset Browser panel to add more sprites");
        }
    } else {
        // When Asset Browser is visible, show status text
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 120);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f)); // Green text
        ImGui::Text("Asset Browser Open");
        ImGui::PopStyleColor();
    }
    
    ImGui::Separator();
    
    // Sprite list with drag/drop reordering
    if (ImGui::BeginChild("SpriteList", ImVec2(0, 0), true)) {
        for (size_t i = 0; i < sprites.size(); ++i) {
            renderSpriteItem(sprites[i].get(), i);
        }
        
        // Handle drag and drop completion
        if (m_isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_isDragging = false;
            m_draggedIndex = -1;
        }
    }
    ImGui::EndChild();
}

bool AssetPanel::isAssetBrowserVisible() const {
    return m_assetBrowserPanel && m_assetBrowserPanel->isVisible();
}

void AssetPanel::renderSpriteItem(Sprite* sprite, size_t index) {
    const auto& sprites = m_character->getSprites();
    bool isSelected = (sprite == m_selectedSprite);
    
    // Create unique ID for this item
    ImGui::PushID(static_cast<int>(index));
    
    // Get current cursor position
    ImVec2 startPos = ImGui::GetCursorPos();
    float windowWidth = ImGui::GetContentRegionAvail().x;
    float itemHeight = 30.0f;
    float buttonHeight = 23.0f;
    float buttonSpacing = 2.0f;
    
    // Calculate button widths and positions (from right to left)
    float deleteButtonWidth = 50.0f;  // "Delete"
    float upDownButtonWidth = 35.0f;  // "Up"/"Down" text buttons
    float visibilityButtonWidth = 40.0f; // "Show"/"Hide"
    
    float deleteButtonX = windowWidth - deleteButtonWidth;
    float downButtonX = deleteButtonX - upDownButtonWidth - buttonSpacing;
    float upButtonX = downButtonX - upDownButtonWidth - buttonSpacing;
    float visibilityButtonX = upButtonX - visibilityButtonWidth - buttonSpacing;
    float nameAreaWidth = visibilityButtonX - 10.0f; // Leave some padding
    
    // Draw selection background ONLY for name area (not full width)
    ImVec2 nameAreaMin = ImGui::GetCursorScreenPos();
    ImVec2 nameAreaMax = ImVec2(nameAreaMin.x + nameAreaWidth, nameAreaMin.y + itemHeight);
    
    if (isSelected) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            nameAreaMin, nameAreaMax, 
            IM_COL32(31, 86, 60, 255) // selection background only on name area
        );
    }
    
    // Sprite name area (clickable for selection/deselection)
    ImGui::SetCursorPos(ImVec2(startPos.x + 5, startPos.y + 2));
    
    std::string displayName = sprite->getName();
    if (displayName.empty()) {
        displayName = "Sprite " + std::to_string(index);
    }
    
    // Invisible button for name selection area
    ImGui::InvisibleButton("##SelectArea", ImVec2(nameAreaWidth - 10, itemHeight - 4));

    // Handle drag and drop AFTER the button is created
    handleDragAndDrop(index);

    if (ImGui::IsItemClicked()) {
        // TOGGLE SELECTION: If already selected, unselect. If not selected, select.
        if (m_selectedSprite == sprite) {
            // Unselect if clicking the same sprite
            m_selectedSprite = nullptr;
            if (m_onSpriteSelected) {
                m_onSpriteSelected(nullptr);
            }
            std::cout << "Unselected sprite: " << displayName << std::endl;
        } else {
            // Select new sprite
            m_selectedSprite = sprite;
            if (m_onSpriteSelected) {
                m_onSpriteSelected(m_selectedSprite);
            }
            std::cout << "Selected sprite: " << displayName << std::endl;
        }
    }
    
    // Draw the text over the invisible button
    ImGui::SetCursorPos(ImVec2(startPos.x + 8, startPos.y + 7));
    ImGui::Text("%s", displayName.c_str());
    
    // Position and draw buttons with consistent height
    
    // Visibility toggle button
    ImGui::SetCursorPos(ImVec2(startPos.x + visibilityButtonX, startPos.y + 3.5f));
    bool isVisible = sprite->isVisible();
    
    // Set button color based on visibility state
    if (isVisible) {
        // Green color for visible sprites
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.80f, 0.20f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.80f, 0.30f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.60f, 0.15f, 1.00f));
    } else {
        // Gray color for hidden sprites
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.50f, 0.50f, 0.40f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.60f, 0.60f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.40f, 0.40f, 0.40f, 1.00f));
    }
    
    const char* visibilityText = isVisible ? "Show" : "Hide";
    if (ImGui::Button(visibilityText, ImVec2(visibilityButtonWidth, buttonHeight))) {
        toggleSpriteVisibility(index);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(isVisible ? "Click to hide sprite" : "Click to show sprite");
    }
    
    ImGui::PopStyleColor(3);
    
    // Up button - now with text
    ImGui::SetCursorPos(ImVec2(startPos.x + upButtonX, startPos.y + 3.5f));
    if (ImGui::Button("Up", ImVec2(upDownButtonWidth, buttonHeight))) {
        moveSpriteUp(index);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Move sprite up (forward in draw order)");
    }
    
    // Down button - now with text
    ImGui::SetCursorPos(ImVec2(startPos.x + downButtonX, startPos.y + 3.5f));
    if (ImGui::Button("Down", ImVec2(upDownButtonWidth, buttonHeight))) {
        moveSpriteDown(index);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Move sprite down (backward in draw order)");
    }
    
    // Delete button with red color
    ImGui::SetCursorPos(ImVec2(startPos.x + deleteButtonX, startPos.y + 3.5f));
    
    // Set red color for delete button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.20f, 0.20f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.30f, 0.30f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.15f, 0.15f, 1.00f));
    
    if (ImGui::Button("Delete", ImVec2(deleteButtonWidth, buttonHeight))) {
        deleteSprite(index);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Delete sprite permanently");
    }
    
    ImGui::PopStyleColor(3);
    
    // Enhanced tooltip when hovering over name area
    ImGui::SetCursorPos(ImVec2(startPos.x + 5, startPos.y + 2));
    ImGui::InvisibleButton("##TooltipArea", ImVec2(nameAreaWidth - 10, itemHeight - 4));
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Name: %s", sprite->getName().c_str());
        ImGui::Text("Path: %s", sprite->getTexturePath().c_str());
        Transform transform = sprite->getLocalTransform();
        ImGui::Text("Position: (%.1f, %.1f)", transform.position.x, transform.position.y);
        ImGui::Text("Visible: %s", sprite->isVisible() ? "Yes" : "No");
        if (sprite->isBoundToBone()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "(Bound)");
            
            // Show which bone it's bound to
            if (auto boundBone = sprite->getBoundBone()) {
                ImGui::SameLine();
                ImGui::Text("-> %s", boundBone->getName().c_str());
            }
        }
        ImGui::Separator();
        // Add selection instruction to tooltip
        if (isSelected) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.667f, 1.0f), "Click to unselect");
        } else {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.667f, 1.0f), "Click to select");
        }
        ImGui::EndTooltip();
    }
    
    // Move cursor to next item position
    ImGui::SetCursorPos(ImVec2(startPos.x, startPos.y + itemHeight));
    
    ImGui::PopID();
}

void AssetPanel::handleDragAndDrop(size_t index) {
    // Use ImGui's built-in drag and drop
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        // Set payload to carry the index of our sprite
        ImGui::SetDragDropPayload("SPRITE_REORDER", &index, sizeof(size_t));
        
        // Display preview
        std::string displayName = m_character->getSprites()[index]->getName();
        if (displayName.empty()) {
            displayName = "Sprite " + std::to_string(index);
        }
        ImGui::Text("Moving: %s", displayName.c_str());
        
        ImGui::EndDragDropSource();
    }
    
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE_REORDER")) {
            size_t sourceIndex = *(const size_t*)payload->Data;
            size_t targetIndex = index;
            
            if (sourceIndex != targetIndex && m_character) {
                std::cout << "Reordering sprite from " << sourceIndex << " to " << targetIndex << std::endl;
                
                auto& sprites = m_character->getSprites();
                
                if (sourceIndex < sprites.size() && targetIndex < sprites.size()) {
                    // Simple approach: extract sprite and insert at new position
                    auto sprite = std::move(sprites[sourceIndex]);
                    sprites.erase(sprites.begin() + sourceIndex);
                    
                    // Adjust target index if we removed an element before it
                    if (sourceIndex < targetIndex) {
                        targetIndex--;
                    }
                    
                    sprites.insert(sprites.begin() + targetIndex, std::move(sprite));
                    
                    // Force update deformations
                    m_character->forceUpdateDeformations();
                    
                    std::cout << "Successfully reordered sprites" << std::endl;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void AssetPanel::moveSpriteUp(size_t index) {
    if (index > 0 && m_character) {
        auto& sprites = m_character->getSprites();
        if (index < sprites.size()) {
            std::swap(sprites[index], sprites[index - 1]);
            m_character->forceUpdateDeformations(); // Force update after reorder
            std::cout << "Moved sprite up from index " << index << " to " << (index - 1) << std::endl;
        }
    }
}

void AssetPanel::moveSpriteDown(size_t index) {
    if (m_character) {
        auto& sprites = m_character->getSprites();
        if (index < sprites.size() - 1) {
            std::swap(sprites[index], sprites[index + 1]);
            m_character->forceUpdateDeformations(); // Force update after reorder
            std::cout << "Moved sprite down from index " << index << " to " << (index + 1) << std::endl;
        }
    }
}

void AssetPanel::deleteSprite(size_t index) {
    if (!m_character) return;
    
    const auto& sprites = m_character->getSprites();
    if (index >= sprites.size()) return;
    
    Sprite* spriteToDelete = sprites[index].get();
    std::string spriteName = spriteToDelete->getName();
    
    // Clear selection if deleting selected sprite
    if (m_selectedSprite == spriteToDelete) {
        m_selectedSprite = nullptr;
        if (m_onSpriteSelected) {
            m_onSpriteSelected(nullptr);
        }
    }
    
    // Notify about deletion
    if (m_onSpriteDeleted) {
        m_onSpriteDeleted(spriteToDelete);
    }
    
    // Remove from character using the new method
    m_character->removeSpriteAt(index);
    
    std::cout << "Deleted sprite: " << spriteName << " at index " << index << std::endl;
}

void AssetPanel::toggleSpriteVisibility(size_t index) {
    if (!m_character) return;
    
    const auto& sprites = m_character->getSprites();
    if (index >= sprites.size()) return;
    
    Sprite* sprite = sprites[index].get();
    bool newVisibility = !sprite->isVisible();
    sprite->setVisible(newVisibility);
    
    // Force update to reflect visibility change
    m_character->forceUpdateDeformations();
    
    std::cout << "Toggled visibility for sprite '" << sprite->getName() 
              << "' to " << (newVisibility ? "visible" : "hidden") << std::endl;
}

} // namespace Riggle