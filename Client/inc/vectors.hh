#pragma once

struct Vector3 {
    float X, Y, Z;

    float Magnitude();
    Vector3 Normalized();

    Vector3 Up(), Right(), Forward();
};

extern Vector3 operator+(Vector3, Vector3);
extern Vector3 operator-(Vector3, Vector3);
extern Vector3 operator*(Vector3, Vector3);
extern Vector3 operator/(Vector3, Vector3);
extern Vector3 operator-(Vector3);
extern Vector3 operator*(Vector3, float);
extern Vector3 operator/(Vector3, float);
extern Vector3 operator*(float, Vector3);
extern Vector3 operator/(float, Vector3);


struct Vector2 {
    float X, Y;

    float Magnitude();
    Vector2 Normalized();

    Vector2 Up(), Right();
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
