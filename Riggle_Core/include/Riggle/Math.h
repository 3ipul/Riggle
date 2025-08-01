#pragma once
#include <cmath>

namespace Riggle {

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
    
    Vector2() = default;
    Vector2(float x, float y) : x(x), y(y) {}
    
    // Basic operations
    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
    Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(float scalar) const { return Vector2(x / scalar, y / scalar); }
    
    // Assignment operators
    Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
    Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }
    Vector2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    
    // Dot product
    float dot(const Vector2& other) const { return x * other.x + y * other.y; }
    
    // Cross product (2D returns scalar)
    float cross(const Vector2& other) const { return x * other.y - y * other.x; }
    
    // Length
    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSquared() const { return x * x + y * y; }
    
    // Normalize
    Vector2 normalized() const {
        float len = length();
        if (len < 0.001f) return Vector2(0, 0);
        return *this / len;
    }
};

// Consistent Transform struct with Vector2 for position and scale
struct Transform {
    Vector2 position = {0.0f, 0.0f};  // Position as Vector2
    float rotation = 0.0f;            // Rotation in radians
    Vector2 scale = {1.0f, 1.0f};     // Scale as Vector2
    float length = 50.0f;             // Length (used by bones)
    
    Transform() = default;
    Transform(float x, float y, float rotation = 0.0f, float scaleX = 1.0f, float scaleY = 1.0f, float length = 50.0f)
        : position(x, y), rotation(rotation), scale(scaleX, scaleY), length(length) {}
    Transform(const Vector2& pos, float rot = 0.0f, const Vector2& scl = {1.0f, 1.0f}, float len = 50.0f)
        : position(pos), rotation(rot), scale(scl), length(len) {}
    
    // Legacy accessors as getter/setter methods (if needed for backward compatibility)
    float getX() const { return position.x; }
    float getY() const { return position.y; }
    float getScaleX() const { return scale.x; }
    float getScaleY() const { return scale.y; }
    
    void setX(float x) { position.x = x; }
    void setY(float y) { position.y = y; }
    void setScaleX(float sx) { scale.x = sx; }
    void setScaleY(float sy) { scale.y = sy; }
};

} // namespace Riggle