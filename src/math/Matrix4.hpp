#ifndef MATRIX4_HPP
#define MATRIX4_HPP

#include "Point.hpp"
#include "Vector3.hpp"
#include "Quaternion.hpp"

class Matrix4
{
  private:
    float m[4][4];
  public:
    Matrix4();
    Matrix4 operator+(Matrix4& m);
    Matrix4 operator-(Matrix4& m);
    Matrix4 operator*(Matrix4& m);
    float& operator()(int i, int j);
    Point operator*(Point& p);
    Vector3 operator*(Vector3& v);
    Quaternion operator*(Quaternion& q);
    void identity();
    Matrix4 inverse();
    friend std::ostream& operator<< (std::ostream& stream, Matrix4& matrix);
};

#endif
