#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>
#include "Ray.hpp"

Ray::Ray()
{
  p0_ = Point();
  d_ = Vector3();
}
Ray::Ray(Point p0, Vector3 d)
{
  p0_ = p0;
  d_ = d;
  d_.normalize();
}

Point Ray::calc_point(float t)
{
  float p1, p2, p3, d1, d2, d3;
  p0_.get_coordinates(&p1, &p2, &p3);
  d_.get_coordinates(&d1, &d2, &d3);
  return Point(p1 + t*d1, p2 + t*d2, p3 + t*d3);
}

Point Ray::get_p0() { return p0_; }
Vector3 Ray::get_d() { return d_; }

void Ray::set_params(Point* p0, Vector3* d)
{
  p0_ = *p0;
  d_ = *d;
}
Ray Ray::calc_reflection(Point p_int, Vector3 n)
{
  Vector3 v = d_;
  Vector3 k = n * (-2*v.dot_product(&n));

  Vector3 rd = k + v;
  return Ray(p_int, rd);
}
Ray Ray::calc_refraction(Point p_int, Vector3 n, float ior)
{
  Vector3 I = d_ * -1;
  float cosi = I.dot_product(&n);
  float eta_i = 1.0f, eta_t = ior;
  Vector3 n_rf = n;
  if (cosi < 0) {
    cosi = -cosi;
  } else {
    std::swap(eta_i, eta_t);
    n_rf = n * -1;
  }
  float eta = eta_t / eta_i;
  float k = 1 - eta * eta * (1 - cosi * cosi);
  Vector3 f1 = I * eta;
  Vector3 f2 = n_rf * (eta * cosi - sqrtf(k));
  Vector3 dir = k < 0 ? Vector3() : f1 + f2;
  return Ray(p_int, dir);
}
