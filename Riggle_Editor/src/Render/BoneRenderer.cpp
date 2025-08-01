#include "Editor/Render/BoneRenderer.h"
#include <iostream>
#include <cmath>
#include <cstdint>

namespace Riggle {

BoneRenderer::BoneRenderer()
    : m_character(nullptr)
    , m_showBoneNames(false)
    , m_showJoints(true)
    , m_boneThickness(3.0f)
    , m_boneColor(255, 255, 255, 200)      // White
    , m_jointColor(100, 255, 100, 255)     // Green
    , m_selectedBoneColor(255, 100, 100, 255)  // Red
    , m_rootBoneColor(100, 100, 255, 255)  // Blue
    , m_fontLoaded(false)
{
    // Try to load a font for bone names (optional)
    // skip this for now, as it requires an asset
    // if (m_font.loadFromFile("assets/fonts/arial.ttf")) {
    //     m_fontLoaded = true;
    // }
}

void BoneRenderer::render(sf::RenderTarget& target, float zoomLevel) {
    if (!m_character || !m_character->getRig()) return;
    
    const auto& rootBones = m_character->getRig()->getRootBones();
    
    // Render all bones
    for (const auto& rootBone : rootBones) {
        renderBone(target, rootBone, zoomLevel);
    }
}

void BoneRenderer::renderBone(sf::RenderTarget& target, std::shared_ptr<Bone> bone, float zoomLevel) {
    if (!bone) return;
    
    // Get bone world endpoints
    float startX, startY, endX, endY;
    bone->getWorldEndpoints(startX, startY, endX, endY);
    
    sf::Vector2f start(startX, startY);
    sf::Vector2f end(endX, endY);
    
    // Choose color based on bone type
    sf::Color boneColor = m_boneColor;
    if (!bone->getParent()) {
        boneColor = m_rootBoneColor; // Root bone
    }
    
    // Render bone as triangle (thicker at start, pointed at end)
    renderBoneTriangle(target, start, end, boneColor, m_boneThickness, zoomLevel);
    
    // Render joints with zoom-adjusted size
    if (m_showJoints) {
        float jointSize = 6.0f / zoomLevel;  // Maintain visual size
        float endJointSize = 3.0f / zoomLevel;
        renderJoint(target, start, m_jointColor, jointSize);
        renderJoint(target, end, m_jointColor, endJointSize);
    }
    
    // Render bone name
    if (m_showBoneNames) {
        sf::Vector2f midPoint = (start + end) * 0.5f;
        renderBoneName(target, bone, midPoint);
    }
    
    // Recursively render children
    for (const auto& child : bone->getChildren()) {
        renderBone(target, child, zoomLevel);
    }
}

void BoneRenderer::renderBoneTriangle(sf::RenderTarget& target, const sf::Vector2f& start, const sf::Vector2f& end, 
                                    const sf::Color& color, float thickness, float zoomLevel) {
    sf::Vector2f direction = end - start;
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    // Adjust thickness based on zoom level to maintain visual size
    float adjustedThickness = thickness / zoomLevel;
    
    if (length < 0.1f) {
        // Too short, render as circle with adjusted size
        sf::CircleShape circle(adjustedThickness * 0.5f);
        circle.setFillColor(color);
        circle.setOrigin(sf::Vector2f(adjustedThickness * 0.5f, adjustedThickness * 0.5f));
        circle.setPosition(start);
        target.draw(circle);
        return;
    }
    
    direction /= length;
    sf::Vector2f perpendicular(-direction.y, direction.x);
    
    // Create triangle - thick at start, pointed at end
    float baseWidth = adjustedThickness;
    float tipWidth = adjustedThickness * 0.2f; // Very narrow at tip
    
    sf::Vector2f baseLeft = start + perpendicular * (baseWidth * 0.5f);
    sf::Vector2f baseRight = start - perpendicular * (baseWidth * 0.5f);
    sf::Vector2f tipLeft = end + perpendicular * (tipWidth * 0.5f);
    sf::Vector2f tipRight = end - perpendicular * (tipWidth * 0.5f);
    
    // Create triangle strip (two triangles forming the bone shape)
    std::array<sf::Vertex, 6> vertices;
    
    vertices[0].position = baseLeft;
    vertices[0].color = color;
    vertices[1].position = baseRight;
    vertices[1].color = color;
    vertices[2].position = tipLeft;
    vertices[2].color = color;
    
    vertices[3].position = baseRight;
    vertices[3].color = color;
    vertices[4].position = tipRight;
    vertices[4].color = color;
    vertices[5].position = tipLeft;
    vertices[5].color = color;
    
    target.draw(vertices.data(), 6, sf::PrimitiveType::Triangles);
    
    // Add outline for better visibility (also zoom-adjusted)
    sf::Color outlineColor(color.r, color.g, color.b, static_cast<std::uint8_t>(color.a * 0.8f));
    
    std::array<sf::Vertex, 8> outline;
    outline[0].position = baseLeft;
    outline[0].color = outlineColor;
    outline[1].position = baseRight;
    outline[1].color = outlineColor;
    outline[2].position = baseRight;
    outline[2].color = outlineColor;
    outline[3].position = tipRight;
    outline[3].color = outlineColor;
    outline[4].position = tipRight;
    outline[4].color = outlineColor;
    outline[5].position = tipLeft;
    outline[5].color = outlineColor;
    outline[6].position = tipLeft;
    outline[6].color = outlineColor;
    outline[7].position = baseLeft;
    outline[7].color = outlineColor;
    
    target.draw(outline.data(), 8, sf::PrimitiveType::Lines);
}

void BoneRenderer::renderBoneHighlight(sf::RenderTarget& target, std::shared_ptr<Bone> bone, float zoomLevel) {
    if (!bone) return;
    
    float startX, startY, endX, endY;
    bone->getWorldEndpoints(startX, startY, endX, endY);
    
    sf::Vector2f start(startX, startY);
    sf::Vector2f end(endX, endY);
    
    // Render highlighted bone triangle (larger and different color) with zoom adjustment
    renderBoneTriangle(target, start, end, m_selectedBoneColor, m_boneThickness * 1.5f, zoomLevel);
    
    // Render highlighted joints (larger) with zoom adjustment
    float highlightJointSize = 10.0f / zoomLevel;
    float highlightEndJointSize = 6.0f / zoomLevel;
    renderJoint(target, start, m_selectedBoneColor, highlightJointSize);
    renderJoint(target, end, m_selectedBoneColor, highlightEndJointSize);
}

void BoneRenderer::renderBoneLine(sf::RenderTarget& target, const sf::Vector2f& start, const sf::Vector2f& end, 
                                 const sf::Color& color, float thickness) {
    sf::Vector2f direction = end - start;
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    if (length < 0.1f) return;
    
    direction /= length;
    sf::Vector2f perpendicular(-direction.y, direction.x);
    perpendicular *= thickness * 0.5f;
    
    // FIXED: SFML 3.0 Vertex constructor
    std::array<sf::Vertex, 4> vertices;
    vertices[0] = sf::Vertex();
    vertices[0].position = start + perpendicular;
    vertices[0].color = color;
    
    vertices[1] = sf::Vertex();
    vertices[1].position = start - perpendicular;
    vertices[1].color = color;
    
    vertices[2] = sf::Vertex();
    vertices[2].position = end - perpendicular;
    vertices[2].color = color;
    
    vertices[3] = sf::Vertex();
    vertices[3].position = end + perpendicular;
    vertices[3].color = color;
    
    target.draw(vertices.data(), 4, sf::PrimitiveType::TriangleStrip);
}

void BoneRenderer::renderJoint(sf::RenderTarget& target, const sf::Vector2f& position, 
                              const sf::Color& color, float radius) {
    sf::CircleShape circle(radius);
    circle.setFillColor(color);
    circle.setOrigin(sf::Vector2f(radius, radius));
    circle.setPosition(position);
    
    target.draw(circle);
}

void BoneRenderer::renderBoneName(sf::RenderTarget& target, std::shared_ptr<Bone> bone, const sf::Vector2f& position) {
    // if (!m_fontLoaded) return; // Skip if no font loaded
    
    // sf::Text text;
    // text.setFont(m_font);
    // text.setString(bone->getName());
    // text.setCharacterSize(12);
    // text.setFillColor(sf::Color::White);
    
    // // Center the text
    // sf::FloatRect bounds = text.getLocalBounds();
    // text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    // text.setPosition(position);
    
    // target.draw(text);
}

} // namespace Riggle