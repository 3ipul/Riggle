#include "Editor/Render/SpriteRenderer.h"
#include <iostream>

namespace Riggle {

SpriteRenderer::SpriteRenderer() {
}

void SpriteRenderer::render(sf::RenderTarget& target, const Character& character) {
    for (const auto& sprite : character.getSprites()) {
        // Check visibility before rendering
        if (sprite->isVisible()) {
            renderSprite(target, *sprite);
        }
    }
}

void SpriteRenderer::renderSprite(sf::RenderTarget& target, const Sprite& sprite) {
    // Additional visibility check for safety
    if (!sprite.isVisible()) {
        return;
    }
    
    const auto& vertices = sprite.getDeformedVertices();
    if (vertices.size() < 4) return;

    // Try to get texture
    sf::Texture* texture = getTexture(sprite.getTexturePath());
    
    if (texture) {
        // Render with texture
        sf::VertexArray quad(sf::PrimitiveType::TriangleFan, 4);
        
        // Get texture size for UV coordinates
        sf::Vector2u textureSize = texture->getSize();
        
        // Map vertices to texture coordinates
        quad[0].position = toSFMLVector(vertices[0]);
        quad[0].texCoords = sf::Vector2f(0, 0);
        quad[0].color = sf::Color::White;
        
        quad[1].position = toSFMLVector(vertices[1]);
        quad[1].texCoords = sf::Vector2f(static_cast<float>(textureSize.x), 0);
        quad[1].color = sf::Color::White;
        
        quad[2].position = toSFMLVector(vertices[2]);
        quad[2].texCoords = sf::Vector2f(static_cast<float>(textureSize.x), static_cast<float>(textureSize.y));
        quad[2].color = sf::Color::White;
        
        quad[3].position = toSFMLVector(vertices[3]);
        quad[3].texCoords = sf::Vector2f(0, static_cast<float>(textureSize.y));
        quad[3].color = sf::Color::White;
        
        // Draw with texture
        sf::RenderStates states;
        states.texture = texture;
        target.draw(quad, states);
    } else {
        // Fallback: render as colored shape
        sf::ConvexShape shape;
        shape.setPointCount(vertices.size());
        
        for (size_t i = 0; i < vertices.size(); ++i) {
            shape.setPoint(i, toSFMLVector(vertices[i]));
        }
        
        shape.setFillColor(sf::Color(100, 150, 200, 128)); // Semi-transparent blue
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(1.0f);
        
        target.draw(shape);
    }
}

bool SpriteRenderer::loadTexture(const std::string& path) {
    if (m_textureCache.find(path) != m_textureCache.end()) {
        return true; // Already loaded
    }
    
    sf::Texture texture;
    if (texture.loadFromFile(path)) {
        m_textureCache[path] = std::move(texture);
        std::cout << "Loaded texture: " << path << std::endl;
        return true;
    }
    
    std::cout << "Failed to load texture: " << path << std::endl;
    return false;
}

void SpriteRenderer::clearTextureCache() {
    m_textureCache.clear();
}

sf::Texture* SpriteRenderer::getTexture(const std::string& path) {
    // Try to load texture if not already cached
    if (m_textureCache.find(path) == m_textureCache.end()) {
        if (!loadTexture(path)) {
            return nullptr;
        }
    }
    
    return &m_textureCache[path];
}

sf::Vector2f SpriteRenderer::toSFMLVector(const Riggle::Vertex& vertex) const {
    return sf::Vector2f(vertex.x, vertex.y);
}

} // namespace Riggle