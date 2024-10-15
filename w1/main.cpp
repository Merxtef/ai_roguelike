#include "raylib.h"
#include <entt/entt.hpp>
#include <algorithm>
#include "ecsTypes.h"
#include "roguelike.h"

static void update_camera(Camera2D &cam, entt::registry &registry)
{
  static auto playersView = registry.view<const Position, const IsPlayer>();
  
  for (auto &&[_, pos]: playersView.each())
  {
    cam.target.x = pos.x;
    cam.target.y = pos.y;
  }
}

int main(int argc, const char **argv)
{
  int width = 1920;
  int height = 1080;

  InitWindow(width, height, "w1 AI MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight, height);
    SetWindowSize(width, height);
  }

  entt::registry registry;

  init_roguelike(registry);

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 64.f;

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
  while (!WindowShouldClose())
  {
    process_turn(registry);

    update_camera(camera, registry);

    BeginDrawing();
      ClearBackground(GetColor(0x052c46ff));
      BeginMode2D(camera);
      progress_roguelike_systems(registry);
      EndMode2D();
      print_stats(registry);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
