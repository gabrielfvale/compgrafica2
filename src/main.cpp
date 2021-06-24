#include <iostream>
#include <cstdio>
#include <cmath>
#include <vector>

#include <GL/freeglut.h>
#include "./imgui/imgui.h"
#include "./imgui/imgui_impl_glut.h"
#include "./imgui/imgui_impl_opengl2.h"

#include "./geometry/Plane.hpp"
#include "./geometry/Triangle.hpp"
#include "./geometry/Sphere.hpp"
#include "./geometry/AABB.hpp"
#include "./geometry/OBB.hpp"
#include "./geometry/Cone.hpp"
#include "./geometry/Cylinder.hpp"
#include "./geometry/Solid.hpp"
#include "./geometry/Object.hpp"

#include "./scene/Scene.hpp"

using namespace std;

int resolution = 500;
int samples = 0;
float upscaling = 1.0f;
bool has_shadow = true;
GLubyte* PixelBuffer;

/* Origin */
static float observerf3[3] = { 0.0f, 75.0f, 150.0f };
static float lookatf3[3] = { 0.0f, 75.0f, 0.0f };
static float viewupf3[3] = { 0.0f, 76.0f, 150.0f };

/* Top lights
static float observerf3[3] = { 0.0f, 280.0f, 20.0f };
static float lookatf3[3] = { 0.0f, 0.0f, 0.0f };
static float viewupf3[3] = { 0.0f, 280.0f, 20.0f };
*/

Camera* camera = new Camera(observerf3, lookatf3, viewupf3);

/* Lights */
  /* Point lights */
float pl_intensity[4][3] = {
  {0.3f, 0.3f, 0.3f},
  {0.1f, 0.1f, 0.1f},
  {0.1f, 0.1f, 0.1f},
  {0.1f, 0.1f, 0.1f}
};
float pl_pos[4][3] = {
  {0, 145, 0},
  {-100, 270, 90},
  {100, 270, 260},
  {-100, 270, 260}
};
vector<Light*> point_lights = {
  new Light(pl_intensity[0], Vector3(pl_pos[0]))
};
  // new Light(pl_intensity[1], Vector3(pl_pos[1])),
  // new Light(pl_intensity[2], Vector3(pl_pos[2])),
  // new Light(pl_intensity[3], Vector3(pl_pos[3]))
/* Remote light */
float rl_intensity[3] = {0.3f, 0.3f, 0.3f};
float rl_dir[3] = {1, -1, 1};
Light* remote_light = new Light(rl_intensity, Vector3(rl_dir), REMOTE);
/* Spotlight */
float sp_intensity[3] = {0.3f, 0.3f, 0.3f};
float sp_pos[3] = {0, 145, 0};
float sp_dir[3] = {0, -1, 0};
float sp_angle = M_PI_4/3; float sp_falloff = M_PI_2/3; float sp_focus = 1;
Light* spot_light = new Light(sp_intensity, Point(sp_pos), Vector3(sp_dir), sp_angle, sp_falloff, sp_focus);

/* Ambient light */
Light* ambient_light = new Light(RGB(0.5, 0.5, 0.5), Vector3(), AMBIENT);

vector<Light*> lights = {
  ambient_light,
  remote_light,
  // spot_light
};
bool global_switch = true;
Scene* scene;
vector<Object*> objects;

Material* mat_mirror = new Material(
	RGB(0.0, 0.0, 0.0),
  RGB(0.0, 0.0, 0.0),
  RGB(0.0, 0.0, 0.0),
  {1.0f, 0.0f, 0.0f, 38.4},
  REFLECTIVE
);
Material* mat_glass = new Material(
	RGB(0.0, 0.0, 0.0),
  RGB(0.0, 0.0, 0.0),
  RGB(0.0, 0.0, 0.0),
  {1.0f, 1.53f, 0.9f, 38.4},
  REFLECTIVE_AND_REFRACTIVE
);
Material* mat_white = new Material(
  RGB(0.9, 0.9, 0.9),
  RGB(0.9, 0.9, 0.9),
  RGB()
);
Material* mat_red = new Material(
  RGB(0.7, 0.1, 0.1),
  RGB(0.8, 0.8, 0.8),
  RGB(0.1, 0.1, 0.1)
);
Material* mat_blue = new Material(
  RGB(0.1, 0.1, 0.7),
  RGB(0.8, 0.8, 0.8),
  RGB(0.1, 0.1, 0.1)
);

float obj_ambient[3] = {0.0f, 0.0f, 0.0f};
float obj_diffuse[3] = {0.0f, 0.0f, 0.0f};
float obj_specular[3] = {0.0f, 0.0f, 0.0f};
float obj_properties[4] = {0.0f, 0.0f, 0.0f, 0.0f};
const int MAT_COUNT = 3; 
const char* mat_names[MAT_COUNT] = { "DIFFUSE", "REFLECTIVE", "REFLECTIVE AND"};
static int current_type = DIFFUSE;

float obj_translate[3] = {0.0f, 0.0f, 0.0f};
float obj_rangle = 0.0f;
float obj_raxis[3] = {0.0f, 0.0f, 0.0f};

Solid* picked_solid = NULL;
Object* picked_object = NULL;
const char* object_name = "No object selected";

void redraw()
{
  scene->print(PixelBuffer, samples);
  glutPostRedisplay();
}

void display_gui()
{
  // ImGui::ShowDemoWindow();
  ImGuiStyle& style = ImGui::GetStyle();
  style.FrameRounding = 12.0f;
  //ImGui::StyleColorsLight();

  ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_Float;
  ImGuiWindowFlags window_flags = 0;
  //window_flags |= ImGuiWindowFlags_NoTitleBar;
  //window_flags |= ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoResize;
  //window_flags |= ImGuiWindowFlags_NoCollapse;
  //window_flags |= ImGuiWindowFlags_NoBackground;
  window_flags |= ImGuiWindowFlags_NoSavedSettings;
  //window_flags |= ImGuiWindowFlags_NoDecoration;
  window_flags |= ImGuiWindowFlags_AlwaysAutoResize;

  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
  //ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_Once);
  // Janela de propriedades do cenario
  ImGui::Begin("Scene", NULL, window_flags);
  if(ImGui::BeginTabBar("Main tab bar"))
  {
    /* Projeto */
    if(ImGui::BeginTabItem("Project"))
    {
      if(ImGui::Checkbox("Shadows", &has_shadow))
      {
        scene->setShadow(has_shadow);
        redraw();
      }
      ImGui::SliderInt("Samples", &samples, 0, 16);
      if(ImGui::Button("Update"))
      {
        redraw();
      }
      ImGui::EndTabItem();
    }
    /* Propriedades de camera */
    if(ImGui::BeginTabItem("Camera"))
    {
      ImGui::InputFloat3("observer", observerf3);
      ImGui::InputFloat3("lookat", lookatf3);
      ImGui::InputFloat3("viewup", viewupf3);
      if(ImGui::Button("Update camera"))
      {
        camera->set_eye(observerf3);
        camera->set_lookat(lookatf3);
        camera->set_viewup(viewupf3);
        redraw();
      }
      ImGui::EndTabItem();
    }

    /* Configuração de luzes */
    if(ImGui::BeginTabItem("Lights"))
    {
      if(ImGui::Checkbox("Toggle all", &global_switch))
      {
        for(auto light : lights)
          if(light->type() != AMBIENT)
            *(light->active()) = global_switch;

        redraw();
      }
      /* Point lights 
      if(ImGui::CollapsingHeader("Point lights"))
      {
        for(unsigned i = 0; i < point_lights.size(); i++)
        {
          string uid = "##pl_" + to_string(i+1);
          string i_light = "Point light " + uid;
          if(ImGui::TreeNode(i_light.c_str()))
          {
            string label_enabled = "Enabled" + uid;
            string label_position = "Position" + uid;
            string label_intensity = "Intensity" + uid;
            if(ImGui::Checkbox(label_enabled.c_str(), point_lights[i]->active()))
              redraw();
            ImGui::InputFloat3(label_position.c_str(), pl_pos[i]);
            if(ImGui::IsItemDeactivatedAfterEdit())
            {
              point_lights[i]->set_position(pl_pos[i]);
              redraw();
            }
            ImGui::ColorEdit3(label_intensity.c_str(), pl_intensity[i]);
            if(ImGui::IsItemDeactivatedAfterEdit())
            {
              point_lights[i]->set_intensity(pl_intensity[i]);
              redraw();
            }
            ImGui::TreePop();
          }
        }
      }
      */
      /* Remote light */
      if(ImGui::CollapsingHeader("Remote light"))
      {
        if(ImGui::Checkbox("Enabled##rl_toggle", remote_light->active()))
          redraw();
        ImGui::InputFloat3("Direction##rl_dir", rl_dir, 3);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          remote_light->set_position(Vector3(rl_dir));
          redraw();
        }
        ImGui::ColorEdit3("Intensity##rl_rgb", rl_intensity);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          remote_light->set_intensity(rl_intensity);
          redraw();
        }
      }
      /* Spotlight */
      if(ImGui::CollapsingHeader("Spot light"))
      {
        if(ImGui::Checkbox("Enabled##sp_toggle", spot_light->active()))
          redraw();
        ImGui::SliderAngle("Angle", &sp_angle, 0.0f);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          spot_light->set_spot(sp_pos, sp_angle, sp_falloff, sp_focus);
          redraw();
        }
        ImGui::SliderAngle("Falloff", &sp_falloff, (180/M_PI) * sp_angle);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          spot_light->set_spot(sp_pos, sp_angle, sp_falloff, sp_focus);
          redraw();
        }
        ImGui::DragFloat("Focus", &sp_focus, 1, 1, 10);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          spot_light->set_spot(sp_pos, sp_angle, sp_falloff, sp_focus);
          redraw();
        }
        ImGui::InputFloat3("Direction##sp_dir", sp_dir, 3);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          spot_light->set_position(Vector3(sp_dir));
          redraw();
        }
        ImGui::InputFloat3("Position", sp_pos, 3);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          spot_light->set_spot(sp_pos, sp_angle, sp_falloff, sp_focus);
          redraw();
        }
        ImGui::ColorEdit3("Intensity##sp_rgb ", sp_intensity);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          spot_light->set_intensity(sp_intensity);
          redraw();
        }
      }
      ImGui::EndTabItem();
    }
    /* Object picking */
    if(ImGui::BeginTabItem("Object Picking"))
    {
      ImGui::Text(object_name);
      if(picked_solid != NULL)
      {
        if(ImGui::Checkbox("Visible", picked_object->visible_ptr()))
          redraw();
        ImGui::ColorEdit3("Ambient", obj_ambient, picker_flags);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          Material* material = picked_solid->get_material();
          material->set_ambient(obj_ambient);
          redraw();
        }
        ImGui::ColorEdit3("Diffuse", obj_diffuse, picker_flags);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          Material* material = picked_solid->get_material();
          material->set_diffuse(obj_diffuse);
          redraw();
        }
        ImGui::ColorEdit3("Specular", obj_specular, picker_flags);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          Material* material = picked_solid->get_material();
          material->set_specular(obj_specular);
          redraw();
        }
        ImGui::Text("Reflection / Refraction / Transmittance / Specular");
        ImGui::InputFloat4("Properties", obj_properties);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          Material* material = picked_solid->get_material();
          material->set_properties(obj_properties);
          redraw();
        }
        const char* cur_mat_name = (current_type >= 0 && current_type < MAT_COUNT) ? mat_names[current_type] : "Unknown";
        ImGui::SliderInt("slider enum", &current_type, 0, MAT_COUNT - 1, cur_mat_name);
        if(ImGui::IsItemDeactivatedAfterEdit())
        {
          Material* material = picked_solid->get_material();
          material->type = (MatType)current_type;
          redraw();
        }
      }
      ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Object selection"))
    {
      ImGui::BeginChild("selection", ImVec2(0, 100));
      for (unsigned i = 0; i < objects.size(); i++)
        ImGui::Selectable(objects[i]->name, objects[i]->visible_ptr());
      ImGui::EndChild();
      if(ImGui::Button("Update objects"))
        redraw();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  ImGui::End();

  /* Object picking (Ray cast) */
  ImGuiIO& io = ImGui::GetIO();
  if(ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
  {
    // Manda um raio em MousePos.x, MousePos.y
    Intersection intersection;
    scene->castRay(io.MousePos.x/upscaling, io.MousePos.y/upscaling, intersection);
    if(intersection.index != -1)
    {
      cout <<  *(intersection.object_hit) << endl;
      Material* int_material = intersection.solid_hit->get_material();

      obj_ambient[0] = int_material->ambient.r;
      obj_ambient[1] = int_material->ambient.g;
      obj_ambient[2] = int_material->ambient.b;

      obj_diffuse[0] = int_material->diffuse.r;
      obj_diffuse[1] = int_material->diffuse.g;
      obj_diffuse[2] = int_material->diffuse.b;

      obj_specular[0] = int_material->specular.r;
      obj_specular[1] = int_material->specular.g;
      obj_specular[2] = int_material->specular.b;

      int_material->get_properties(obj_properties);
      current_type = int_material->type;

      object_name = intersection.object_hit->name;
      picked_solid = intersection.solid_hit;
      picked_object = intersection.object_hit;
    }
  }
}

void render()
{
  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplGLUT_NewFrame();
  display_gui();
  ImGui::Render();
  //ImGuiIO& io = ImGui::GetIO();

  glClear(GL_COLOR_BUFFER_BIT);
  glDrawPixels(resolution, resolution, GL_RGB, GL_UNSIGNED_BYTE, PixelBuffer);
  glPixelZoom(upscaling, upscaling);

  ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

  glutSwapBuffers();
  glutPostRedisplay();
}

int main(int argc, char *argv[])
{
  if(argc >= 2)
    resolution = atoi(argv[1]);
  if(argc >= 3)
    samples = atoi(argv[2]);
  if(argc == 4)
    upscaling = atof(argv[3]);
  PixelBuffer = new GLubyte[resolution * resolution * 3];

  Point origin;

  for(auto point_light : point_lights)
    lights.push_back(point_light);

  Object* mirror_obj = new Object(
    "Mirror",
    OBB(Point(-30, -30, -30), Point(30, 30, 30)),
    vector<Solid*>{
      new Sphere(
      Point(0, 0, 0),
      30,
      mat_mirror
    )}
  );
  Object* glass_obj = new Object(
    "Glass",
    OBB(Point(-30, -30, -30), Point(30, 30, 30)),
    vector<Solid*>{
      new Sphere(
      Point(0, 0, 0),
      30,
      mat_glass
    )}
  );

  Object* cornell_box = new Object(
    "Box",
    OBB(Point(-92, -2, -92), Point(92, 152, 200)),
    vector<Solid*>{
      // new Plane(Point(0, 0, 0), Vector3(0, 1, 0), mat_white)
      new AABB(Point(-90, -2, -75), Point(90, 0, 150), mat_white), // floor
      new AABB(Point(-90, 150, -75), Point(90, 152, 150), mat_white), // ceiling
      new AABB(Point(-90, 0, -75), Point(90, 150, -77), mat_white), // back
      new AABB(Point(-92, 0, -75), Point(-90, 150, 150), mat_red), // right
      new AABB(Point(90, 0, -75), Point(92, 150, 150), mat_blue), // left
    }
  );

  mirror_obj->translate(Vector3(-40, 30, -20));
  glass_obj->translate(Vector3(40, 30, 0));

  objects.push_back(cornell_box);
  objects.push_back(mirror_obj);
  objects.push_back(glass_obj);

  scene = new Scene(resolution, camera, objects, lights);
  scene->print(PixelBuffer, samples);

  // inicia GLUT
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH)-resolution*upscaling)/2, (glutGet(GLUT_SCREEN_HEIGHT)-resolution*upscaling)/2);
  glutInitWindowSize(resolution*upscaling, resolution*upscaling);
  glutCreateWindow("Trabalho CG");

  glutDisplayFunc(render);

  // Inicia contexto imgui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();

  // Seta bindings com GLUT
  ImGui_ImplGLUT_Init();
  ImGui_ImplGLUT_InstallFuncs();
  ImGui_ImplOpenGL2_Init();

  glutMainLoop();

  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGLUT_Shutdown();
  ImGui::DestroyContext();

  return 0;
}
