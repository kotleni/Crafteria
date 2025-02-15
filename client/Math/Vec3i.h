#ifndef H_VEC3I
#define H_VEC3I

#include <cmath>

class Vec3i {
public:
    Vec3i(int x, int y, int z);

    bool operator==(const Vec3i &other) const;
    Vec3i operator+(const Vec3i &other) const;
    Vec3i operator*(int i) const;

    [[nodiscard]] double distanceTo(const Vec3i& vec3_i) const;

    int x;
    int y;
    int z;
};

#endif