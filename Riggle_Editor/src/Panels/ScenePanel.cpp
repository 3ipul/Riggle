#include "Editor/Panels/ScenePanel.h"
#include "Riggle/IK_Solver.h"
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
    , m_boneCreationTool(std::make_unique<BoneCreationTool>())
    , m_window(nullptr)
    , m_toolMode(SceneToolMode::SpriteManipulation)
    , m_isDragging(false)
    , m_isPanning(false)
    , m_draggedSprite(nullptr)
    , m_selectedSprite(nullptr)
    , m_selectedBone(nullptr)
    , m_showBindingUI(false)
    , m_spriteToBindFrom(nullptr)
    , m_boneToBindFrom(nullptr)
    , m_viewportInitialized(false)
    , m_viewportSize(800, 600)
    , m_showBones(true)
    , m_showSprites(true)
    , m_showGrid(true)
    , m_showBindings(true)
{
    // Initialize view
    m_view.setSize(sf::Vector2f(800.0f, 600.0f));
    m_view.setCenter(sf::Vector2f(400.0f, 300.0f));
    
    // Create empty rig for the character
    auto rig = std::make_unique<Rig>("Scene Rig");
    m_character->setRig(std::move(rig));
    
    // Initialize bone creation tool
    m_boneCreationTool->setRig(m_character->getRig());
    
    // Setup bone tool callbacks
    m_boneCreationTool->setOnBoneCreated([this](std::shared_ptr<Bone> bone) {
        std::cout << "Bone created: " << bone->getName() << std::endl;
    });
    
    m_boneCreationTool->setOnBoneSelected([this](std::shared_ptr<Bone> bone) {
        m_selectedBone = bone;
        if (m_onBoneSelected) {
            m_onBoneSelected(bone);
        }
    });
    
    // NEW: Setup bone rotation callback
    m_boneCreationTool->setOnBoneRotated([this](std::shared_ptr<Bone> bone, float rotation) {
    // CRITICAL: Force character deformation update during rotation
    if (m_character) {
        m_character->forceUpdateDeformations();
    }
    
    if (m_onBoneRotated) {
        m_onBoneRotated(bone, rotation);
    }
    });
    
    // Initialize render texture
    initializeViewport(m_viewportSize);
}

void ScenePanel::setToolMode(SceneToolMode mode) {
    m_toolMode = mode;
    
    // Update bone tool mode based on scene tool mode
    switch (mode) {
        case SceneToolMode::BoneCreation:
            m_boneCreationTool->setMode(BoneToolMode::Creating);
            break;
        case SceneToolMode::BoneSelection:
            m_boneCreationTool->setMode(BoneToolMode::Selecting);
            break;
        case SceneToolMode::BoneRotation:
            m_boneCreationTool->setMode(BoneToolMode::Rotating);
            break;
        case SceneToolMode::SpriteBinding:
            m_boneCreationTool->setActive(false);
            break;
        case SceneToolMode::IKSolver:
            m_boneCreationTool->setActive(false);
            m_ikMode = true;
            m_ikTarget = nullptr;
            m_ikDragging = false;
            break;
        default:
            m_boneCreationTool->setActive(false);
            break;
    }
    
    // Clear current operations when switching modes
    m_boneCreationTool->cancelCurrentOperation();
    m_showBindingUI = false;
    m_spriteToBindFrom = nullptr;
    m_boneToBindFrom = nullptr;
}

void ScenePanel::handleEvent(const sf::Event& event) {
    // Handle keyboard shortcuts for tool switching
    if (event.is<sf::Event::KeyPressed>()) {
        const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
        if (keyPressed) {
            switch (keyPressed->code) {
                case sf::Keyboard::Key::Num1:
                    setToolMode(SceneToolMode::SpriteManipulation);
                    std::cout << "Switched to Sprite Mode" << std::endl;
                    break;
                case sf::Keyboard::Key::Num2:
                    setToolMode(SceneToolMode::BoneCreation);
                    std::cout << "Switched to Bone Creation Mode" << std::endl;
                    break;
                case sf::Keyboard::Key::Num3:
                    setToolMode(SceneToolMode::BoneSelection);
                    std::cout << "Switched to Bone Selection Mode" << std::endl;
                    break;
                case sf::Keyboard::Key::Num4:
                    setToolMode(SceneToolMode::BoneRotation);
                    std::cout << "Switched to Bone Rotation Mode (FK)" << std::endl;
                    break;
                case sf::Keyboard::Key::Num5:
                    setToolMode(SceneToolMode::SpriteBinding);
                    std::cout << "Switched to Sprite Binding Mode" << std::endl;
                    break;
                case sf::Keyboard::Key::Num6:
                    setToolMode(SceneToolMode::IKSolver);
                    std::cout << "Switched to IK Solver Mode" << std::endl;
                    break;
                case sf::Keyboard::Key::Escape:
                    // FIXED: Only cancel current operations, don't exit application
                    handleEscapeKey();
                    break;
                default:
                    break; // Don't handle other keys
            }
        }
    }
}

void ScenePanel::handleEscapeKey() {
    bool operationCancelled = false;
    
    if (m_toolMode == SceneToolMode::BoneCreation || m_toolMode == SceneToolMode::BoneRotation) {
        if (m_boneCreationTool->isCreating() || m_boneCreationTool->isRotating()) {
            m_boneCreationTool->cancelCurrentOperation();
            std::cout << "Cancelled current bone operation" << std::endl;
            operationCancelled = true;
        }
    } else if (m_toolMode == SceneToolMode::SpriteBinding) {
        if (m_showBindingUI || m_spriteToBindFrom || m_boneToBindFrom) {
            m_showBindingUI = false;
            m_spriteToBindFrom = nullptr;
            m_boneToBindFrom = nullptr;
            std::cout << "Cancelled binding operation" << std::endl;
            operationCancelled = true;
        }
    } else if (m_toolMode == SceneToolMode::IKSolver) {  // ADD THIS
        if (m_ikTarget || m_ikDragging) {
            m_ikTarget = nullptr;
            m_ikDragging = false;
            std::cout << "Cancelled IK operation" << std::endl;
            operationCancelled = true;
        }
    }
    
    // If no operation was cancelled, clear selections
    if (!operationCancelled) {
        m_selectedSprite = nullptr;
        m_selectedBone = nullptr;
        m_draggedSprite = nullptr;
        m_isDragging = false;
        
        if (m_onSpriteSelected) m_onSpriteSelected(nullptr);
        if (m_onBoneSelected) m_onBoneSelected(nullptr);
        
        std::cout << "Cleared all selections" << std::endl;
    }
}

void ScenePanel::handleBoneInteraction(const sf::Vector2f& worldPos) {
    // Update bone creation tool settings
    m_boneCreationTool->setActive(true);
    
    // Handle mouse events through the bone creation tool
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_boneCreationTool->handleMousePress(worldPos);
        m_selectedSprite = nullptr; // Clear sprite selection
    }
    
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        m_boneCreationTool->handleMouseMove(worldPos);
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_boneCreationTool->handleMouseRelease(worldPos);
        
        // CRITICAL: Update deformations after bone manipulation
        if (m_character) {
            m_character->forceUpdateDeformations();
        }
    }
    
    // Update selected bone from tool
    auto toolSelectedBone = m_boneCreationTool->getSelectedBone();
    if (toolSelectedBone != m_selectedBone) {
        m_selectedBone = toolSelectedBone;
        if (m_onBoneSelected) {
            m_onBoneSelected(m_selectedBone);
        }
    }
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
        m_character->forceUpdateDeformations();
    }

    // Render sprites using SpriteRenderer
    if (m_showSprites && m_character) {
        m_spriteRenderer->render(target, *m_character);
    }

    // Render bones using BoneRenderer
    if (m_showBones && m_character && m_character->getRig()) {
        m_boneRenderer->render(target, *m_character->getRig());
    }

    // NEW: Render binding connections
    if (m_showBindings) {
        renderBindingConnections(target);
    }

    // Render tool-specific overlays
    switch (m_toolMode) {
        case SceneToolMode::BoneCreation:
            renderBoneCreationPreview(target);
            break;
        case SceneToolMode::BoneRotation:
            renderBoneRotationPreview(target);
            renderBoneHandles(target);
            break;
        case SceneToolMode::SpriteBinding:
            // Highlight bindable elements differently
            break;
        case SceneToolMode::IKSolver:
            renderIKTargets(target);
            break;
    }

    // Highlight selected sprite
    if (m_selectedSprite) {
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
            
            // Different colors for different modes
            if (m_toolMode == SceneToolMode::SpriteBinding) {
                outline.setOutlineColor(sf::Color::Magenta);  // Binding mode
            } else {
                outline.setOutlineColor(sf::Color::Yellow);   // Normal selection
            }
            outline.setOutlineThickness(2.0f);
            
            target.draw(outline);
        }
    }

    // Highlight selected bone
    if (m_selectedBone) {
        float startX, startY, endX, endY;
        m_selectedBone->getWorldEndpoints(startX, startY, endX, endY);
        
        // Different colors for different modes
        sf::Color boneColor = (m_toolMode == SceneToolMode::SpriteBinding) ? 
                             sf::Color::Magenta : sf::Color::Cyan;
        
        // Draw thick highlight line
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                sf::Vertex thickLine[2];
                thickLine[0].position = sf::Vector2f(startX + i, startY + j);
                thickLine[0].color = boneColor;
                thickLine[1].position = sf::Vector2f(endX + i, endY + j);
                thickLine[1].color = boneColor;
                target.draw(thickLine, 2, sf::PrimitiveType::Lines);
            }
        }
    }

    // Display the render texture
    if (auto* renderTexture = dynamic_cast<sf::RenderTexture*>(&target)) {
        renderTexture->display();
    }
}

void ScenePanel::renderBoneRotationPreview(sf::RenderTarget& target) {
    // Show rotation arc/indicator when rotating
    if (m_boneCreationTool && m_boneCreationTool->isRotating()) {
        auto rotatingBone = m_boneCreationTool->getRotatingBone();
        if (rotatingBone) {
            float startX, startY, endX, endY;
            rotatingBone->getWorldEndpoints(startX, startY, endX, endY);
            
            // Draw rotation arc (simplified - just show the rotation line in different color)
            sf::Vertex rotationLine[2];
            rotationLine[0].position = sf::Vector2f(startX, startY);
            rotationLine[0].color = sf::Color::Red;
            rotationLine[1].position = sf::Vector2f(endX, endY);
            rotationLine[1].color = sf::Color::Red;
            
            // Draw multiple times for thickness
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    sf::Vertex thickLine[2];
                    thickLine[0].position = sf::Vector2f(startX + i, startY + j);
                    thickLine[0].color = sf::Color::Red;
                    thickLine[1].position = sf::Vector2f(endX + i, endY + j);
                    thickLine[1].color = sf::Color::Red;
                    target.draw(thickLine, 2, sf::PrimitiveType::Lines);
                }
            }
        }
    }
}

void ScenePanel::renderBoneHandles(sf::RenderTarget& target) {
    // Show rotation handles when in rotation mode
    if (m_toolMode == SceneToolMode::BoneRotation && m_selectedBone) {
        float startX, startY, endX, endY;
        m_selectedBone->getWorldEndpoints(startX, startY, endX, endY);
        
        // Draw start point (joint)
        sf::CircleShape startHandle(8.0f);
        startHandle.setPosition(sf::Vector2f(startX - 8.0f, startY - 8.0f));
        startHandle.setFillColor(sf::Color::Blue);
        startHandle.setOutlineColor(sf::Color::White);
        startHandle.setOutlineThickness(2.0f);
        target.draw(startHandle);
        
        // Draw end point (rotation handle)
        sf::CircleShape endHandle(10.0f);
        endHandle.setPosition(sf::Vector2f(endX - 10.0f, endY - 10.0f));
        endHandle.setFillColor(sf::Color::Green);
        endHandle.setOutlineColor(sf::Color::White);
        endHandle.setOutlineThickness(2.0f);
        target.draw(endHandle);
        
        // Draw small text indicating this is a rotation handle
        // (For now, just use colored circles - text rendering would need font setup)
    }
}

void ScenePanel::renderBindingConnections(sf::RenderTarget& target) {
    if (!m_character) return;
    
    // Draw lines from sprites to their bound bones
    const auto& sprites = m_character->getSprites();
    for (const auto& sprite : sprites) {
        // FIXED: Remove legacy checks, only use new binding system
        if (!sprite->isBoundToBones()) continue;
        
        // Get sprite center position
        const auto& vertices = sprite->getDeformedVertices();
        if (vertices.empty()) continue;
        
        float spriteX = 0.0f, spriteY = 0.0f;
        for (const auto& vertex : vertices) {
            spriteX += vertex.x;
            spriteY += vertex.y;
        }
        spriteX /= vertices.size();
        spriteY /= vertices.size();
        
        // Draw connection lines to bound bones
        const auto& bindings = sprite->getBoneBindings();
        for (const auto& binding : bindings) {
            if (!binding.bone) continue;
            
            float boneStartX, boneStartY, boneEndX, boneEndY;
            binding.bone->getWorldEndpoints(boneStartX, boneStartY, boneEndX, boneEndY);
            
            // Line from sprite center to bone start
            sf::Vertex connectionLine[2];
            connectionLine[0].position = sf::Vector2f(spriteX, spriteY);
            connectionLine[0].color = sf::Color(0, 255, 255, 100); // Cyan with transparency
            connectionLine[1].position = sf::Vector2f(boneStartX, boneStartY);
            connectionLine[1].color = sf::Color(0, 255, 255, 100);
            
            target.draw(connectionLine, 2, sf::PrimitiveType::Lines);
        }
    }
}

void ScenePanel::renderSceneControls() {
    ImGui::Text("Tool Mode:");
    ImGui::SameLine();
    
    // Tool mode buttons
    if (ImGui::Button("Sprites (1)")) {
        setToolMode(SceneToolMode::SpriteManipulation);
    }
    ImGui::SameLine();
    if (ImGui::Button("Create Bones (2)")) {
        setToolMode(SceneToolMode::BoneCreation);
    }
    ImGui::SameLine();
    if (ImGui::Button("Select Bones (3)")) {
        setToolMode(SceneToolMode::BoneSelection);
    }
    ImGui::SameLine();
    if (ImGui::Button("Rotate Bones (4)")) {
        setToolMode(SceneToolMode::BoneRotation);
    }
    ImGui::SameLine();
    if (ImGui::Button("Bind Sprites (5)")) {
        setToolMode(SceneToolMode::SpriteBinding);
    }
    ImGui::SameLine();
    if (ImGui::Button("IK Solver (6)")) {
        setToolMode(SceneToolMode::IKSolver);
    }

    ImGui::Separator();
    
    ImGui::Text("Display Options:");
    ImGui::Checkbox("Show Sprites", &m_showSprites);
    ImGui::SameLine();
    ImGui::Checkbox("Show Bones", &m_showBones);
    ImGui::SameLine();
    ImGui::Checkbox("Show Grid", &m_showGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Show Bindings", &m_showBindings);

    // NEW: View controls
    ImGui::Separator();
    ImGui::Text("View Controls:");
    if (ImGui::Button("Reset View")) {
        resetView();
    }
    ImGui::SameLine();
    if (ImGui::Button("Fit to Content")) {
        fitViewToContent();
    }
    ImGui::SameLine();
    if (ImGui::Button("Zoom In")) {
        sf::Vector2f currentSize = m_view.getSize();
        m_view.setSize(currentSize * 0.8f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Zoom Out")) {
        sf::Vector2f currentSize = m_view.getSize();
        m_view.setSize(currentSize * 1.25f);
    }

    if (m_character && m_character->getRig()) {
        ImGui::Text("Sprites: %zu | Bones: %zu", 
                   m_character->getSprites().size(),
                   m_character->getRig()->getAllBones().size());
    }
}

void ScenePanel::fitViewToContent() {
    if (!m_character) return;
    
    const auto& sprites = m_character->getSprites();
    if (sprites.empty()) return;
    
    // Calculate bounding box of all sprites
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    for (const auto& sprite : sprites) {
        const auto& vertices = sprite->getDeformedVertices();
        for (const auto& vertex : vertices) {
            minX = std::min(minX, vertex.x);
            maxX = std::max(maxX, vertex.x);
            minY = std::min(minY, vertex.y);
            maxY = std::max(maxY, vertex.y);
        }
    }
    
    // Add padding
    float padding = 100.0f;
    minX -= padding;
    maxX += padding;
    minY -= padding;
    maxY += padding;
    
    // Set view to fit content
    sf::Vector2f center((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);
    sf::Vector2f size(maxX - minX, maxY - minY);
    
    m_view.setCenter(center);
    m_view.setSize(size);
}

void ScenePanel::renderBindingUI() {
    if (!m_showBindingUI) return;
    
    ImGui::Begin("Sprite-Bone Binding", &m_showBindingUI, ImGuiWindowFlags_AlwaysAutoResize);
    
    // Sprite selection dropdown
    ImGui::Text("Select Sprite to Bind:");
    if (m_character) {
        const auto& sprites = m_character->getSprites();
        
        const char* currentSpriteText = m_selectedSprite ? m_selectedSprite->getName().c_str() : "None";
        if (ImGui::BeginCombo("##SpriteSelect", currentSpriteText)) {
            
            if (ImGui::Selectable("None", m_selectedSprite == nullptr)) {
                m_selectedSprite = nullptr;
                if (m_onSpriteSelected) m_onSpriteSelected(nullptr);
            }
            
            for (size_t i = 0; i < sprites.size(); ++i) {
                const auto& sprite = sprites[i];
                bool isSelected = (sprite.get() == m_selectedSprite);
                
                std::string displayName = sprite->getName();
                if (displayName.empty()) {
                    displayName = "Sprite " + std::to_string(i);
                }
                
                // FIXED: Add binding info (NO LEGACY)
                if (sprite->isBoundToBones()) {
                    const auto& bindings = sprite->getBoneBindings();
                    displayName += " [" + std::to_string(bindings.size()) + " bones]";
                }
                
                if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                    m_selectedSprite = sprite.get();
                    if (m_onSpriteSelected) {
                        m_onSpriteSelected(m_selectedSprite);
                    }
                }
            }
            ImGui::EndCombo();
        }
        
        // Bone selection dropdown
        ImGui::Separator();
        ImGui::Text("Select Bone to Bind To:");
        if (m_character->getRig()) {
            const auto& allBones = m_character->getRig()->getAllBones();
            
            const char* currentBoneText = m_selectedBone ? m_selectedBone->getName().c_str() : "None";
            if (ImGui::BeginCombo("##BoneSelect", currentBoneText)) {
                
                if (ImGui::Selectable("None", m_selectedBone == nullptr)) {
                    m_selectedBone = nullptr;
                    if (m_onBoneSelected) m_onBoneSelected(nullptr);
                }
                
                for (const auto& bone : allBones) {
                    bool isSelected = (bone == m_selectedBone);
                    
                    if (ImGui::Selectable(bone->getName().c_str(), isSelected)) {
                        m_selectedBone = bone;
                        if (m_onBoneSelected) {
                            m_onBoneSelected(bone);
                        }
                    }
                }
                ImGui::EndCombo();
            }
        }
    }
    
    ImGui::Separator();
    
    if (m_selectedSprite) {
        ImGui::Text("Selected Sprite: %s", m_selectedSprite->getName().c_str());
        
        // FIXED: Show current bindings (NO LEGACY)
        if (m_selectedSprite->isBoundToBones()) {
            ImGui::Text("Current Bindings:");
            const auto& bindings = m_selectedSprite->getBoneBindings();
            for (size_t i = 0; i < bindings.size(); ++i) {
                const auto& binding = bindings[i];
                ImGui::Text("- %s (weight: %.2f)", 
                           binding.bone->getName().c_str(), 
                           binding.weight);
                
                ImGui::SameLine();
                std::string unbindLabel = "Unbind##" + std::to_string(i);
                if (ImGui::Button(unbindLabel.c_str())) {
                    m_selectedSprite->unbindFromBone(binding.bone);
                    if (m_onSpriteBindingChanged) {
                        m_onSpriteBindingChanged(m_selectedSprite, nullptr);
                    }
                }
            }
        } else {
            ImGui::Text("No bindings");
        }
        
        ImGui::Separator();
        
        // Bind to selected bone
        if (m_selectedBone) {
            ImGui::Text("Bind '%s' to '%s'", 
                       m_selectedSprite->getName().c_str(),
                       m_selectedBone->getName().c_str());
            
            static float bindWeight = 1.0f;
            ImGui::SliderFloat("Weight", &bindWeight, 0.0f, 1.0f);
            
            if (ImGui::Button("Bind Sprite to Bone")) {
                m_selectedSprite->bindToBone(m_selectedBone, bindWeight);
                if (m_onSpriteBindingChanged) {
                    m_onSpriteBindingChanged(m_selectedSprite, m_selectedBone);
                }
            }
        } else {
            ImGui::Text("Select a bone to bind to");
        }
    } else {
        ImGui::Text("Select a sprite to bind");
    }
    
    if (ImGui::Button("Close")) {
        m_showBindingUI = false;
    }
    
    ImGui::End();
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

                // Handle mouse input based on current tool mode
                handleMouseInput(worldPos, relativeMousePos, sf::Vector2f(viewportWidth, viewportHeight));
            }
        }
        
        ImGui::EndChild();
    }

    // Display viewport info
    ImGui::Separator();
    ImGui::Text("Viewport: %.0fx%.0f", viewportWidth, viewportHeight);
    ImGui::Text("View Center: (%.1f, %.1f)", m_view.getCenter().x, m_view.getCenter().y);
    
    // Display current tool mode
    const char* toolModeNames[] = { 
        "Sprite Mode", 
        "Bone Creation", 
        "Bone Selection", 
        "Bone Rotation (FK)",
        "Sprite Binding"
        "IK Solver"
    };
    ImGui::Text("Tool: %s", toolModeNames[(int)m_toolMode]);
    
    if (m_selectedSprite) {
        ImGui::Text("Selected Sprite: %s", m_selectedSprite->getName().c_str());
        Transform transform = m_selectedSprite->getLocalTransform();
        ImGui::Text("Position: (%.1f, %.1f)", transform.x, transform.y);
    } else if (m_selectedBone) {
        ImGui::Text("Selected Bone: %s", m_selectedBone->getName().c_str());
        Transform transform = m_selectedBone->getLocalTransform();
        ImGui::Text("Position: (%.1f, %.1f)", transform.x, transform.y);
        ImGui::Text("Rotation: %.1f°", transform.rotation * 180.0f / 3.14159f);
    } else {
        ImGui::Text("Selected: None");
    }

    // Instructions based on tool mode
    ImGui::Text("Controls:");
    switch (m_toolMode) {
        case SceneToolMode::SpriteManipulation:
            ImGui::Text("- Left click: Select sprite");
            ImGui::Text("- Left drag: Move selected sprite");
            break;
        case SceneToolMode::BoneCreation:
            ImGui::Text("- Left click & drag: Create bone");
            ImGui::Text("- ESC: Cancel creation");
            break;
        case SceneToolMode::BoneSelection:
            ImGui::Text("- Left click: Select bone");
            ImGui::Text("- Left drag: Move selected bone");
            break;
        case SceneToolMode::BoneRotation:
            ImGui::Text("- Left click: Select bone");
            ImGui::Text("- Drag green handle: Rotate bone (FK)");
            ImGui::Text("- ESC: Cancel rotation");
            break;
        case SceneToolMode::SpriteBinding:
            ImGui::Text("- Left click sprite: Select for binding");
            ImGui::Text("- Left click bone: Select target bone");
            ImGui::Text("- Use binding panel to connect");
            ImGui::Text("- ESC: Cancel binding");
            break;
        case SceneToolMode::IKSolver:
            ImGui::Text("- Left click bone end: Select IK target");
            ImGui::Text("- Left drag: Move IK target");
            ImGui::Text("- IK solves in real-time");
            ImGui::Text("- ESC: Clear IK target");
            break;
    }
    ImGui::Text("- Right drag: Pan view");

    ImGui::End();
    
    // Render binding UI as separate window
    renderBindingUI();

    if (m_toolMode == SceneToolMode::IKSolver) {
        renderIKUI();
    }
}

void ScenePanel::update(sf::RenderWindow& window) {
    m_window = &window;
}

void ScenePanel::setCharacter(std::unique_ptr<Character> character) {
    m_character = std::move(character);
    m_selectedSprite = nullptr;
    m_selectedBone = nullptr;
    m_draggedSprite = nullptr;
    
    // Update bone creation tool rig reference
    if (m_character && m_character->getRig()) {
        m_boneCreationTool->setRig(m_character->getRig());
    }
}

void ScenePanel::addSpriteFromAsset(const AssetInfo& asset, const sf::Vector2f& position) {
    if (!m_character) return;

    auto sprite = std::make_unique<Sprite>(asset.name, asset.path);
    
    // NEW: Load texture to get actual dimensions
    sf::Texture tempTexture;
    float spriteWidth = 200.0f;  // Default fallback
    float spriteHeight = 200.0f;
    
    if (tempTexture.loadFromFile(asset.path)) {
        sf::Vector2u textureSize = tempTexture.getSize();
        spriteWidth = static_cast<float>(textureSize.x);
        spriteHeight = static_cast<float>(textureSize.y);
        
        // Scale down if too large (optional)
        float maxSize = 300.0f;
        if (spriteWidth > maxSize || spriteHeight > maxSize) {
            float scale = maxSize / std::max(spriteWidth, spriteHeight);
            spriteWidth *= scale;
            spriteHeight *= scale;
        }
    }
    
    // Setup quad with actual aspect ratio
    sprite->setupAsQuad(spriteWidth, spriteHeight);
    
    Transform transform;
    transform.x = position.x;
    transform.y = position.y;
    sprite->setTransform(transform);

    m_spriteRenderer->loadTexture(asset.path);
    m_character->addSprite(std::move(sprite));
    
    std::cout << "Added sprite with dimensions: " << spriteWidth << "x" << spriteHeight << std::endl;
}

void ScenePanel::resetView() {
    m_view.setCenter(sf::Vector2f(400.0f, 300.0f));
    m_view.setSize(sf::Vector2f(800.0f, 600.0f));
}

sf::Vector2f ScenePanel::screenToWorld(const sf::Vector2f& screenPos, const sf::Vector2f& viewportSize) const {
    sf::Vector2f normalized(
        screenPos.x / viewportSize.x,
        screenPos.y / viewportSize.y
    );
    
    sf::Vector2f viewCenter = m_view.getCenter();
    sf::Vector2f viewSize = m_view.getSize();
    
    sf::Vector2f worldPos(
        viewCenter.x + (normalized.x - 0.5f) * viewSize.x,
        viewCenter.y + (normalized.y - 0.5f) * viewSize.y
    );
    
    return worldPos;
}

void ScenePanel::handleMouseInput(const sf::Vector2f& worldPos, const sf::Vector2f& screenPos, const sf::Vector2f& viewportSize) {
    // Handle panning first (works in all modes)
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        m_isPanning = true;
        m_lastMousePos = worldPos;
        std::cout << "Started panning" << std::endl;
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        if (m_isPanning) {
            std::cout << "Stopped panning" << std::endl;
        }
        m_isPanning = false;
    }
    
    // Handle panning movement
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && m_isPanning) {
        sf::Vector2f delta = m_lastMousePos - worldPos;
        m_view.move(delta);
    }
    
    // Handle tool-specific interactions
    switch (m_toolMode) {
        case SceneToolMode::SpriteManipulation:
            handleSpriteInteraction(worldPos);
            break;
            
        case SceneToolMode::BoneCreation:
        case SceneToolMode::BoneSelection:
        case SceneToolMode::BoneRotation:
            handleBoneInteraction(worldPos);
            break;
            
        case SceneToolMode::SpriteBinding:
            handleBindingInteraction(worldPos);
            break;
        
        case SceneToolMode::IKSolver:
            handleIKInteraction(worldPos);
            break;
    }
    
    // Always update last mouse position
    m_lastMousePos = worldPos;
}

void ScenePanel::handleSpriteInteraction(const sf::Vector2f& worldPos) {
    // Handle sprite selection and dragging
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        Sprite* clickedSprite = getSpriteAtPosition(worldPos);
        
        if (clickedSprite) {
            m_selectedSprite = clickedSprite;
            m_selectedBone = nullptr; // Clear bone selection
            m_draggedSprite = clickedSprite;
            m_isDragging = true;
            
            // Calculate drag offset
            Transform spriteTransform = clickedSprite->getLocalTransform();
            m_dragOffset = sf::Vector2f(worldPos.x - spriteTransform.x, worldPos.y - spriteTransform.y);
            
            if (m_onSpriteSelected) {
                m_onSpriteSelected(m_selectedSprite);
            }
            
            std::cout << "Selected sprite: " << clickedSprite->getName() << std::endl;
        } else {
            // Clicked on empty space
            m_selectedSprite = nullptr;
            m_isDragging = false;
            m_draggedSprite = nullptr;
            
            if (m_onSpriteSelected) {
                m_onSpriteSelected(nullptr);
            }
        }
    }
    
    // Handle mouse release
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_isDragging = false;
        m_draggedSprite = nullptr;
    }
    
    // Handle sprite dragging
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && m_isDragging && m_draggedSprite) {
        sf::Vector2f newPosition = sf::Vector2f(worldPos.x - m_dragOffset.x, worldPos.y - m_dragOffset.y);
        
        Transform transform = m_draggedSprite->getLocalTransform();
        transform.x = newPosition.x;
        transform.y = newPosition.y;
        m_draggedSprite->setTransform(transform);
    }
}

void ScenePanel::handleBindingInteraction(const sf::Vector2f& worldPos) {
    // Handle sprite-bone binding interactions
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // First, check if clicking on a sprite
        Sprite* clickedSprite = getSpriteAtPosition(worldPos);
        if (clickedSprite) {
            m_selectedSprite = clickedSprite;
            m_selectedBone = nullptr;
            
            // Show context menu for sprite binding
            m_spriteToBindFrom = clickedSprite;
            m_showBindingUI = true;
            
            if (m_onSpriteSelected) {
                m_onSpriteSelected(clickedSprite);
            }
            
            std::cout << "Selected sprite for binding: " << clickedSprite->getName() << std::endl;
            return;
        }
        
        // Then check if clicking on a bone
        auto clickedBone = m_boneCreationTool->findBoneAtPosition(worldPos);
        if (clickedBone) {
            m_selectedBone = clickedBone;
            m_selectedSprite = nullptr;
            
            // Show context menu for bone binding
            m_boneToBindFrom = clickedBone;
            m_showBindingUI = true;
            
            if (m_onBoneSelected) {
                m_onBoneSelected(clickedBone);
            }
            
            std::cout << "Selected bone for binding: " << clickedBone->getName() << std::endl;
            return;
        }
        
        // Clicked on empty space - deselect
        m_selectedSprite = nullptr;
        m_selectedBone = nullptr;
        m_showBindingUI = false;
        m_spriteToBindFrom = nullptr;
        m_boneToBindFrom = nullptr;
        
        if (m_onSpriteSelected) m_onSpriteSelected(nullptr);
        if (m_onBoneSelected) m_onBoneSelected(nullptr);
    }
}

Sprite* ScenePanel::getSpriteAtPosition(const sf::Vector2f& worldPos) {
    if (!m_character) return nullptr;

    const auto& sprites = m_character->getSprites();
    for (auto it = sprites.rbegin(); it != sprites.rend(); ++it) {
        const auto& sprite = *it;
        const auto& vertices = sprite->getDeformedVertices();
        
        if (vertices.size() >= 4) {
            if (isPointInQuad(worldPos, vertices)) {
                return sprite.get();
            }
        }
    }

    return nullptr;
}

bool ScenePanel::isPointInQuad(const sf::Vector2f& point, const std::vector<Vertex>& vertices) const {
    if (vertices.size() < 4) return false;
    
    float minX = vertices[0].x, maxX = vertices[0].x;
    float minY = vertices[0].y, maxY = vertices[0].y;
    
    for (const auto& vertex : vertices) {
        minX = std::min(minX, vertex.x);
        maxX = std::max(maxX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }
    
    if (point.x < minX || point.x > maxX || point.y < minY || point.y > maxY) {
        return false;
    }
    
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
    // This is now handled in handleSpriteInteraction() method
}

void ScenePanel::renderGrid(sf::RenderTarget& target) {
    sf::Vector2f viewSize = m_view.getSize();
    sf::Vector2f viewCenter = m_view.getCenter();
    
    float gridSize = 50.0f;
    sf::Color gridColor(100, 100, 100, 100);
    
    float startX = viewCenter.x - viewSize.x * 0.5f;
    float endX = viewCenter.x + viewSize.x * 0.5f;
    float startY = viewCenter.y - viewSize.y * 0.5f;
    float endY = viewCenter.y + viewSize.y * 0.5f;
    
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

void ScenePanel::renderBoneCreationPreview(sf::RenderTarget& target) {
    if (m_boneCreationTool && m_boneCreationTool->isCreating()) {
        sf::Vector2f start = m_boneCreationTool->getCreationStart();
        sf::Vector2f end = m_boneCreationTool->getCreationEnd();
        
        // Draw preview line
        sf::Vertex previewLine[2];
        previewLine[0].position = start;
        previewLine[0].color = sf::Color::Green;
        previewLine[1].position = end;
        previewLine[1].color = sf::Color::Green;
        
        target.draw(previewLine, 2, sf::PrimitiveType::Lines);
        
        // Draw start point
        sf::CircleShape startPoint(5.0f);
        startPoint.setPosition(sf::Vector2f(start.x - 5.0f, start.y - 5.0f));
        startPoint.setFillColor(sf::Color::Green);
        target.draw(startPoint);
        
        // Draw end point
        sf::CircleShape endPoint(3.0f);
        endPoint.setPosition(sf::Vector2f(end.x - 3.0f, end.y - 3.0f));
        endPoint.setFillColor(sf::Color::Yellow);
        target.draw(endPoint);
    }
}

void ScenePanel::initializeViewport(const sf::Vector2u& size) {
    m_renderTexture.resize(size);
    m_viewportInitialized = true;
    m_renderTexture.setSmooth(true);
}

void ScenePanel::bindSpriteToSelectedBone() {
    if (!m_selectedSprite || !m_selectedBone) {
        std::cout << "Error: No sprite or bone selected for binding" << std::endl;
        return;
    }
    
    m_selectedSprite->bindToBone(m_selectedBone, 1.0f);
    
    if (m_onSpriteBindingChanged) {
        m_onSpriteBindingChanged(m_selectedSprite, m_selectedBone);
    }
    
    std::cout << "Bound sprite '" << m_selectedSprite->getName() 
              << "' to bone '" << m_selectedBone->getName() << "'" << std::endl;
}

void ScenePanel::unbindSelectedSprite() {
    if (!m_selectedSprite) {
        std::cout << "Error: No sprite selected for unbinding" << std::endl;
        return;
    }
    
    m_selectedSprite->clearAllBindings();
    
    if (m_onSpriteBindingChanged) {
        m_onSpriteBindingChanged(m_selectedSprite, nullptr);
    }
    
    std::cout << "Unbound all bones from sprite '" << m_selectedSprite->getName() << "'" << std::endl;
}

void ScenePanel::showSpriteContextMenu(Sprite* sprite, const sf::Vector2f& mousePos) {
    if (!sprite) return;
    
    // This could be implemented with ImGui popup menus in the future
    std::cout << "Context menu for sprite: " << sprite->getName() << std::endl;
    
    // For now, just select the sprite and show binding UI
    m_selectedSprite = sprite;
    m_showBindingUI = true;
    
    if (m_onSpriteSelected) {
        m_onSpriteSelected(sprite);
    }
}

void ScenePanel::showBoneContextMenu(std::shared_ptr<Bone> bone, const sf::Vector2f& mousePos) {
    if (!bone) return;
    
    // This could be implemented with ImGui popup menus in the future
    std::cout << "Context menu for bone: " << bone->getName() << std::endl;
    
    // For now, just select the bone and show binding UI
    m_selectedBone = bone;
    m_showBindingUI = true;
    
    if (m_onBoneSelected) {
        m_onBoneSelected(bone);
    }
}

void ScenePanel::handleIKInteraction(const sf::Vector2f& worldPos) {
    if (!m_character || !m_character->getRig()) return;
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {  // FIXED: Use ImGui mouse detection
        if (!m_ikDragging) {
            // Select end effector bone
            const auto& bones = m_character->getRig()->getAllBones();
            for (const auto& bone : bones) {
                float startX, startY, endX, endY;
                bone->getWorldEndpoints(startX, startY, endX, endY);
                
                // Check if clicking near bone end (for end effector selection)
                float distToEnd = std::sqrt((worldPos.x - endX) * (worldPos.x - endX) + 
                                          (worldPos.y - endY) * (worldPos.y - endY));
                
                if (distToEnd < 15.0f) {
                    m_ikTarget = bone;
                    m_ikTargetPos = worldPos;
                    m_ikDragging = true;
                    std::cout << "Selected IK target: " << bone->getName() << std::endl;
                    break;
                }
            }
        }
    }
    
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && m_ikDragging) {  // FIXED: Separate dragging logic
        // Update target position and solve IK
        m_ikTargetPos = worldPos;
        
        if (m_ikTarget) {
            IKSolverSettings settings;
            settings.maxIterations = 10;
            settings.tolerance = 5.0f;
            settings.dampening = 0.7f;
            
            Vector2 target(m_ikTargetPos.x, m_ikTargetPos.y);
            bool success = IK_Solver::solveCCD(m_ikTarget, target, settings);
            
            // Force character deformation update
            if (m_character) {
                m_character->forceUpdateDeformations();
            }
        }
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {  // FIXED: Proper mouse release
        m_ikDragging = false;
    }
}

void ScenePanel::renderIKTargets(sf::RenderTarget& target) {
    if (m_toolMode != SceneToolMode::IKSolver || !m_character || !m_character->getRig()) return;
    
    const auto& bones = m_character->getRig()->getAllBones();
    
    // Draw end effector targets
    for (const auto& bone : bones) {
        float startX, startY, endX, endY;
        bone->getWorldEndpoints(startX, startY, endX, endY);
        
        // Draw end effector handle
        sf::CircleShape endHandle(8.0f);
        endHandle.setPosition(sf::Vector2f(endX - 8.0f, endY - 8.0f));
        
        if (bone == m_ikTarget) {
            endHandle.setFillColor(sf::Color::Red);
            endHandle.setOutlineColor(sf::Color::White);
        } else {
            endHandle.setFillColor(sf::Color(255, 165, 0)); // Orange
            endHandle.setOutlineColor(sf::Color::Black);
        }
        endHandle.setOutlineThickness(2.0f);
        target.draw(endHandle);
    }
    
    // Draw IK target position
    if (m_ikDragging && m_ikTarget) {
        sf::CircleShape targetViz(12.0f);
        targetViz.setPosition(sf::Vector2f(m_ikTargetPos.x - 12.0f, m_ikTargetPos.y - 12.0f));
        targetViz.setFillColor(sf::Color(255, 0, 0, 100)); // Semi-transparent red
        targetViz.setOutlineColor(sf::Color::Red);
        targetViz.setOutlineThickness(3.0f);
        target.draw(targetViz);
        
        // Draw line from current end effector to target
        if (m_ikTarget) {
            float startX, startY, endX, endY;
            m_ikTarget->getWorldEndpoints(startX, startY, endX, endY);
            
            sf::Vertex line[2];
            line[0].position = sf::Vector2f(endX, endY);
            line[0].color = sf::Color::Red;
            line[1].position = m_ikTargetPos;
            line[1].color = sf::Color::Red;
            
            target.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }
}

void ScenePanel::renderIKUI() {
    if (m_toolMode != SceneToolMode::IKSolver) return;
    
    ImGui::Begin("IK Solver", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Inverse Kinematics Solver");
    ImGui::Separator();
    
    if (m_ikTarget) {
        ImGui::Text("Target Bone: %s", m_ikTarget->getName().c_str());
        ImGui::Text("Target Position: (%.1f, %.1f)", m_ikTargetPos.x, m_ikTargetPos.y);
        
        if (ImGui::Button("Clear Target")) {
            m_ikTarget = nullptr;
            m_ikDragging = false;
        }
        
        ImGui::Separator();
        
        // IK Settings
        static int maxIter = 10;
        static float tolerance = 5.0f;
        static float dampening = 0.7f;
        
        ImGui::SliderInt("Max Iterations", &maxIter, 1, 50);
        ImGui::SliderFloat("Tolerance", &tolerance, 0.1f, 20.0f);
        ImGui::SliderFloat("Dampening", &dampening, 0.1f, 1.0f);
        
        if (ImGui::Button("Solve IK")) {
            IKSolverSettings settings;
            settings.maxIterations = maxIter;
            settings.tolerance = tolerance;
            settings.dampening = dampening;
            
            Vector2 target(m_ikTargetPos.x, m_ikTargetPos.y);
            bool success = IK_Solver::solveCCD(m_ikTarget, target, settings);
            
            if (success) {
                std::cout << "IK solved successfully!" << std::endl;
            } else {
                std::cout << "IK did not converge" << std::endl;
            }
        }
    } else {
        ImGui::Text("Click on a bone end to select IK target");
        ImGui::Text("Then drag to set target position");
    }
    
    ImGui::Separator();
    ImGui::Text("Instructions:");
    ImGui::Text("1. Click on bone end to select target");
    ImGui::Text("2. Drag to move IK target");
    ImGui::Text("3. IK will solve in real-time");
    
    ImGui::End();
}

} // namespace Riggle