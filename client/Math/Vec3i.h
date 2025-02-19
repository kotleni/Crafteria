#ifndef H_VEC3I
#define H_VEC3I

#include <cmath>
#include <iomanip>

class Vec3i {
public:
    Vec3i(int x, int y, int z);

    bool operator==(const Vec3i &other) const;
    Vec3i operator-(const Vec3i &other) const;
    Vec3i operator+(const Vec3i &other) const;
    Vec3i operator*(int i) const;

    [[nodiscard]] double distanceTo(const Vec3i& vec3_i) const;

    int x;
    int y;
    int z;
};

template <>
struct std::hash<Vec3i>
{
    std::size_t operator()(const Vec3i& k) const
    {
        using std::size_t;
        using std::hash;

        return ((hash<int>{}(k.x)
                 ^ (hash<int>{}(k.y) << 1)) >> 1)
                 ^ (hash<int>{}(k.z) << 1);
    }
};

#endif