#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include <limits>
#include <GL/freeglut.h>

#include "../util/RGB.hpp"
#include "../geometry/Object.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Intersection.hpp"

using std::vector;
using std::numeric_limits;

class Scene
{
  private:
    int resolution;
    float width;
    float distance;
    bool shadow;
    Camera* camera;
    vector<Light*> lights;
    void set_pixel(GLubyte* pixels, int x, int y, RGB rgb);
  public:
    vector<Object*> objects;
    Scene(int resolution, Camera* camera, vector<Object*> objects, vector<Light*> lights, bool shadow = true, float w = 6, float d = 3);
    void setShadow(bool value);
    bool trace(Ray& ray, Intersection& intersection);
    void castRay(int x, int y, Intersection& intersection, float offset=1);
    void print(GLubyte* pixels, int samples=1);
};

#endif
