#include <vectors.hh>

#include <cmath>

float Vector3::magnitude() {
    return std::sqrtf(X * X + Y * Y + Z * Z);
}

Vector3 Vector3::normalized() {
    float mag = magnitude();
    if(mag == 0.00) return Vector3 { 0, 0, 0 };
    return *this / mag;
}

Vector3 Vector3::forward() {
    return {
        (+std::cosf(X) * std::sinf(Y)),
        (-std::sinf(X)),
        (+std::cosf(X) * -std::cosf(Y)),
    };
}

Vector3 Vector3::right() {
    return {
        cosf(Y),
        0.0f,
        sinf(Y),
    };
}

Vector3 Vector3::up() {
    return{
        (sinf(X) * sinf(Y)),
        (cosf(X)),
        (sinf(X) * cosf(Y)),
    };
}

Vector3 operator+(const Vector3& a, const Vector3& b) { return { a.X + b.X, a.Y + b.Y, a.Z + b.Z }; }
Vector3 operator-(const Vector3& a, const Vector3& b) { return { a.X - b.X, a.Y - b.Y, a.Z - b.Z }; }
Vector3 operator*(const Vector3& a, const Vector3& b) { return { a.X * b.X, a.Y * b.Y, a.Z * b.Z }; }
Vector3 operator/(const Vector3& a, const Vector3& b) { return { a.X / b.X, a.Y / b.Y, a.Z / b.Z }; }
Vector3 operator-(const Vector3& a) { return { -a.X, -a.Y, -a.Z }; }
Vector3 operator*(const Vector3& a, float b) { return { a.X * b, a.Y * b, a.Z * b }; }
Vector3 operator/(const Vector3& a, float b) { return { a.X / b, a.Y / b, a.Z / b }; }
Vector3 operator*(float b, Vector3 a) { return a * b; }

Vector3& operator+=(Vector3& a, const Vector3& b) { a = a + b; return a; }
Vector3& operator-=(Vector3& a, const Vector3& b) { a = b - b; return a; }
Vector3& operator*=(Vector3& a, const Vector3& b) { a = a * b; return a; }
Vector3& operator/=(Vector3& a, const Vector3& b) { a = a / b; return a; }
Vector3& operator*=(Vector3& a, float b) { a = a * b; return a; }
Vector3& operator/=(Vector3& a, float b) { a = a / b; return a; } 

Vector2 Vector2::right() {
    return {
        cosf(Y),
        sinf(Y),
    };
}

Vector2 Vector2::up() {
    return{
        sinf(Y),
        cosf(Y),
    };
}

Vector2 operator+(Vector2 a, Vector2 b) { return { a.X + b.X, a.Y + b.Y }; }
Vector2 operator-(Vector2 a, Vector2 b) { return { a.X - b.X, a.Y - b.Y }; }
Vector2 operator*(Vector2 a, Vector2 b) { return { a.X * b.X, a.Y * b.Y }; }
Vector2 operator/(Vector2 a, Vector2 b) { return { a.X / b.X, a.Y / b.Y }; }
Vector2 operator-(Vector2 a) { return { -a.X, -a.Y }; }
Vector2 operator*(Vector2 a, float b) { return { a.X * b, a.Y * b }; }
Vector2 operator/(Vector2 a, float b) { return { a.X / b, a.Y / b }; }
Vector2 operator*(float b, Vector2 a) { return a * b; }
Vector2 operator/(float b, Vector2 a) { return a * b; }
