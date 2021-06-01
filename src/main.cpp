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
static float observerf3[3] = { -70.0f, 120.0f, 20.0f };
static float lookatf3[3] = { 0.0f, 100.0f, 0.0f };
static float viewupf3[3] = { -70.0f, 121.0f, 20.0f };

/* Top 
static float observerf3[3] = { 0.0f, 280.0f, 20.0f };
static float lookatf3[3] = { 0.0f, 0.0f, 0.0f };
static float viewupf3[3] = { 0.0f, 280.0f, 20.0f };
*/

Camera* camera = new Camera(observerf3, lookatf3, viewupf3);

/* Lights */
  /* Point lights */
float pl_intensity[4][3] = {
  {0.1f, 0.1f, 0.1f},
  {0.1f, 0.1f, 0.1f},
  {0.1f, 0.1f, 0.1f},
  {0.1f, 0.1f, 0.1f}
};
float pl_pos[4][3] = {
  {100, 270, 90},
  {-100, 270, 90},
  {100, 270, 260},
  {-100, 270, 260}
};
vector<Light*> point_lights = {
  new Light(pl_intensity[0], Vector3(pl_pos[0])),
  new Light(pl_intensity[1], Vector3(pl_pos[1])),
  new Light(pl_intensity[2], Vector3(pl_pos[2])),
  new Light(pl_intensity[3], Vector3(pl_pos[3]))
};
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
  spot_light
};
bool global_switch = true;
Scene* scene;
vector<Object*> objects;

/* Materials */
Material* mat_silver = new Material(
	RGB(0.23125, 0.23125, 0.23125),
  RGB(0.2775, 0.2775, 0.2775),
  RGB(0.773911, 0.773911, 0.773911),
  {0.05f, 0.0f, 0.f, 89.6},
  REFLECTIVE
);
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
Material* mat_ruby = new Material(
  RGB(0.1745, 0.01175, 0.01175),
  RGB(0.61424, 0.04136, 0.04136),
  RGB(0.727811, 0.626959, 0.626959),
  {1.0f, 1.53f, 0.9f, 76.8},
  REFLECTIVE_AND_REFRACTIVE
);
Material* mat_white_concrete = new Material(
  RGB(0.847058, 0.819607, 0.756862),
  RGB(0.854901, 0.843137, 0.815686),
  RGB()
);
Material* mat_beige_paint = new Material(
  RGB(0.819607, 0.776470, 0.698039),
  RGB(0.882352, 0.839215, 0.760784),
  RGB(),
  {0.1f, 0.0f, 0.0f, 0.0f},
  REFLECTIVE
);
Material* mat_terrazo = new Material(
  RGB(0.490196, 0.454901, 0.435294),
  RGB(0.57254, 0.53725, 0.51764),
  RGB(0.3, 0.3, 0.3),
  {0.025f, 0.0f, 0.0f, 38.0},
  REFLECTIVE
);
Material* mat_darkwood = new Material(
  RGB(0.149019, 0.090196, 0.062745),
  RGB(0.01, 0.01, 0.01),
  RGB(0.1, 0.1, 0.1),
  {0.0f, 0.0f, 0.0f, 38.0}
);
Material* mat_old_plastic = new Material(
  RGB(0.772549, 0.721568, 0.549019),
  RGB(0.949019, 0.898039, 0.760784),
  RGB(0.5, 0.5, 0.5),
  {0.1f, 0.0f, 0.0f, 32},
  REFLECTIVE
);
Material* mat_black_plastic = new Material(
  RGB(0, 0, 0),
  RGB(0.01, 0.01, 0.01),
  RGB(0.5, 0.5, 0.5),
  {0.0f, 0.0f, 0.0f, 32}
);
Material* mat_white_plastic = new Material(
  RGB(0.933333, 0.925490, 0.878431),
  RGB(0.976470, 0.968627, 0.921568),
  RGB(0.5, 0.5, 0.5),
  {0.1f, 0.0f, 0.0f, 32}
);
Material* mat_transp_plastic = new Material(
  RGB(0.0, 0.0, 0.0),
  RGB(0.0, 0.0, 0.0),
  RGB(0.0, 0.0, 0.0),
  {0.2f, 1.60, 0.9f, 32},
  REFLECTIVE_AND_REFRACTIVE
);
Material* mat_white_lamp = new Material(
  RGB(1, 1, 1),
  RGB(1, 1, 1),
  RGB(1, 1, 1)
);
Material* mat_steel = new Material(
	RGB(0.537354, 0.537354, 0.537354),
  RGB(0.772549, 0.772549, 0.772549),
  RGB(0.773911, 0.773911, 0.773911),
  {0.0f, 0.0f, 0.0f, 32}
);
Material* mat_marble = new Material(
  RGB(0.901960, 0.901960, 0.901960),
  RGB(0.949019, 0.949019, 0.949019),
  RGB(0.7, 0.7, 0.7),
  {0.05f, 0.0f, 0.0f, 89.6},
  REFLECTIVE
);
Material* mat_mdf = new Material(
  RGB(0.560784, 0.392156, 0.235294),
  RGB(0.901960, 0.811764, 0.662745),
  RGB(0.3, 0.3, 0.3),
  {0.2f, 0.0f, 0.0f, 38},
  REFLECTIVE
);
Material* mat_blue_chair = new Material(
  RGB(0.1921, 0.2588, 0.4274),
  RGB(0.2509, 0.4, 0.4980),
  RGB()
);
/* Table materials */
Material* mat_table_top = new Material(
  RGB(0.1921, 0.2588, 0.4274),
  RGB(0.2509, 0.4, 0.4980),
  RGB()
);
Material* mat_table_border = new Material(
  RGB(0.847058, 0.819607, 0.756862),
  RGB(0.854901, 0.843137, 0.815686),
  RGB()
);
Material* mat_table_sup = new Material(
	RGB(0.537354, 0.537354, 0.537354),
  RGB(0.772549, 0.772549, 0.772549),
  RGB(0.773911, 0.773911, 0.773911),
  {0.1f, 0.0f, 32},
  REFLECTIVE
);

float obj_ambient[3] = {0.0f, 0.0f, 0.0f};
float obj_diffuse[3] = {0.0f, 0.0f, 0.0f};
float obj_specular[3] = {0.0f, 0.0f, 0.0f};

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
  //ImGui::ShowDemoWindow();
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
    /* Samples */
    if(ImGui::BeginTabItem("Samples"))
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
      /* Point lights */
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

  /*
  for(auto point_light : point_lights)
    lights.push_back(point_light);
  */

  /* Porta retrato */
  Object* pic_frame = new Object(
    "Picture frame",
    OBB(Point(-1, 0, -6), Point(1, 20, 6)),
    vector<Solid*>{
      new AABB(Point(-0.9, 2, -4), Point(0.9, 13, 4), mat_white_plastic), // center
      new AABB(Point(-1, 0, -6), Point(1, 2, 6), mat_mdf), // bottom
      new AABB(Point(-1, 13, -6), Point(1, 15, 6), mat_mdf), // top
      new AABB(Point(-1, 0, -6), Point(1, 15, -4), mat_mdf), // left
      new AABB(Point(-1, 0, 4), Point(1, 15, 6), mat_mdf), // right
    }
  );

  /* Mesa central */
  Object* table = new Object(
    "Table",
    OBB(Point(-60, 0, -60), Point(60, 80, 60)),
    vector<Solid*>{
      new AABB(
        Point(-60, 0, -60),
        Point(60, 80, 60),
        mat_mdf
      )}
  );

  Object* gem = new Object(
    "Gem",
    OBB(Point(-60, 0, -60), Point(60, 80, 60)),
    vector<Solid*>{}
  );
  gem->include("./models/obj/gem.obj", mat_ruby);

  /* Garrafa BTG */
  Object* btg_bottle = new Object(
    "BTG Bottle",
    OBB(Point(-3.5, 0, -3.5), Point(3.5, 22, 3.5)),
    vector<Solid*>{
      new Cylinder(Point(), Vector3(), 18.5, 3, mat_transp_plastic),
      new Cylinder(Point(0, 8, 0), Vector3(), 10.5, 3.2, mat_transp_plastic),
      new Cylinder(Point(0, 10, 0), Vector3(), 8.5, 3.4, mat_transp_plastic),
      new Cylinder(Point(0, 12, 0), Vector3(), 6.5, 3.5, mat_transp_plastic),
      new Cylinder(Point(0, 14, 0), Vector3(), 4.5, 3.5, mat_transp_plastic),
      new Cylinder(Point(0, 18.5, 0), Vector3(), 0.5, 3, mat_transp_plastic),
      new Cylinder(Point(0, 19, 0), Vector3(), 3, 3, mat_blue_chair)
    }
  );

  Object* globe = new Object(
    "Globe",
    OBB(Point(-20, -20, -20), Point(20, 20, 20)),
    vector<Solid*>{
      new Sphere(
      Point(0, 0, 0),
      20,
      mat_mirror
    )}
  );

  gem->scale(10, 10, 10);
  gem->translate(Vector3(0, 80, 0));
  pic_frame->translate(Vector3(-20, 80, 10));
  btg_bottle->translate(Vector3(-40, 80, 20));
  globe->translate(Vector3(0, 100, 0));


  // objects.push_back(gem);
  objects.push_back(globe);
  objects.push_back(table);
  objects.push_back(pic_frame);
  objects.push_back(btg_bottle);

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
