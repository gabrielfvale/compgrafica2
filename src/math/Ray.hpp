#ifndef RAY_HPP
#define RAY_HPP

#include <vector>
#include "Point.hpp"
#include "Vector3.hpp"

class Ray
{
  private:
    Point p0_; // known point of the ray
    Vector3 d_; // unit vector that determines the ray's direction
  public:
    Ray();
    Ray(Point p0, Vector3 d);
    Point calc_point(float t);
    Point get_p0();
    Vector3 get_d();
    void set_params(Point* p0, Vector3* d);
    Ray calc_reflection(Point p_int, Vector3 n);
    Ray calc_refraction(Point p_int, Vector3 n, float ior, bool& has_refr);
    void fresnel(Vector3& I, Vector3& N, float& ior, float& kr);
};

#endif
