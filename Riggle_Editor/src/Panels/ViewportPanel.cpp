#include "Editor/Panels/ViewportPanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <algorithm>

namespace Riggle {

ViewportPanel::ViewportPanel()
    : BasePanel("Viewport")
    , m_character(nullptr)
    , m_currentTool(ViewportTool::SpriteTool)
    , m_viewportInitialized(false)
    , m_isPanning(false)
    , m_selectedSprite(nullptr)
    , m_selectedBone(nullptr)
    , m_isDragging(false)
    , m_dragOffset(0.0f, 0.0f)
    , m_showGrid(true)
    , m_showBones(true)
    , m_showSprites(true)
{
    m_spriteRenderer = std::make_unique<SpriteRenderer>();
    m_boneRenderer = std::make_unique<BoneRenderer>();
    m_boneTool = std::make_unique<BoneTool>();
    m_spriteTool = std::make_unique<SpriteTool>();
    
    setupTools();
}

void ViewportPanel::render() {
    if (!isVisible()) return;

    if (ImGui::Begin(getName().c_str(), &m_isVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        // Tool selection buttons (fixed height)
        renderToolButtons();

        ImGui::Separator();
        
        // Display options (fixed height)
        ImGui::SameLine();
        ImGui::Checkbox("Grid", &m_showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Bones", &m_showBones);
        ImGui::SameLine();
        ImGui::Checkbox("Sprites", &m_showSprites);
        
        ImGui::Separator();
        
        // Calculate available space for viewport (subtract fixed UI elements)
        ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        if (contentRegion.x > 10 && contentRegion.y > 10) { // Minimum size check
            
            // Create child window for viewport with proper flags
            if (ImGui::BeginChild("ViewportChild", contentRegion, false, 
                                 ImGuiWindowFlags_NoScrollbar | 
                                 ImGuiWindowFlags_NoScrollWithMouse |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize)) {
                
                // Get the exact size of the child window
                ImVec2 childSize = ImGui::GetContentRegionAvail();
                sf::Vector2u newSize(static_cast<unsigned int>(std::max(1.0f, childSize.x)), 
                                    static_cast<unsigned int>(std::max(1.0f, childSize.y)));

                if (!m_viewportInitialized || newSize != m_viewportSize) {
                    initializeViewport(newSize);
                }

                if (m_viewportInitialized) {
                    renderViewport();

                    // Handle interactions
                    if (ImGui::IsItemHovered() && ImGui::IsWindowHovered()) {
                        ImVec2 mousePos = ImGui::GetMousePos();
                        ImVec2 imagePos = ImGui::GetItemRectMin();
                        
                        sf::Vector2f relativeMousePos(
                            mousePos.x - imagePos.x,
                            mousePos.y - imagePos.y
                        );

                        // Ensure mouse is within viewport bounds
                        if (relativeMousePos.x >= 0 && relativeMousePos.x <= static_cast<float>(m_viewportSize.x) &&
                            relativeMousePos.y >= 0 && relativeMousePos.y <= static_cast<float>(m_viewportSize.y)) {
                            
                            sf::Vector2f worldPos = screenToWorld(relativeMousePos);
                            handleViewportInteraction(worldPos, relativeMousePos);
                        }
                    }
                }
            }
            // Draw the overlay on top of the viewport image
            if (m_currentTool == ViewportTool::BoneTool) {
                drawBoneToolOverlay();
            }

            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
        
        // Status bar (fixed height)
        if (m_viewportInitialized) {
            ImGui::Separator();
            ImGui::Text("Tool: %s", getToolName());
            if (m_selectedSprite) {
                ImGui::SameLine();
                ImGui::Text("| Selected Sprite: %s", m_selectedSprite->getName().c_str());
            }
            if (m_selectedBone) {
                ImGui::SameLine();
                ImGui::Text("| Selected Bone: %s", m_selectedBone->getName().c_str());
            }
        }
    }

    ImGui::End();
}

void ViewportPanel::renderToolButtons() {
    // Tool selection buttons
    const char* toolNames[] = {
        "Sprite Tool",
        "Bone Tool", 
    };
    
    ViewportTool tools[] = {
        ViewportTool::SpriteTool,
        ViewportTool::BoneTool,
    };
    
    for (int i = 0; i < 2; ++i) {
        if (i > 0) ImGui::SameLine();
        
        bool isSelected = (m_currentTool == tools[i]);
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.137f, 0.514f, 0.353f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.106f, 0.255f, 0.180f, 1.0f));
        }
        
        if (ImGui::Button(toolNames[i])) {
            setTool(tools[i]);
        }
        
        ImGui::PopStyleColor();
        
    }
}

void ViewportPanel::drawBoneToolOverlay() {
    // We will draw directly into the current window (the Viewport),
    // but we'll set the cursor position to create an overlay effect.
    const float padding = 10.0f;
    ImGui::SetCursorPos(ImVec2(padding, padding));

    // Use a helper lambda for clean, consistent button drawing
    auto subToolButton = [&](const char* label, BoneSubTool tool) {
        bool isActive = (getCurrentBoneSubTool() == tool);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.137f, 0.514f, 0.353f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.106f, 0.255f, 0.180f, 1.0f));
        }

        if (ImGui::Button(label, ImVec2(100, 0))) {
            setBoneSubTool(tool);
        }

        ImGui::PopStyleColor();
    };

    // Group the buttons together so they don't affect layout below them
    ImGui::BeginGroup();
    subToolButton("Create", BoneSubTool::CreateBone);
    subToolButton("Transform", BoneSubTool::BoneTransform);
    subToolButton("IK Solver", BoneSubTool::IKSolver);
    ImGui::EndGroup();
}

void ViewportPanel::update(sf::RenderWindow& window) {
    // No parameterless update needed
}

void ViewportPanel::handleEvent(const sf::Event& event) {
    if (!m_viewportInitialized) return;

    // Handle Ctrl key state changes
    if (event.is<sf::Event::KeyPressed>()) {
        const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
        if (keyPressed && (keyPressed->code == sf::Keyboard::Key::LControl ||
                          keyPressed->code == sf::Keyboard::Key::RControl)) {
            
            if (!m_ctrlHoverMode) {
                // Entering Ctrl+hover mode
                m_ctrlHoverMode = true;
                m_previouslySelectedSprite = m_selectedSprite; // Remember current selection
                std::cout << "Entered Ctrl+hover mode" << std::endl;
            }
        }
    }
    
    if (event.is<sf::Event::KeyReleased>()) {
        const auto* keyReleased = event.getIf<sf::Event::KeyReleased>();
        if (keyReleased && (keyReleased->code == sf::Keyboard::Key::LControl ||
                           keyReleased->code == sf::Keyboard::Key::RControl)) {
            
            if (m_ctrlHoverMode) {
                // Exiting Ctrl+hover mode - restore previous selection
                m_ctrlHoverMode = false;
                m_selectedSprite = m_previouslySelectedSprite;
                
                // Notify about selection change
                if (m_onSpriteSelected) {
                    m_onSpriteSelected(m_selectedSprite);
                }
                
                m_previouslySelectedSprite = nullptr;
                std::cout << "Exited Ctrl+hover mode, restored previous selection" << std::endl;
            }
        }
    }
}

void ViewportPanel::setCharacter(Character* character) {
    m_character = character;
    
    if (m_spriteRenderer) {
        m_spriteRenderer->setCharacter(character);
    }
    if (m_boneRenderer) {
        m_boneRenderer->setCharacter(character);
    }
    if (m_boneTool) {
        m_boneTool->setCharacter(character);
        if (m_boneTool->getIKTool()) {
            m_boneTool->getIKTool()->setCharacter(character);
        }
    }
    if (m_spriteTool) {
        m_spriteTool->setCharacter(character);
    }
}

void ViewportPanel::setTool(ViewportTool tool) {
    if (m_currentTool == tool) return;
    
    if (m_spriteTool) {
        m_spriteTool->setActive(false);
    }
    if (m_boneTool) {
        m_boneTool->setActive(false);
    }
    
    m_currentTool = tool;
    
    switch (tool) {
        case ViewportTool::SpriteTool:
            if (m_spriteTool) {
                m_spriteTool->setActive(true);
            }
            break;
            
        case ViewportTool::BoneTool:
            if (m_boneTool) {
                m_boneTool->setActive(true);
            }
            break;
    }
    
    std::cout << "Switched to tool: " << getToolName() << std::endl;
}

void ViewportPanel::resetView() {
    if (!m_viewportInitialized) return;
    
    m_view.setCenter(sf::Vector2f(0, 0));
    m_view.setSize(static_cast<sf::Vector2f>(m_viewportSize));
    std::cout << "View reset to center" << std::endl;
}

void ViewportPanel::initializeViewport(const sf::Vector2u& size) {
    if (size.x == 0 || size.y == 0) return;

    m_viewportSize = size;
    
    if (!m_renderTexture.resize(size)) {
        std::cerr << "Failed to create render texture of size " << size.x << "x" << size.y << std::endl;
        return;
    }

    m_view.setSize(static_cast<sf::Vector2f>(size));
    m_view.setCenter(sf::Vector2f(0, 0));

    m_viewportInitialized = true;
    std::cout << "Viewport initialized: " << size.x << "x" << size.y << std::endl;
}

void ViewportPanel::renderViewport() {
    if (!m_viewportInitialized) return;

    m_renderTexture.clear(sf::Color(45, 45, 48));
    m_renderTexture.setView(m_view);
    
    renderScene(m_renderTexture);
    
    m_renderTexture.display();
    
    // Fix: Use proper ImGui::Image call with texture ID and size
    const sf::Texture& texture = m_renderTexture.getTexture();
    void* textureID = reinterpret_cast<void*>(static_cast<uintptr_t>(texture.getNativeHandle()));
    
    ImGui::Image(textureID,
                ImVec2(static_cast<float>(m_viewportSize.x), static_cast<float>(m_viewportSize.y)),
                ImVec2(0, 1), ImVec2(1, 0));
}

void ViewportPanel::renderScene(sf::RenderTarget& target) {
    if (!m_character) return;

    if (m_showGrid) {
        renderGrid(target);
    }

    // Get current zoom level
    float zoomLevel = getZoomLevel();

    if (m_showSprites && m_spriteRenderer) {
        m_spriteRenderer->render(target);
        
        // Render selection highlight
        if (m_selectedSprite) {
            m_spriteRenderer->renderSpriteHighlight(target, m_selectedSprite);
        }
    }

    if (m_showBones && m_boneRenderer) {
        m_boneRenderer->render(target, zoomLevel);
        
        if (m_selectedBone) {
            m_boneRenderer->renderBoneHighlight(target, m_selectedBone, zoomLevel);
        }
    }

    renderToolOverlays(target);
}

void ViewportPanel::renderGrid(sf::RenderTarget& target) {
    const float gridSpacing = 50.0f;
    const sf::Color gridColor(100, 100, 100, 100);
    
    sf::View currentView = target.getView();
    sf::Vector2f viewCenter = currentView.getCenter();
    sf::Vector2f viewSize = currentView.getSize();
    
    float left = viewCenter.x - viewSize.x * 0.5f;
    float right = viewCenter.x + viewSize.x * 0.5f;
    float top = viewCenter.y - viewSize.y * 0.5f;
    float bottom = viewCenter.y + viewSize.y * 0.5f;
    
    left = std::floor(left / gridSpacing) * gridSpacing;
    right = std::ceil(right / gridSpacing) * gridSpacing;
    top = std::floor(top / gridSpacing) * gridSpacing;
    bottom = std::ceil(bottom / gridSpacing) * gridSpacing;
    
    std::vector<sf::Vertex> gridLines;
    
    // Vertical lines
    for (float x = left; x <= right; x += gridSpacing) {
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(x, top);
        v1.color = gridColor;
        v2.position = sf::Vector2f(x, bottom);
        v2.color = gridColor;
        gridLines.push_back(v1);
        gridLines.push_back(v2);
    }
    
    // Horizontal lines
    for (float y = top; y <= bottom; y += gridSpacing) {
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(left, y);
        v1.color = gridColor;
        v2.position = sf::Vector2f(right, y);
        v2.color = gridColor;
        gridLines.push_back(v1);
        gridLines.push_back(v2);
    }
    
    if (!gridLines.empty()) {
        target.draw(&gridLines[0], gridLines.size(), sf::PrimitiveType::Lines);
    }
    
    // Draw axes
    sf::Color axisColor(150, 150, 150, 200);
    std::vector<sf::Vertex> axisLines;
    
    if (top <= 0 && bottom >= 0) {
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(left, 0);
        v1.color = axisColor;
        v2.position = sf::Vector2f(right, 0);
        v2.color = axisColor;
        axisLines.push_back(v1);
        axisLines.push_back(v2);
    }
    
    if (left <= 0 && right >= 0) {
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(0, top);
        v1.color = axisColor;
        v2.position = sf::Vector2f(0, bottom);
        v2.color = axisColor;
        axisLines.push_back(v1);
        axisLines.push_back(v2);
    }
    
    if (!axisLines.empty()) {
        target.draw(&axisLines[0], axisLines.size(), sf::PrimitiveType::Lines);
    }
}

void ViewportPanel::renderToolOverlays(sf::RenderTarget& target) {
    if (m_currentTool == ViewportTool::SpriteTool && m_spriteTool) {
        m_spriteTool->renderOverlay(target);
    }
    
    if (m_currentTool == ViewportTool::BoneTool && m_boneTool) {
        float zoomLevel = getZoomLevel();
        m_boneTool->renderOverlay(target, zoomLevel); // BoneTool handles all sub-tools
    }
}

void ViewportPanel::handleViewportInteraction(const sf::Vector2f& worldPos, const sf::Vector2f& screenPos) {
    // Handle panning with right/middle mouse ONLY
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        m_isPanning = true;
        m_panStartPos = screenPos;
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) || ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
        m_isPanning = false;
    }
    
    // Handle panning movement - ONLY with right/middle mouse
    if (m_isPanning && (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle))) {
        sf::Vector2f currentScreenPos = screenPos;
        sf::Vector2f screenDelta = m_panStartPos - currentScreenPos;
        
        sf::Vector2f viewSize = m_view.getSize();
        sf::Vector2f worldDelta;
        worldDelta.x = screenDelta.x * (viewSize.x / static_cast<float>(m_viewportSize.x));
        worldDelta.y = screenDelta.y * (viewSize.y / static_cast<float>(m_viewportSize.y));
        
        m_view.move(worldDelta);
        m_panStartPos = currentScreenPos;
    }
    
    // Handle zoom
    ImGuiIO& io = ImGui::GetIO();
    float wheel = io.MouseWheel;
    if (wheel != 0.0f) {
        float zoomFactor = wheel > 0 ? 0.9f : 1.1f;
        m_view.zoom(zoomFactor);
    }

    // Handle Ctrl+hover sprite auto-selection
    if (m_ctrlHoverMode) {
        Sprite* hoveredSprite = getSpriteAtPosition(worldPos);
        
        if (hoveredSprite != m_selectedSprite) {
            m_selectedSprite = hoveredSprite;
            
            // Notify about selection change
            if (m_onSpriteSelected) {
                m_onSpriteSelected(m_selectedSprite);
            }
        }
    }

    // Don't handle tool interactions if we're panning
    if (m_isPanning) {
        return;
    }

    switch (m_currentTool) {
        case ViewportTool::SpriteTool:
            handleSpriteManipulation(worldPos);
            break;
            
        case ViewportTool::BoneTool:
            // Delegate ALL bone handling to BoneTool
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                m_boneTool->handleMousePressed(worldPos);
            } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                m_boneTool->handleMouseMoved(worldPos);
            } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                m_boneTool->handleMouseReleased(worldPos);
            }
            break;
    }
}

void ViewportPanel::handleSpriteManipulation(const sf::Vector2f& worldPos) {
    // Left mouse click - select sprite
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        Sprite* clickedSprite = getSpriteAtPosition(worldPos);
        
        if (clickedSprite) {
            m_selectedSprite = clickedSprite;
            m_isDragging = true;
            
            Transform spriteTransform = clickedSprite->getLocalTransform();
            m_dragOffset = sf::Vector2f(worldPos.x - spriteTransform.position.x, 
                                      worldPos.y - spriteTransform.position.y);
            
            if (m_onSpriteSelected) {
                m_onSpriteSelected(m_selectedSprite);
            }
        } else {
            m_selectedSprite = nullptr;
            m_isDragging = false;
            if (m_onSpriteSelected) {
                m_onSpriteSelected(nullptr);
            }
        }
    }
    
    // Left mouse drag - move selected sprite
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && m_isDragging && m_selectedSprite) {
        sf::Vector2f newPosition = sf::Vector2f(worldPos.x - m_dragOffset.x, 
                                               worldPos.y - m_dragOffset.y);
        
        Transform transform = m_selectedSprite->getLocalTransform();
        transform.position.x = newPosition.x;
        transform.position.y = newPosition.y;
        m_selectedSprite->setTransform(transform);
    }
    
    // Left mouse release - stop dragging
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_isDragging = false;
    }
}

sf::Vector2f ViewportPanel::screenToWorld(const sf::Vector2f& screenPos) const {
    if (!m_viewportInitialized) return sf::Vector2f(0, 0);
    
    sf::Vector2f normalized;
    normalized.x = screenPos.x / static_cast<float>(m_viewportSize.x);
    normalized.y = screenPos.y / static_cast<float>(m_viewportSize.y);
    
    sf::Vector2f viewSize = m_view.getSize();
    sf::Vector2f viewCenter = m_view.getCenter();
    
    sf::Vector2f worldPos;
    worldPos.x = viewCenter.x + (normalized.x - 0.5f) * viewSize.x;
    worldPos.y = viewCenter.y + (normalized.y - 0.5f) * viewSize.y;
    
    return worldPos;
}

Sprite* ViewportPanel::getSpriteAtPosition(const sf::Vector2f& worldPos) {
    if (!m_character) return nullptr;
    
    const auto& sprites = m_character->getSprites();
    
    // Iterate through sprites in reverse order (top-most first)
    for (auto it = sprites.rbegin(); it != sprites.rend(); ++it) {
        const auto& sprite = *it;
        if (!sprite->isVisible()) continue;
        
        // Get sprite's world transform
        Transform worldTransform = sprite->getWorldTransform();
        
        // Get texture to determine sprite bounds
        sf::Texture* texture = nullptr;
        if (m_spriteRenderer) {
            texture = m_spriteRenderer->getTexture(sprite->getTexturePath());
        }
        
        // Use texture size or default size if no texture
        float spriteWidth = 64.0f;  // Default size
        float spriteHeight = 64.0f;
        
        if (texture) {
            sf::Vector2u textureSize = texture->getSize();
            spriteWidth = static_cast<float>(textureSize.x);
            spriteHeight = static_cast<float>(textureSize.y);
        }
        
        // Apply scale to get actual sprite size
        float actualWidth = spriteWidth * worldTransform.scale.x;
        float actualHeight = spriteHeight * worldTransform.scale.y;
        
        // Calculate sprite bounds (sprites are centered on their position)
        float halfWidth = actualWidth * 0.5f;
        float halfHeight = actualHeight * 0.5f;
        
        float left = worldTransform.position.x - halfWidth;
        float right = worldTransform.position.x + halfWidth;
        float top = worldTransform.position.y - halfHeight;
        float bottom = worldTransform.position.y + halfHeight;
        
        // Simple bounding box check (ignoring rotation for now)
        if (worldPos.x >= left && worldPos.x <= right &&
            worldPos.y >= top && worldPos.y <= bottom) {
            return sprite.get();
        }
    }
    
    return nullptr;
}

std::shared_ptr<Bone> ViewportPanel::getBoneAtPosition(const sf::Vector2f& worldPos) {
    if (!m_boneTool) return nullptr;
    return m_boneTool->findBoneAtPosition(worldPos);
}

void ViewportPanel::setupTools() {
    // Sprite manipulation tool
    m_spriteTool = std::make_unique<SpriteTool>();
    if (m_spriteTool) {
        m_spriteTool->setOnSpriteSelected([this](Sprite* sprite) {
            m_selectedSprite = sprite;
            if (m_onSpriteSelected) {
                m_onSpriteSelected(sprite);
            }
        });
    }

    m_boneTool = std::make_unique<BoneTool>();
    if (m_boneTool) {
        // Set up IK tool integration
        auto ikTool = std::make_unique<IKSolverTool>();
        if (m_character) {
            ikTool->setCharacter(m_character);
        }
        
        // Set up callbacks
        ikTool->setOnEndEffectorSelected([this](std::shared_ptr<Bone> bone) {
            if (m_onBoneSelected) {
                m_onBoneSelected(bone);
            }
        });
        
        m_boneTool->setIKTool(ikTool.release()); // Transfer ownership
        
        // Set up bone tool callbacks
        m_boneTool->setOnBoneCreated([this](std::shared_ptr<Bone> bone) {
            if (m_onBoneCreated) {
                m_onBoneCreated(bone);
            }
        });
        
        m_boneTool->setOnBoneSelected([this](std::shared_ptr<Bone> bone) {
            if (m_onBoneSelected) {
                m_onBoneSelected(bone);
            }
        });
        
        m_boneTool->setOnBoneRotated([this](std::shared_ptr<Bone> bone, float rotation) {
            if (m_onBoneRotated) {
                m_onBoneRotated(bone, rotation);
            }
        });
    }
    
    setTool(ViewportTool::SpriteTool);
}

const char* ViewportPanel::getToolName() const {
    switch (m_currentTool) {
        case ViewportTool::SpriteTool: 
            return "Sprite Tool";
        case ViewportTool::BoneTool:
            if (m_boneTool) {
                return ("Bone Tool - " + std::string(m_boneTool->getSubToolName())).c_str();
            }
            return "Bone Tool";
        default: 
            return "Unknown";
    }
}

float ViewportPanel::getZoomLevel() const {
    // Get the current view size and compare to initial size
    sf::Vector2f currentSize = m_view.getSize();
    sf::Vector2f initialSize(static_cast<float>(m_viewportSize.x), static_cast<float>(m_viewportSize.y));
    
    // Zoom level is the ratio of initial size to current size
    return initialSize.x / currentSize.x;
}

} // namespace Riggle