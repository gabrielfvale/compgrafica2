#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <vector>
#include "Enums.hpp"
#include "Texture.hpp"
#include "RGB.hpp"

class Material
{
  private:
    Texture* texture = NULL;
  public:
    RGB ambient;
    RGB diffuse;
    RGB specular;
    float polish;
    float refraction;
    float shine;
    MatType type;
    Material();
    Material(RGB ambient, RGB diffuse, RGB specular, std::vector<float> properties = {0.0f, 0.0f, 1.0f}, MatType type = DIFFUSE);
    Material(RGB ambient, RGB specular, Texture* texture, std::vector<float> properties  = {0.0f, 0.0f, 1.0f}, MatType type = DIFFUSE);
    void set_ambient(float* rgb);
    void set_diffuse(float* rgb);
    void set_specular(float* rgb);
    void set_polish(float p);
    void set_refraction(float r);
    void set_shine(float s);
    RGB lambertian(float u, float v, Point& p_int);
};

#endif
