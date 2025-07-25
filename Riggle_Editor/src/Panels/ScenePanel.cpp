#include "Editor/Panels/ScenePanel.h"
#include <imgui.h>
#include <imgui-SFML.h>
#include <cmath>
#include <iostream>

namespace Riggle {

ScenePanel::ScenePanel()
    : BasePanel("Scene")
    , m_character(std::make_unique<Character>("New Character"))
    , m_spriteRenderer(std::make_unique<SpriteRenderer>())
    , m_boneRenderer(std::make_unique<BoneRenderer>())
    , m_window(nullptr)
    , m_isDragging(false)
    , m_isPanning(false)
    , m_draggedSprite(nullptr)
    , m_selectedSprite(nullptr)
    , m_viewportInitialized(false)
    , m_viewportSize(800, 600)
    , m_showBones(true)
    , m_showSprites(true)
    , m_showGrid(true)
{
    // Initialize view
    m_view.setSize(sf::Vector2f(800.0f, 600.0f));
    m_view.setCenter(sf::Vector2f(400.0f, 300.0f));
    
    // Create empty rig for the character
    auto rig = std::make_unique<Rig>("Scene Rig");
    m_character->setRig(std::move(rig));
    
    // Initialize render texture
    initializeViewport(m_viewportSize);
}

void ScenePanel::render() {
    if (!m_isVisible) return;

    ImGui::Begin(m_name.c_str(), &m_isVisible);

    // Scene controls
    renderSceneControls();

    ImGui::Separator();

    // Get available content region
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    
    // Ensure minimum size
    float viewportWidth = std::max(contentRegion.x - 20, 400.0f);
    float viewportHeight = std::max(contentRegion.y - 100, 300.0f);
    
    // Resize render texture if needed
    sf::Vector2u newSize(static_cast<unsigned int>(viewportWidth), static_cast<unsigned int>(viewportHeight));
    if (newSize != m_viewportSize) {
        m_viewportSize = newSize;
        initializeViewport(m_viewportSize);
    }

    // Render the scene to texture
    if (m_viewportInitialized) {
        renderScene(m_renderTexture);
    }

    // Display the rendered texture in a child window to prevent dragging issues
    if (m_viewportInitialized) {
        // Create a child window that handles mouse input properly
        ImGui::BeginChild("SceneViewport", 
                         ImVec2(viewportWidth, viewportHeight), 
                         true, // Border
                         ImGuiWindowFlags_NoScrollbar | 
                         ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoMove);
        
        // Get the child window's position for mouse calculations
        ImVec2 childWindowPos = ImGui::GetCursorScreenPos();
        
        // Display the texture - this won't be draggable in the child window
        ImGui::Image(m_renderTexture, sf::Vector2f(viewportWidth, viewportHeight));
        
        // Handle mouse interaction within the child window
        if (ImGui::IsItemHovered()) {
            // Get mouse position relative to the child window
            ImVec2 mousePos = ImGui::GetMousePos();
            sf::Vector2f relativeMousePos = sf::Vector2f(
                mousePos.x - childWindowPos.x, 
                mousePos.y - childWindowPos.y
            );
            
            // Ensure mouse is within bounds
            if (relativeMousePos.x >= 0 && relativeMousePos.x <= viewportWidth &&
                relativeMousePos.y >= 0 && relativeMousePos.y <= viewportHeight) {
                
                // Convert to world coordinates
                sf::Vector2f worldPos = screenToWorld(relativeMousePos, sf::Vector2f(viewportWidth, viewportHeight));

                // Handle mouse input
                handleMouseInput(worldPos, relativeMousePos, sf::Vector2f(viewportWidth, viewportHeight));
            }
        }
        
        ImGui::EndChild();
    }

    // Display viewport info
    ImGui::Separator();
    ImGui::Text("Viewport: %.0fx%.0f", viewportWidth, viewportHeight);
    ImGui::Text("View Center: (%.1f, %.1f)", m_view.getCenter().x, m_view.getCenter().y);
    
    if (m_selectedSprite) {
        ImGui::Text("Selected: %s", m_selectedSprite->getName().c_str());
        Transform transform = m_selectedSprite->getLocalTransform();
        ImGui::Text("Position: (%.1f, %.1f)", transform.x, transform.y);
    } else {
        ImGui::Text("Selected: None");
    }

    // Instructions
    ImGui::Text("Controls:");
    ImGui::Text("- Left click: Select sprite");
    ImGui::Text("- Left drag: Move selected sprite");
    ImGui::Text("- Right drag: Pan view");

    ImGui::End();
}

void ScenePanel::update(sf::RenderWindow& window) {
    m_window = &window;
}

void ScenePanel::handleEvent(const sf::Event& event) {
    // Events are now handled in the render() function within ImGui context
}

void ScenePanel::handleMouseInput(const sf::Vector2f& worldPos, const sf::Vector2f& screenPos, const sf::Vector2f& viewportSize) {
    // Handle mouse clicks first (before checking drag state)
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        Sprite* clickedSprite = getSpriteAtPosition(worldPos);
        
        if (clickedSprite) {
            m_selectedSprite = clickedSprite;
            m_draggedSprite = clickedSprite;
            m_isDragging = true;
            
            // Calculate drag offset (difference between mouse position and sprite center)
            Transform spriteTransform = clickedSprite->getLocalTransform();
            m_dragOffset = sf::Vector2f(worldPos.x - spriteTransform.x, worldPos.y - spriteTransform.y);
            
            if (m_onSpriteSelected) {
                m_onSpriteSelected(m_selectedSprite);
            }
            
            std::cout << "Selected sprite: " << clickedSprite->getName() << " at (" << spriteTransform.x << ", " << spriteTransform.y << ")" << std::endl;
        } else {
            // Clicked on empty space
            m_selectedSprite = nullptr;
            m_isDragging = false;
            m_draggedSprite = nullptr;
            
            if (m_onSpriteSelected) {
                m_onSpriteSelected(nullptr);
            }
            
            std::cout << "Clicked on empty space" << std::endl;
        }
    }
    
    // Right mouse button for panning
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        m_isPanning = true;
        m_lastMousePos = worldPos;
        std::cout << "Started panning" << std::endl;
    }
    
    // Handle mouse release
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (m_isDragging) {
            std::cout << "Stopped dragging sprite" << std::endl;
        }
        m_isDragging = false;
        m_draggedSprite = nullptr;
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        if (m_isPanning) {
            std::cout << "Stopped panning" << std::endl;
        }
        m_isPanning = false;
    }
    
    // Handle continuous mouse movement
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && m_isDragging && m_draggedSprite) {
        // Calculate new position (subtract the drag offset to maintain relative position)
        sf::Vector2f newPosition = sf::Vector2f(worldPos.x - m_dragOffset.x, worldPos.y - m_dragOffset.y);
        
        // Update sprite transform
        Transform transform = m_draggedSprite->getLocalTransform();
        transform.x = newPosition.x;
        transform.y = newPosition.y;
        m_draggedSprite->setTransform(transform);
        
        std::cout << "Dragging sprite to: (" << newPosition.x << ", " << newPosition.y << ")" << std::endl;
    }
    
    // Handle panning
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && m_isPanning) {
        sf::Vector2f delta = m_lastMousePos - worldPos;
        m_view.move(delta);
        std::cout << "Panning view by: (" << delta.x << ", " << delta.y << ")" << std::endl;
    }
    
    // Always update last mouse position
    m_lastMousePos = worldPos;
}

void ScenePanel::renderScene(sf::RenderTarget& target) {
    // Clear the render texture
    if (auto* renderTexture = dynamic_cast<sf::RenderTexture*>(&target)) {
        renderTexture->clear(sf::Color(50, 50, 50, 255)); // Dark gray background
        renderTexture->setView(m_view);
    }

    // Render grid
    if (m_showGrid) {
        renderGrid(target);
    }

    // Update character deformations
    if (m_character) {
        m_character->updateDeformations();
    }

    // Render sprites using SpriteRenderer
    if (m_showSprites && m_character) {
        m_spriteRenderer->render(target, *m_character);
    }

    // Render bones using BoneRenderer
    if (m_showBones && m_character && m_character->getRig()) {
        m_boneRenderer->render(target, *m_character->getRig());
    }

    // Highlight selected sprite
    if (m_selectedSprite) {
        // Draw selection outline
        const auto& vertices = m_selectedSprite->getDeformedVertices();
        if (vertices.size() >= 4) {
            sf::RectangleShape outline;
            
            // Calculate bounding box
            float minX = vertices[0].x;
            float maxX = vertices[0].x;
            float minY = vertices[0].y;
            float maxY = vertices[0].y;

            for (const auto& vertex : vertices) {
                minX = std::min(minX, vertex.x);
                maxX = std::max(maxX, vertex.x);
                minY = std::min(minY, vertex.y);
                maxY = std::max(maxY, vertex.y);
            }

            outline.setPosition(sf::Vector2f(minX, minY));
            outline.setSize(sf::Vector2f(maxX - minX, maxY - minY));
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::Yellow);
            outline.setOutlineThickness(2.0f);
            
            target.draw(outline);
        }
    }

    // Display the render texture
    if (auto* renderTexture = dynamic_cast<sf::RenderTexture*>(&target)) {
        renderTexture->display();
    }
}

void ScenePanel::setCharacter(std::unique_ptr<Character> character) {
    m_character = std::move(character);
    m_selectedSprite = nullptr;
    m_draggedSprite = nullptr;
}

void ScenePanel::addSpriteFromAsset(const AssetInfo& asset, const sf::Vector2f& position) {
    if (!m_character) return;

    auto sprite = std::make_unique<Sprite>(asset.name, asset.path);
    
    // Setup as quad with reasonable default size (we'll scale based on texture later)
    sprite->setupAsQuad(200.0f, 200.0f); // Default size
    
    // Set position
    Transform transform;
    transform.x = position.x;
    transform.y = position.y;
    sprite->setTransform(transform);

    // Pre-load the texture in the renderer
    m_spriteRenderer->loadTexture(asset.path);

    m_character->addSprite(std::move(sprite));
}

void ScenePanel::resetView() {
    m_view.setCenter(sf::Vector2f(400.0f, 300.0f));
    m_view.setSize(sf::Vector2f(800.0f, 600.0f));
}

sf::Vector2f ScenePanel::screenToWorld(const sf::Vector2f& screenPos, const sf::Vector2f& viewportSize) const {
    // Convert screen position to normalized coordinates (0 to 1)
    sf::Vector2f normalized(
        screenPos.x / viewportSize.x,
        screenPos.y / viewportSize.y
    );
    
    // Get view properties
    sf::Vector2f viewCenter = m_view.getCenter();
    sf::Vector2f viewSize = m_view.getSize();
    
    // Convert to world coordinates
    sf::Vector2f worldPos(
        viewCenter.x + (normalized.x - 0.5f) * viewSize.x,
        viewCenter.y + (normalized.y - 0.5f) * viewSize.y
    );
    
    return worldPos;
}

Sprite* ScenePanel::getSpriteAtPosition(const sf::Vector2f& worldPos) {
    if (!m_character) return nullptr;

    // Test sprites in reverse order (last added = top layer)
    const auto& sprites = m_character->getSprites();
    for (auto it = sprites.rbegin(); it != sprites.rend(); ++it) {
        const auto& sprite = *it;
        const auto& vertices = sprite->getDeformedVertices();
        
        if (vertices.size() >= 4) {
            // Use point-in-polygon test for better accuracy
            if (isPointInQuad(worldPos, vertices)) {
                return sprite.get();
            }
        }
    }

    return nullptr;
}

bool ScenePanel::isPointInQuad(const sf::Vector2f& point, const std::vector<Vertex>& vertices) const {
    if (vertices.size() < 4) return false;
    
    // Simple bounding box test first (faster)
    float minX = vertices[0].x, maxX = vertices[0].x;
    float minY = vertices[0].y, maxY = vertices[0].y;
    
    for (const auto& vertex : vertices) {
        minX = std::min(minX, vertex.x);
        maxX = std::max(maxX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }
    
    // If point is outside bounding box, it's definitely outside the quad
    if (point.x < minX || point.x > maxX || point.y < minY || point.y > maxY) {
        return false;
    }
    
    // More accurate point-in-polygon test using ray casting
    bool inside = false;
    for (size_t i = 0, j = vertices.size() - 1; i < vertices.size(); j = i++) {
        if (((vertices[i].y > point.y) != (vertices[j].y > point.y)) &&
            (point.x < (vertices[j].x - vertices[i].x) * (point.y - vertices[i].y) / (vertices[j].y - vertices[i].y) + vertices[i].x)) {
            inside = !inside;
        }
    }
    
    return inside;
}

void ScenePanel::updateSpriteDragging() {
    // This is now handled in handleMouseInput() method
}

void ScenePanel::renderGrid(sf::RenderTarget& target) {
    // Draw a simple grid
    sf::Vector2f viewSize = m_view.getSize();
    sf::Vector2f viewCenter = m_view.getCenter();
    
    float gridSize = 50.0f;
    sf::Color gridColor(100, 100, 100, 100); // Semi-transparent gray
    
    // Vertical lines
    float startX = viewCenter.x - viewSize.x * 0.5f;
    float endX = viewCenter.x + viewSize.x * 0.5f;
    float startY = viewCenter.y - viewSize.y * 0.5f;
    float endY = viewCenter.y + viewSize.y * 0.5f;
    
    // Round to grid
    startX = std::floor(startX / gridSize) * gridSize;
    startY = std::floor(startY / gridSize) * gridSize;
    
    for (float x = startX; x <= endX; x += gridSize) {
        sf::Vertex line[2];
        line[0].position = sf::Vector2f(x, startY);
        line[0].color = gridColor;
        line[1].position = sf::Vector2f(x, endY);
        line[1].color = gridColor;
        target.draw(line, 2, sf::PrimitiveType::Lines);
    }
    
    for (float y = startY; y <= endY; y += gridSize) {
        sf::Vertex line[2];
        line[0].position = sf::Vector2f(startX, y);
        line[0].color = gridColor;
        line[1].position = sf::Vector2f(endX, y);
        line[1].color = gridColor;
        target.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void ScenePanel::renderSceneControls() {
    ImGui::Text("Display Options:");
    ImGui::Checkbox("Show Sprites", &m_showSprites);
    ImGui::SameLine();
    ImGui::Checkbox("Show Bones", &m_showBones);
    ImGui::SameLine();
    ImGui::Checkbox("Show Grid", &m_showGrid);

    if (ImGui::Button("Reset View")) {
        resetView();
    }
    ImGui::SameLine();
    if (ImGui::Button("Center Character")) {
        // Center view on character - implement later
    }

    if (m_character) {
        ImGui::Text("Sprites: %zu", m_character->getSprites().size());
    }
}

void ScenePanel::initializeViewport(const sf::Vector2u& size) {
    // Use resize() for SFML 3.0
    m_renderTexture.resize(size);
    m_viewportInitialized = true;
    m_renderTexture.setSmooth(true);
}

} // namespace Riggle