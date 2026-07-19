#pragma once
#include <cmath>
#include <algorithm>

struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
    bool empty() const { return x == 0 && y == 0; }
};

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    float Distance(Vector3 other) const {
        return sqrtf(powf(x - other.x, 2) + powf(y - other.y, 2) + powf(z - other.z, 2));
    }

    float Dot(Vector3 other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    inline bool empty() const {
        return x == 0.0 && y == 0.0 && z == 0.0;
    }
};

struct Matrix {
    float m[4][4];
};

inline bool WorldToScreen(Vector3 world, Vector2& screen, Matrix viewMatrix, int width, int height) {
    float w = viewMatrix.m[0][3] * world.x + viewMatrix.m[1][3] * world.y + viewMatrix.m[2][3] * world.z + viewMatrix.m[3][3];

    if (w < 0.001f)
        return false;

    float x = viewMatrix.m[0][0] * world.x + viewMatrix.m[1][0] * world.y + viewMatrix.m[2][0] * world.z + viewMatrix.m[3][0];
    float y = viewMatrix.m[0][1] * world.x + viewMatrix.m[1][1] * world.y + viewMatrix.m[2][1] * world.z + viewMatrix.m[3][1];

    screen.x = (width / 2) * (1 + x / w);
    screen.y = (height / 2) * (1 - y / w);

    return true;
}
