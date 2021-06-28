#include "Scene.hpp"
#include "../util/params.h"

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
  // int n_photons = 1000000;
  this->resolution = resolution;
  this->camera = camera;
  this->objects = objects;
  this->lights = lights;
  this->width = w;
  this->distance = d;
  this->shadow = shadow;
  this->n_photons = 1000000;
  // this->pmap_global = PhotonMap(n_photons);
}

void Scene::setShadow(bool value)
{
  this->shadow = value;
}

bool Scene::intersects(Ray& ray, Intersection& intersection)
{
  bool hasIntersection = false;
  float t_min = numeric_limits<float>::infinity();
  for(unsigned i = 0; i < objects.size(); i++)
  {
    if(objects[i]->trace(ray, intersection) && intersection.tint < t_min)
    {
      t_min = intersection.tint;
      intersection.object_hit = objects[i];
      intersection.index = i;
      intersection.p_int = ray.calc_point(intersection.tint);
      hasIntersection = true;
    }
  }
  return hasIntersection;
}

RGB Scene::trace(Ray& ray, Intersection& intersection, int depth)
{
  Point observer = *(camera->get_eye());

  RGB ret_color;
  Intersection shadow_intersect;

  if(intersects(ray, intersection))
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

		float color[3];
		float pos[3];
		float normal[3];
    p_int.to_float(pos);
    surfc_normal.to_float(normal);
		pmap_global.irradiance_estimate(color, pos, normal, 10, 1000);
		ret_color = RGB(color[0], color[1], color[2]);

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
          if(objects[k]->trace(shadowray, shadow_intersect))
          {
            visible = 0;
            if(lights[i]->type() == POINT || lights[i]->type() == SPOT)
            {
              Point shadow_point = shadowray.calc_point(shadow_intersect.tint);
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
          Ray refractionRay = ray.calc_refraction(intersection.p_int, n, solid_mat->refraction);
          Intersection refraction;
          refraction_color = trace(refractionRay, refraction, depth + 1);
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
  chrono::time_point<chrono::system_clock> render_start, render_end;
  render_start = chrono::system_clock::now();

  std::cout << "[SCENE] Started photon mapping" << std::endl;
  std::cout << "[PHOTON] Mapping " << n_photons << " photons..." << std::endl;
  /* Photon Mapping */
  int k = 0;
  for(unsigned i = 0; i < lights.size(); i++)
  {
    if(! *(lights[i]->active()) || lights[i]->type() == AMBIENT) continue;
    while (pmap_global.stored_photons < pmap_global.max_photons)
    {
      // fprintf(stderr,"\r[PHOTON] Mapping %5.2f%%",100.* pmap_global.stored_photons/pmap_global.max_photons);
      Vector3 lv = lights[i]->get_position();
      Point pos = lv.to_point();
      Vector3 dir = Vector3::sample_hemisphere_cos();
      dir.normalize();
      Ray randomRay = Ray(pos, dir);
      Point _p = Point();
      RGB color = RGB(1.0, 1.0, 1.0);
      photon_tracing(pmap_global, randomRay, color);
      k++;
    }
  }

  pmap_global.balance();
	pmap_global.scale_photon_power(1.f/1000);	

  std::cout << "\n[SCENE] Started rendering" << std::endl;

  std::random_device rd;  //Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(0.0, 1.0);

  RGB out_color;

  #pragma omp parallel for schedule(dynamic, 1) private(out_color)
  for(int y = 0; y < resolution; y++)
  {
    fprintf(stderr,"\r[SCENE] Rendering %5.2f%%",100.*y/(resolution-1));

    for (int x = 0; x < resolution; x++)
    {
      Intersection intersection;
      vector<Intersection> intersections;

      RGB color = castRay(x, y, intersection, 1.0f);
      float colors[3] = {color.r, color.g, color.b};

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
      out_color = RGB(colors[0]/total, colors[1]/total, colors[2]/total);
      set_pixel(pixels, x, y, out_color);
    }
  }

  render_end = chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = render_end - render_start;

  std::cout << "\n[SCENE] Rendered in " << setprecision(2);
  std::cout << elapsed_seconds.count() << "s\n"; 
}

void Scene::photon_tracing(PhotonMap& pmap, Ray& ray, RGB& color, int depth)
{
  Intersection intersection;
  if (!intersects(ray, intersection))
    return;
  if (depth > MAX_RAY_DEPTH)
    return;

  Point p_int = intersection.p_int;
  Vector3 dir = ray.get_d();
  Vector3 n = intersection.solid_hit->surface_normal(p_int);
  Vector3 bias = n * 0.01;
  bias = n.dot_product(&dir) < 0 ? bias : bias * -1; // check for outside
  Point pbias = bias.to_point();

  Point off_point = p_int + pbias;

  Material* solid_mat = intersection.solid_hit->get_material();

  // float reflectivity = solid_mat->polish;
  RGB dif = solid_mat->diffuse;
  RGB spec = solid_mat->specular;
  
  float pd, ps, pa;

  pd = std::max(dif.r, std::max(dif.g, dif.b));
  ps = std::max(spec.r, std::max(spec.g, spec.b));
  pa = 1 - pd - ps;

  float pd_avg = (dif.r + dif.g + dif.b) / 3;
  float ps_avg = (spec.r + spec.g + spec.b) / 3;

  MatType mat_type = solid_mat->type;
  switch (mat_type)
  {
  case DIFFUSE: // store photon
    if (rroulette() > pa)
    {
      float power_a[] = {dif.r*color.r*(1.0/(1-pd_avg)), dif.g*color.g*(1.0/(1-pd_avg)), dif.b*color.b*(1.0/(1-pd_avg))};
      // float power_a[] = {color.r, color.g, color.b};
      float pos_a[3];
      float dir_a[3];
      p_int.to_float(pos_a);
      dir.to_float(dir_a);
      pmap.store(power_a, pos_a, dir_a);
      return;
    } else
    {
      Vector3 u, v;
      n.construct_basis(u, v);
      Vector3 w = Vector3::sample_hemisphere_cos();
      Vector3 newDir = Vector3(u.get_x() * w.get_x(), u.get_y() * w.get_y(), u.get_z() * w.get_z());
      Ray newRay = Ray(off_point, newDir);
      photon_tracing(pmap, newRay, color, depth + 1);
    }
    break;
  case REFLECTIVE: // trace another ray
  {
    Ray reflectionRay = ray.calc_reflection(off_point, n);
    photon_tracing(pmap, reflectionRay, color, depth + 1);
    break;
  }
  case REFLECTIVE_AND_REFRACTIVE: // dieletric reflection
  {
    float ior = solid_mat->refraction;
    float d_dot_n = dir.dot_product(&n);
    float root = 1.0 - (1.0 - (d_dot_n*d_dot_n) / (ior*ior));
    // calculate reflection and refraction rays
    Ray reflectionRay = ray.calc_reflection(off_point, n);
    Ray refractionRay = ray.calc_refraction(p_int, n, solid_mat->refraction);
    // check for total internal reflection
    if (root < 0.0)
    {
      photon_tracing(pmap, reflectionRay, color, depth + 1);
    }
    rroulette() < ps ? photon_tracing(pmap, reflectionRay, color, depth + 1) : photon_tracing(pmap, refractionRay, color, depth + 1);
    break;
  }
  default:
    return;
    break;
  }
}
