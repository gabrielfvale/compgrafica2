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
Ray Ray::calc_refraction(Point p_int, Vector3 n, float ior, bool& has_refr)
{
  has_refr = false;
  Vector3 I = d_;
  Vector3 N = n;

  float cosi = I.dot_product(&N);

  if (cosi < -1) cosi = -1; 
  if (cosi > 1) cosi = 1; 

  float etai = 1, etat = ior; 

  if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); N= N * -1; } 
  float eta = etai / etat;
  float k = 1 - eta * eta * (1 - cosi * cosi);
  Vector3 t1 = (I * eta);
  Vector3 t2 = N * (eta * cosi - sqrtf(k));
  Vector3 dir;
  if (k >= 0) {
    dir = t1 + t2;
    has_refr = true;
  }
  return Ray(p_int, dir); 
}

void Ray::fresnel(Vector3& I, Vector3& N, float& ior, float& kr)
{
  float cosi = I.dot_product(&N);
  if (cosi < -1) cosi = -1; 
  if (cosi > 1) cosi = 1; 

  float etai = 1.0f, etat = ior;
  if (cosi > 0) std::swap(etai, etat);

  float sint = etai / etat * sqrtf(std::max(0.0f, 1 - cosi * cosi));

  if (sint >= 1) {
    kr = 1;
  } else {
    float cost = sqrtf(std::max(0.0f, 1 - sint * sint));
    cosi = fabsf(cosi);
    float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost)); 
    float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
    kr = (Rs * Rs + Rp * Rp) / 2;
  }
}
