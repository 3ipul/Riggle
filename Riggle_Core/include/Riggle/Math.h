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

// Unified Transform struct with all necessary fields
struct Transform {
    float x = 0.0f;        // X position
    float y = 0.0f;        // Y position
    float rotation = 0.0f; // Rotation in radians
    float scaleX = 1.0f;   // X scale factor
    float scaleY = 1.0f;   // Y scale factor
    float length = 50.0f;  // Length (used by bones)
    
    Transform() = default;
    Transform(float x, float y, float rotation = 0.0f, float scaleX = 1.0f, float scaleY = 1.0f, float length = 50.0f)
        : x(x), y(y), rotation(rotation), scaleX(scaleX), scaleY(scaleY), length(length) {}
};

} // namespace Riggle