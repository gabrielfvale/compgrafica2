#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "RGB.hpp"
#include "../math/Point.hpp"
#include "../math/Vector3.hpp"

enum LightType { AMBIENT, REMOTE, POINT, SPOT };

class Light
{
  private:
    RGB intensity_;
    Vector3 position_;
    LightType l_type;
    bool active_;
    float spot_angle;
    float spot_falloff;
    float spot_focus;
    Point spot_pos;
  public:
    Light();
    Light(RGB intensity, Vector3 position, LightType light_type = POINT);
    Light(float* rgb, Vector3 position, LightType light_type = POINT);
    Light(float* rgb, Point position, Vector3 lookat, float angle, float falloff_angle = 45, float focus = 1);
    void set_intensity(RGB new_intensity);
    void set_intensity(float* rgb);
    void set_position(Vector3 position);
    bool* active();
    RGB* get_intensity(Point& point);
    Vector3* get_position();
    LightType type();
};

#endif
