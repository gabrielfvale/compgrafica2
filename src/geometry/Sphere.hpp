#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "Solid.hpp"

class Sphere : public Solid
{
  private:
    Point p0_; // known point of the sphere
    Point center_; // center of the sphere
    float radius_;
    Point scale_aux;
  public:
    Sphere();
    Sphere(Point center, float radius, Material* material);
    Point get_center();
    float get_radius();
    void set_params(Point* center, float* radius);
    Vector3 surface_normal(Point& p_int) override;
    bool intersects(Ray& ray, float& t_int) override;
    void transform(Matrix4 t_matrix, TransformType t_type) override;
    void uv(Point& p_int, float& u, float&v) override;
    virtual Sphere* clone() const {return new Sphere(*this);}
};

#endif
