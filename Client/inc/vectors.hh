#pragma once

struct Vector3 {
    float X, Y, Z;

    float magnitude();
    Vector3 normalized();

    Vector3 up(), right(), forward();
};

extern Vector3 operator+(const Vector3&, const Vector3&);
extern Vector3 operator-(const Vector3&, const Vector3&);
extern Vector3 operator*(const Vector3&, const Vector3&);
extern Vector3 operator/(const Vector3&, const Vector3&);
extern Vector3 operator-(const Vector3&);
extern Vector3 operator*(const Vector3&, float);
extern Vector3 operator/(const Vector3&, float);
extern Vector3 operator*(float, const Vector3&);
extern Vector3 operator/(float, const Vector3&);

extern Vector3& operator+=(Vector3&, const Vector3&);
extern Vector3& operator-=(Vector3&, const Vector3&);
extern Vector3& operator*=(Vector3&, const Vector3&);
extern Vector3& operator/=(Vector3&, const Vector3&);
extern Vector3& operator*=(Vector3&, float);
extern Vector3& operator/=(Vector3&, float);

struct Vector2 {
    float X, Y;

    float magnitude();
    Vector2 normalized();

    Vector2 up(), right();
};

extern Vector2 operator+(Vector2, Vector2);
extern Vector2 operator-(Vector2, Vector2);
extern Vector2 operator*(Vector2, Vector2);
extern Vector2 operator/(Vector2, Vector2);
extern Vector2 operator-(Vector2);
extern Vector2 operator*(Vector2, float);
extern Vector2 operator/(Vector2, float);
extern Vector2 operator*(float, Vector2);
extern Vector2 operator/(float, Vector2);
