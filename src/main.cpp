#include "ai_system.hpp"
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>

// internal
#include "audio_system.hpp"
#include "collision_system.hpp"
#include "level_system.hpp"
#include "physics_system.hpp"
#include "render_system.hpp"
#include "world_system.hpp"

using Clock = std::chrono::high_resolution_clock;

bool   krab_boss_encountered = false;
bool   sharkman_encountered  = false;
bool   is_intro              = false;
bool   is_start              = false;
bool   is_paused             = false;
bool   is_krab_cutscene      = false;
bool   is_sharkman_cutscene  = false;
bool   is_death              = false;
bool   is_end                = false;
Entity overlay;
bool   room_transitioning = false;
Entity rt_entity;

Entity player;

// Consumable Entities
Entity      player_weapon;
Entity      player_projectile;
Entity      harpoon;
Entity      net;
Entity      concussive;
Entity      torpedo;
Entity      shrimp;
PROJECTILES wep_type;

Entity harpoon_gun;
Entity net_gun;
Entity concussive_gun;
Entity torpedo_gun;
Entity shrimp_gun;

// Entry point
int main() {
  // Global systems
  WorldSystem     world;
  RenderSystem    renderer;
  AISystem        ai;
  PhysicsSystem   physics;
  AudioSystem     audios;
  CollisionSystem collisions;
  LevelSystem     level;

  // Initializing window
  GLFWwindow* window = world.create_window();
  if (!window) {
    // Time to read the error message
    printf("Press any key to exit");
    getchar();
    return EXIT_FAILURE;
  }

  // Generate a level.
  LevelBuilder level_builder = LevelBuilder();
  level_builder.generate_random_level();

  // initialize the main systems
  level.init(&renderer, &level_builder);
  collisions.init(&renderer, &level);
  renderer.init(window);
  world.init(&renderer, &level);
  audios.init();
  ai.init(&renderer);

  is_start = true;

  // variable timestep loop
  auto t = Clock::now();
  while (!world.is_over()) {
    // Processes system messages, if this wasn't present the window would become
    // unresponsive
    glfwPollEvents();
    // Calculating elapsed times in milliseconds from the previous iteration
    auto  now = Clock::now();
    float elapsed_ms =
        (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t))
            .count() /
        1000;
    t = now;

    world.step(elapsed_ms);
    bool is_frozen_state = is_intro || is_start || is_paused ||
                           is_krab_cutscene || is_sharkman_cutscene ||
                           is_death || is_end || room_transitioning;
    if (!is_frozen_state) {
      // Note: WorldSystem::step runs simply to update FPS counter, but is
      // mostly disabled
      ai.step(elapsed_ms);
      physics.step(elapsed_ms);
      collisions.step(elapsed_ms);
    }
    audios.step(elapsed_ms);
    renderer.draw();
  }

  return EXIT_SUCCESS;
}
