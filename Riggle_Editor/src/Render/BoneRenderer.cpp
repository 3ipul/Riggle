#include "Editor/Render/BoneRenderer.h"
#include <SFML/Graphics.hpp>
#include <cmath>

namespace Riggle {

BoneRenderer::BoneRenderer() {
}

void BoneRenderer::render(sf::RenderTarget& target, const Rig& rig) {
    // Render all bones in the rig
    for (const auto& bone : rig.getAllBones()) {
        renderBone(target, *bone);
    }
}

void BoneRenderer::renderBone(sf::RenderTarget& target, const Bone& bone) {
    // Get world transform
    Transform transform = bone.getWorldTransform();
    
    // Calculate end point based on bone length and rotation
    float endX = transform.x + std::cos(transform.rotation) * transform.length;
    float endY = transform.y + std::sin(transform.rotation) * transform.length;
    
    // Draw bone line
    drawBoneLine(target, transform.x, transform.y, endX, endY);
    
    // Draw joint at start position
    drawJoint(target, transform.x, transform.y);
    
    // Draw joint at end position if this bone has no children
    if (bone.getChildren().empty()) {
        drawJoint(target, endX, endY, 3.0f); // Smaller radius for end joints
    }
}

void BoneRenderer::drawBoneLine(sf::RenderTarget& target, float startX, float startY, float endX, float endY) {
    // Create a line using sf::Vertex array
    sf::Vertex line[2];
    line[0].position = sf::Vector2f(startX, startY);
    line[0].color = sf::Color::Red;
    line[1].position = sf::Vector2f(endX, endY);
    line[1].color = sf::Color::Yellow;
    
    target.draw(line, 2, sf::PrimitiveType::Lines);
}

void BoneRenderer::drawJoint(sf::RenderTarget& target, float x, float y, float radius) {
    sf::CircleShape joint(radius);
    joint.setPosition(sf::Vector2f(x - radius, y - radius)); // Center the circle
    joint.setFillColor(sf::Color::Green);
    joint.setOutlineColor(sf::Color::White);
    joint.setOutlineThickness(1.0f);
    
    target.draw(joint);
}

} // namespace Riggle