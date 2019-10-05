#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <vector>
#include "Vector3.hpp"
#include "Matrix4.hpp"

class Camera
{
  private:
    Point eye_;
    Point lookat_;
    Vector3 up_;
    Vector3 cx_, cy_, cz_;
  public:
    Camera(Point eye, Point lookat, Vector3 up);
    Matrix4 camera_to_world();
    Matrix4 world_to_camera();
    Vector3* x_axis();
    Vector3* y_axis();
    Vector3* z_axis();
};

#endif
