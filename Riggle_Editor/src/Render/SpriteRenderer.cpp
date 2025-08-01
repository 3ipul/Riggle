#include "Editor/Render/SpriteRenderer.h"
#include <iostream>

namespace Riggle {

SpriteRenderer::SpriteRenderer()
    : m_character(nullptr)
{
    createDefaultTexture();
}

void SpriteRenderer::render(sf::RenderTarget& target) {
    if (!m_character) return;
    
    // Render all sprites
    for (auto& sprite : m_character->getSprites()) {
        if (sprite->isVisible()) {
            renderSprite(target, sprite.get());
        }
    }
}

void SpriteRenderer::renderSprite(sf::RenderTarget& target, Sprite* sprite) {
    if (!sprite) return;
    
    // Get texture
    sf::Texture* texture = getTexture(sprite->getTexturePath());
    if (!texture) {
        texture = &m_defaultTexture; // Use default if texture not found
    }
    
    // Create sprite
    sf::Sprite sfSprite(*texture);
    
    // Get world transform
    Transform worldTransform = sprite->getWorldTransform();
    
    // Apply transform
   sfSprite.setPosition(sf::Vector2f(worldTransform.position.x, worldTransform.position.y));
    sfSprite.setRotation(sf::degrees(worldTransform.rotation * 180.0f / 3.14159f));
    sfSprite.setScale(sf::Vector2f(worldTransform.scale.x, worldTransform.scale.y));
    
    // Center origin
    sf::Vector2u textureSize = texture->getSize();
    sfSprite.setOrigin(sf::Vector2f(textureSize.x * 0.5f, textureSize.y * 0.5f));
    
    // Draw sprite
    target.draw(sfSprite);
}

void SpriteRenderer::renderSpriteHighlight(sf::RenderTarget& target, Sprite* sprite) {
    if (!sprite) return;
    
    // Get texture
    sf::Texture* texture = getTexture(sprite->getTexturePath());
    if (!texture) {
        texture = &m_defaultTexture;
    }
    
    // Create highlighted sprite
    sf::Sprite sfSprite(*texture);
    
    // Get world transform
    Transform worldTransform = sprite->getWorldTransform();
    
    // Apply transform
    sfSprite.setPosition(sf::Vector2f(worldTransform.position.x, worldTransform.position.y));
    sfSprite.setRotation(sf::degrees(worldTransform.rotation * 180.0f / 3.14159f));
    sfSprite.setScale(sf::Vector2f(worldTransform.scale.x, worldTransform.scale.y));
    
    // Center origin
    sf::Vector2u textureSize = texture->getSize();
    sfSprite.setOrigin(sf::Vector2f(textureSize.x * 0.5f, textureSize.y * 0.5f));

    // Add highlight effect (tint with selection color)
    sfSprite.setColor(sf::Color(255, 100, 100, 200)); // Red tint
    
    target.draw(sfSprite);
    
    // Draw selection outline
    sf::RectangleShape outline;
    outline.setSize(sf::Vector2f(textureSize.x * worldTransform.scale.x, 
                                textureSize.y * worldTransform.scale.y));
    outline.setOrigin(sf::Vector2f(outline.getSize().x * 0.5f, outline.getSize().y * 0.5f));
    outline.setPosition(sf::Vector2f(worldTransform.position.x, worldTransform.position.y));
    outline.setRotation(sf::degrees(worldTransform.rotation * 180.0f / 3.14159f));
    outline.setFillColor(sf::Color::Transparent);
    outline.setOutlineThickness(2.0f);
    outline.setOutlineColor(sf::Color::Red);
    
    target.draw(outline);
}

sf::Texture* SpriteRenderer::getTexture(const std::string& path) {
    if (path.empty()) return &m_defaultTexture;
    
    // Check cache first
    auto it = m_textureCache.find(path);
    if (it != m_textureCache.end()) {
        return it->second.get();
    }
    
    // Load new texture
    return loadTexture(path);
}

sf::Texture* SpriteRenderer::loadTexture(const std::string& path) {
    auto texture = std::make_unique<sf::Texture>();
    
    if (texture->loadFromFile(path)) {
        sf::Texture* result = texture.get();
        m_textureCache[path] = std::move(texture);
        std::cout << "Loaded texture: " << path << std::endl;
        return result;
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
        return &m_defaultTexture;
    }
}

void SpriteRenderer::clearTextureCache() {
    m_textureCache.clear();
}

void SpriteRenderer::createDefaultTexture() {
    // Create a simple 64x64 checkerboard pattern
    const unsigned int size = 64;
    sf::Image image;
    image.resize(sf::Vector2u(size, size));
    
    // Create checkerboard pattern
    for (unsigned int x = 0; x < size; ++x) {
        for (unsigned int y = 0; y < size; ++y) {
            bool checker = ((x / 8) + (y / 8)) % 2 == 0;
            sf::Color color = checker ? sf::Color::Magenta : sf::Color::White;
            image.setPixel(sf::Vector2u(x, y), color);
        }
    }
    
    m_defaultTexture.loadFromImage(image);
    std::cout << "Created default texture" << std::endl;
}

} // namespace Riggle