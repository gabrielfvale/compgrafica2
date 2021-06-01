#include "Scene.hpp"

#include <random>
#include <thread>
#include <functional>
#include <chrono>

#include <cmath>
#include <ctime>
#include <iostream>
#include <iomanip>

#define MAX_RAY_DEPTH 4

using namespace std;

Scene::Scene(int resolution, Camera* camera, vector<Object*> objects, vector<Light*> lights, bool shadow, float w, float d)
{
  this->resolution = resolution;
  this->camera = camera;
  this->objects = objects;
  this->lights = lights;
  this->width = w;
  this->distance = d;
  this->shadow = shadow;
}

void Scene::setShadow(bool value)
{
  this->shadow = value;
}

RGB Scene::trace(Ray& ray, Intersection& intersection, int depth, int skip)
{
  Point observer = *(camera->get_eye());
  float t_min = numeric_limits<float>::infinity();

  RGB ret_color;
  Intersection obj_intersect;
  int object_index = -1;

  for(unsigned i = 0; i < objects.size(); i++)
  {
    if((int)i != skip && objects[i]->trace(ray, obj_intersect) && obj_intersect.tint < t_min)
    {
      t_min = obj_intersect.tint;
      intersection = obj_intersect;
      intersection.object_hit = objects[i];
      object_index = i;
      intersection.index = object_index;
      intersection.p_int = ray.calc_point(intersection.tint);
    }
  }

  if(object_index != -1)
  {
    float episilon = 0.01;
    intersection.color = RGB();
    Point p_int = intersection.p_int;
    Vector3 surfc_normal = intersection.solid_hit->surface_normal(p_int);
    surfc_normal = surfc_normal * episilon;

    // shifts intersection so there is no shadow acne
    p_int.set_coordinates(
      p_int.get_x() + surfc_normal.get_x(),
      p_int.get_y() + surfc_normal.get_y(),
      p_int.get_z() + surfc_normal.get_z()
    );

    // calculate color for each light
    for(unsigned i = 0; i < lights.size(); i++)
    {
      if(! *(lights[i]->active()) ) continue;
      if(lights[i]->type() == AMBIENT)
      {
        ret_color += intersection.solid_hit->calculate_color(lights[i], observer, p_int);
        continue;
      }

      Vector3 light_dir = intersection.solid_hit->light_direction(lights[i], p_int);
      Ray shadowray = Ray(p_int, light_dir);

      Vector3 lvp = lights[i]->get_position();
      Point light_point = lights[i]->type() == SPOT ? lights[i]->get_spotpos() : Point(lvp.get_x(), lvp.get_y(), lvp.get_z());
      int visible = 1;
      /* Start of shadow code */
      if(shadow && depth == 0)
      {
        unsigned k = 0;
        while(visible && k < objects.size())
        {
          if(objects[k]->trace(shadowray, obj_intersect))
          {
            visible = 0;
            if(lights[i]->type() == POINT || lights[i]->type() == SPOT)
            {
              Point shadow_point = shadowray.calc_point(obj_intersect.tint);
              if(p_int.distance_from(&shadow_point) > p_int.distance_from(&light_point))
                visible = 1; // light is closer to the object
            }
          }
          k++;
        }
      }
      /* End of shadow code */
      ret_color += intersection.solid_hit->calculate_color(lights[i], observer, p_int) * visible;
    }

    Material* solid_mat = intersection.solid_hit->get_material();
    MatType mat_type = solid_mat->type;
    switch (mat_type)
    {
    case DIFFUSE:
      break;
    case REFLECTIVE:
      if(depth < MAX_RAY_DEPTH) {
        Vector3 n = intersection.solid_hit->surface_normal(p_int);
        Ray reflection_ray = ray.calc_reflection(p_int, n);
        Intersection reflection_int;
        RGB reflection_color = trace(reflection_ray, reflection_int, depth + 1);
        ret_color += reflection_color * solid_mat->polish;
      }
      break;
    case REFLECTIVE_AND_REFRACTIVE:
      if(depth < MAX_RAY_DEPTH) {
        float kr;
        RGB refraction_color;
        Vector3 I = ray.get_d();
        Vector3 n = intersection.solid_hit->surface_normal(p_int);
        ray.fresnel(I, n, solid_mat->refraction, kr);

        // calculate refraction if not total internal reflection
        if (kr < 1) {
          bool has_refr;
          Ray refractionRay = ray.calc_refraction(p_int, n, solid_mat->refraction, has_refr);
          Intersection refraction;
          refraction_color = trace(refractionRay, refraction, depth + 1, intersection.index);
        }
        // calculate reflection
        Ray reflection_ray = ray.calc_reflection(p_int, n);
        Intersection reflection_int;
        RGB reflection_color = trace(reflection_ray, reflection_int, depth + 1);
        // mix
        float kf = (1 - kr);
        ret_color += reflection_color * kr * solid_mat->polish;
        ret_color += refraction_color * kf * solid_mat->transmittance;
      }
      break;
    default:
      break;
    }
  }
  return ret_color;
}

RGB Scene::castRay(int x, int y, Intersection& intersection, float offset)
{
  float pixel_width = width/resolution;
  Point observer = *(camera->get_eye());

  float x_pos = -width/2 + ((pixel_width/2) * offset) + x*pixel_width;
  float y_pos = width/2 - ((pixel_width/2) * offset) - y*pixel_width;

  Point hole_point = Point(x_pos, y_pos, -distance);
  hole_point = camera->camera_to_world() * hole_point;
  Vector3 ray_direction = Vector3(&observer, &hole_point);
  Ray ray = Ray(hole_point, ray_direction);

  return trace(ray, intersection);
}

void Scene::set_pixel(GLubyte* pixels, int x, int y, RGB rgb)
{
  y = resolution-1-y;
  int position = (x + y * resolution) * 3;
  pixels[position] = std::floor(rgb.r * 255);
  pixels[position + 1] = std::floor(rgb.g * 255);
  pixels[position + 2] = std::floor(rgb.b * 255);
}

void Scene::print(GLubyte* pixels, int samples)
{
  cout << "[SCENE] Started rendering" << endl;
  chrono::time_point<chrono::system_clock> render_start, render_end;

  const size_t nthreads = thread::hardware_concurrency();
  vector<thread> threads(nthreads);

  render_start = chrono::system_clock::now();

  for(size_t t = 0; t < nthreads; t++)
  {
    threads[t] = thread(bind(
    [&](const int start_i, const int end_i, const int t)
    {
    for(int y = start_i; y < end_i; y++)
    {
      for (int x = 0; x < resolution; x++)
      {
        Intersection intersection;
        vector<Intersection> intersections;

        RGB color = castRay(x, y, intersection, 1.0f);
        float colors[3] = {color.r, color.g, color.b};

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for(int k = 0; k < samples; k++) {
          float offset = dis(gen);
          int signal = k % 2 == 0 ? 1  : -1;
          Intersection newInt;
          RGB newColor = castRay(x, y, newInt, offset * signal);
          intersections.push_back(newInt);
          colors[0] += newColor.r;
          colors[1] += newColor.g;
          colors[2] += newColor.b;
        }
        int total = samples + 1;
        RGB final_color = RGB(colors[0]/total, colors[1]/total, colors[2]/total);
        set_pixel(pixels, x, y, final_color);
      }
    }
    },t*resolution/nthreads,(t+1)==nthreads?resolution:(t+1)*resolution/nthreads,t));
  }

  for(size_t i = 0; i < nthreads; i++)
    threads[i].join();

  render_end = chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = render_end - render_start;

  cout << "[SCENE] Rendered in " << setprecision(2);
  cout << elapsed_seconds.count() << "s\n"; 
}
