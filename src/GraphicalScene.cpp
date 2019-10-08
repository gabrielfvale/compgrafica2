#include <iostream>
#include <cmath>
#include <vector>
#include <limits>

#include <GL/freeglut.h>

#include "Point.hpp"
#include "Vector3.hpp"
#include "Quaternion.hpp"
#include "Matrix4.hpp"

#include "Ray.hpp"
#include "Plane.hpp"
#include "Cylinder.hpp"
#include "Cone.hpp"
#include "AABB.hpp"

#include "Camera.hpp"
#include "Light.hpp"
#include "PointLight.hpp"
#include "RemoteLight.hpp"

#include "BMP.hpp"

using namespace std;

int panel_holes = 500;
GLubyte* PixelBuffer = new GLubyte[panel_holes * panel_holes * 3];

void render(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawPixels(panel_holes, panel_holes, GL_RGB, GL_UNSIGNED_BYTE, PixelBuffer);
  glutSwapBuffers(); 
}

void set_pixel(int x, int y, int r, int g, int b, GLubyte* pixels, int width, int height)
{
  int position = (x + y * width) * 3;
  pixels[position] = r;
  pixels[position + 1] = g;
  pixels[position + 2] = b;
}

int main(int argc, char *argv[])
{
  // Definições observador e placa
  float panel_l = 6;
  float panel_d = 3;

  Material dark_brown = Material(RGB(0.27, 0.13, 0), RGB(0.27, 0.13, 0), RGB());
  Material tree_green = Material(RGB(0.33, 0.49, 0.18), RGB(0.2, 0.2, 0.2), RGB());
  Material purple = Material(RGB(0.33, 0.18, 0.49), RGB(0.33, 0.18, 0.49), RGB());

  Point observer = Point(10, 4.5, 20);
  Point lookat = Point(10, 4.5, 5);
  Vector3 viewup = Vector3(10, 5.5, 20);

  Camera camera = Camera(observer, lookat, viewup);
  Matrix4 cameraToWorld = camera.camera_to_world();

  // definições de objetos
  Vector3 g_axis = Vector3(0, 1, 0);
  Point cylinder_center = Point(7, 0, 9);
  Point cylinder2_center = Point(13, 0, 9);
  Point bcube_center = Point(10, 0, 5);

  float hole_width = panel_l/panel_holes;
  float cylinder_radius = 0.5;
  float cylinder_height = 2;
  float cone_radius = 2;
  float cone_height = 8;
  float cube_edge = 3;

  // gera os objetos
  Cylinder cylinder = Cylinder(cylinder_center, g_axis, cylinder_height, cylinder_radius, &dark_brown);
  Cylinder cylinder2 = Cylinder(cylinder2_center, g_axis, cylinder_height, cylinder_radius, &dark_brown);
  Cone cone = Cone(
    Point(
      cylinder_center.get_x() + cylinder_height * g_axis.get_x(),
      cylinder_center.get_y() + cylinder_height * g_axis.get_y(),
      cylinder_center.get_z() + cylinder_height * g_axis.get_z()
    ), g_axis, cone_height, cone_radius, &tree_green);
  Cone cone2 = Cone(
    Point(
      cylinder2_center.get_x() + cylinder_height * g_axis.get_x(),
      cylinder2_center.get_y() + cylinder_height * g_axis.get_y(),
      cylinder2_center.get_z() + cylinder_height * g_axis.get_z()
    ), g_axis, cone_height, cone_radius, &tree_green);
  AABB b_cube = AABB(bcube_center, g_axis, cube_edge, &purple);
  float bcx, bcy, bcz;
  bcube_center.get_coordinates(&bcx, &bcy, &bcz);
  float dx, dy, dz;
  g_axis.get_coordinates(&dx, &dy, &dz);
  // cria os demais cubos calculando o centro a partir do cubo base
  //AABB m_cube = AABB(Point(bcx + (dx*cube_edge), bcy + (dy*cube_edge), bcz + (dz*cube_edge)), g_axis, cube_edge, &purple);
  //AABB t_cube = AABB(Point(bcx + (2*dx*cube_edge), bcy + (2*dy*cube_edge), bcz + (2*dz*cube_edge)), g_axis, cube_edge, &purple);

  // cria as luzes
  Light ambient_light = Light(0.5, 0.5, 0.5);
  vector<RemoteLight> rl = vector<RemoteLight>();
  vector<PointLight> pl = {PointLight(RGB(0.3, 0.3, 0.3), Point(15, 4.5, 15))};

  vector<Object*> objects = {
    &cylinder,
    &cylinder2,
    &cone,
    &cone2,
    &b_cube
  };

  // inicia GLUT
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowPosition(100, 100);
  glutInitWindowSize(panel_holes, panel_holes);
  glutCreateWindow("Trabalho CG");

  // projeta cada um dos raios
  for(int i=0; i<panel_holes; i++)
  {
    for (int j=0; j<panel_holes; j++)
    {
      // gera o ponto da matriz em coordenadas de camera
      Point hole_point = Point(-panel_l/2 + hole_width/2 + j*hole_width, panel_l/2 - hole_width/2 - i*hole_width, -panel_d);
      // converte o ponto para coordenadas de mundo
      hole_point = cameraToWorld * hole_point;
      Vector3 ray_direction = Vector3(&observer, &hole_point);
      Ray ray = Ray(observer, ray_direction);

      float t_int;
      float t_min = numeric_limits<float>::infinity();

      Object* object_hit = NULL;

      for(unsigned i=0; i<objects.size(); i++)
      {
        if((*objects[i]).visible() && (*objects[i]).intersects(ray, t_int))
        {
          if(object_hit == NULL || (t_int < t_min))
          {
            t_min = t_int;
            object_hit = objects[i];
          }
        }
      }

      if(object_hit != NULL && (*object_hit).visible())
      {
        Point intersection = ray.calc_point(t_min);
        RGB color = (*object_hit).calculate_color(hole_point, intersection, ambient_light, rl, pl);
        set_pixel(j, panel_holes-1-i, floor(color.r*255), floor(color.g*255), floor(color.b*255), PixelBuffer, panel_holes, panel_holes);
      } else
      {
        set_pixel(j, panel_holes-1-i, 153, 204, 255, PixelBuffer, panel_holes, panel_holes);
      }
    }
  }
  glutDisplayFunc(render);
  glutMainLoop();
  return 0;
}