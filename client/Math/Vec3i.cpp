#include "Vec3i.h"

Vec3i::Vec3i(int x, int y, int z) {
  this->x = x;
  this->y = y;
  this->z = z;
}

Vec3i::Vec3i(glm::vec<3, int> vec) {
  this->x = vec.x;
  this->y = vec.y;
  this->z = vec.z;
}

bool Vec3i::operator==(const Vec3i &other) const {
  return this->x == other.x && this->y == other.y && this->z == other.z;
}

Vec3i Vec3i::operator+(const Vec3i &other) const {
  return {this->x + other.x, this->y + other.y, this->z + other.z};
}

Vec3i Vec3i::operator-(const Vec3i &other) const {
  return {this->x - other.x, this->y - other.y, this->z - other.z};
}

[[nodiscard]] double Vec3i::distanceTo(const Vec3i& vec3_i) const {
  int dx = this->x - vec3_i.x;
  int dy = this->y - vec3_i.y;
  int dz = this->z - vec3_i.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Vec3i Vec3i::operator*(int i) const {
  return {this->x * i, this->y * i, this->z * i};
}
