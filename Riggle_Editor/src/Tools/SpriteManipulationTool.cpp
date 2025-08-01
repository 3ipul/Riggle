#include "Editor/Tools/SpriteManipulationTool.h"
#include <iostream>
#include <cmath>

namespace Riggle {

SpriteManipulationTool::SpriteManipulationTool()
    : m_character(nullptr)
    , m_isActive(false)
    , m_selectedSprite(nullptr)
    , m_draggedSprite(nullptr)
    , m_isDragging(false)
    , m_dragOffset(0, 0)
    , m_dragStartPos(0, 0)
{
}

void SpriteManipulationTool::setCharacter(Character* character) {
    m_character = character;
    clearSelection();
}

void SpriteManipulationTool::handleMousePressed(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_character) return;

    Sprite* clickedSprite = getSpriteAtPosition(worldPos);
    
    if (clickedSprite) {
        if (!clickedSprite->isBoundToBone()) {
            setSelectedSprite(clickedSprite);
            startDragging(clickedSprite, worldPos);
        } else {
            setSelectedSprite(clickedSprite);
            std::cout << "Selected bound sprite: " << clickedSprite->getName() 
                      << " (cannot drag - bound to bone)" << std::endl;
        }
    } else {
        clearSelection();
    }
}

void SpriteManipulationTool::handleMouseMoved(const sf::Vector2f& worldPos) {
    if (!m_isActive || !m_isDragging) return;
    
    updateDragging(worldPos);
}

void SpriteManipulationTool::handleMouseReleased() {
    if (!m_isActive) return;
    
    if (m_isDragging) {
        endDragging();
    }
}

void SpriteManipulationTool::setSelectedSprite(Sprite* sprite) {
    if (m_selectedSprite == sprite) return;
    
    m_selectedSprite = sprite;
    
    if (m_onSpriteSelected) {
        m_onSpriteSelected(sprite);
    }
    
    if (sprite) {
        std::cout << "Selected sprite: " << sprite->getName() << std::endl;
    } else {
        std::cout << "Cleared sprite selection" << std::endl;
    }
}

void SpriteManipulationTool::clearSelection() {
    setSelectedSprite(nullptr);
}

void SpriteManipulationTool::renderOverlay(sf::RenderTarget& target) {
    if (!m_isActive || !m_character) return;
    
    if (m_selectedSprite) {
        Transform transform = m_selectedSprite->getWorldTransform();
        
        // Convert Vector2 to sf::Vector2f
        sf::Vector2f position(transform.position.x, transform.position.y);
        
        sf::RectangleShape selectionRect;
        float width = 100.0f * transform.scale.x;
        float height = 100.0f * transform.scale.y;
        
        selectionRect.setSize(sf::Vector2f(width, height));
        selectionRect.setPosition(sf::Vector2f(position.x - width/2, position.y - height/2));
        selectionRect.setFillColor(sf::Color::Transparent);
        selectionRect.setOutlineColor(m_isDragging ? sf::Color::Yellow : sf::Color::Cyan);
        selectionRect.setOutlineThickness(2.0f);
        
        target.draw(selectionRect);
        
        sf::CircleShape centerPoint(3.0f);
        centerPoint.setPosition(sf::Vector2f(position.x - 3.0f, position.y - 3.0f));
        centerPoint.setFillColor(m_isDragging ? sf::Color::Yellow : sf::Color::Cyan);
        target.draw(centerPoint);
    }
}

Sprite* SpriteManipulationTool::getSpriteAtPosition(const sf::Vector2f& worldPos) {
    if (!m_character) return nullptr;
    
    const auto& sprites = m_character->getSprites();
    for (auto it = sprites.rbegin(); it != sprites.rend(); ++it) {
        if (isPointInSprite(worldPos, it->get())) {
            return it->get();
        }
    }
    
    return nullptr;
}

bool SpriteManipulationTool::isPointInSprite(const sf::Vector2f& point, Sprite* sprite) {
    if (!sprite) return false;
    
    Transform worldTransform = sprite->getWorldTransform();
    
    // Convert Vector2 to sf::Vector2f for comparison
    sf::Vector2f spritePos(worldTransform.position.x, worldTransform.position.y);
    
    float halfWidth = 50.0f * worldTransform.scale.x;
    float halfHeight = 50.0f * worldTransform.scale.y;
    
    return (point.x >= spritePos.x - halfWidth &&
            point.x <= spritePos.x + halfWidth &&
            point.y >= spritePos.y - halfHeight &&
            point.y <= spritePos.y + halfHeight);
}

void SpriteManipulationTool::startDragging(Sprite* sprite, const sf::Vector2f& worldPos) {
    if (!sprite || sprite->isBoundToBone()) return;
    
    m_draggedSprite = sprite;
    m_isDragging = true;
    m_dragStartPos = worldPos;
    
    Transform spriteTransform = sprite->getWorldTransform();
    sf::Vector2f spritePos(spriteTransform.position.x, spriteTransform.position.y);
    m_dragOffset = worldPos - spritePos;
    
    if (m_onSpriteDragStarted) {
        m_onSpriteDragStarted(sprite);
    }
    
    std::cout << "Started dragging sprite: " << sprite->getName() << std::endl;
    std::cout << "Drag offset: (" << m_dragOffset.x << ", " << m_dragOffset.y << ")" << std::endl;
}

void SpriteManipulationTool::updateDragging(const sf::Vector2f& worldPos) {
    if (!m_isDragging || !m_draggedSprite) return;
    
    sf::Vector2f newPosition = worldPos - m_dragOffset;
    
    Transform transform = m_draggedSprite->getLocalTransform();
    // Convert sf::Vector2f to Vector2
    transform.position = Vector2(newPosition.x, newPosition.y);
    m_draggedSprite->setTransform(transform);
}

void SpriteManipulationTool::endDragging() {
    if (m_isDragging && m_draggedSprite) {
        std::cout << "Finished dragging sprite: " << m_draggedSprite->getName() << std::endl;
        
        if (m_onSpriteDragEnded) {
            m_onSpriteDragEnded(m_draggedSprite);
        }
    }
    
    m_isDragging = false;
    m_draggedSprite = nullptr;
    m_dragOffset = sf::Vector2f(0, 0);
    m_dragStartPos = sf::Vector2f(0, 0);
}

} // namespace Riggle