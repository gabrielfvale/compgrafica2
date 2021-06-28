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
#include "PhotonMap.hpp"

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
    int n_photons = 1000000;
    PhotonMap pmap_global = PhotonMap(n_photons);
  public:
    vector<Object*> objects;
    Scene(int resolution, Camera* camera, vector<Object*> objects, vector<Light*> lights, bool shadow = true, float w = 6, float d = 3);
    void setShadow(bool value);
    bool intersects(Ray& ray, Intersection& intersection);
    RGB trace(Ray& ray, Intersection& intersection, int depth=0);
    RGB castRay(int x, int y, Intersection& intersection, float offset=1);
    void print(GLubyte* pixels, int samples=1);
    void photon_tracing(PhotonMap& pmap, Ray& ray, RGB& color, int depth=0);
};

#endif
