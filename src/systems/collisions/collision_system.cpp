#include "collision_system.hpp"

#include <cstdio>
#include <damage.hpp>
#include <glm/geometric.hpp>
#include <player_controls.hpp>
#include <player_factories.hpp>
#include <physics_system.hpp>
#include <player_hud.hpp>

#include "ai.hpp"
#include "debuff.hpp"
#include "enemy.hpp"
#include "oxygen.hpp"
#include "player.hpp"
#include "tiny_ecs_registry.hpp"
#include <consumable_utils.hpp>

void CollisionSystem::init(RenderSystem* renderer, LevelSystem* level) {
  this->renderer = renderer;
  this->level = level;
}

bool CollisionSystem::checkBoxCollision(Entity entity_i, Entity entity_j) {
  if (!registry.positions.has(entity_i) || !registry.positions.has(entity_j)) {
    return false;
  }
  Position& position_i = registry.positions.get(entity_i);
  Position& position_j = registry.positions.get(entity_j);
  if (box_collides(position_i, position_j)) {
    registry.collisions.emplace_with_duplicates(entity_i, entity_j);
    registry.collisions.emplace_with_duplicates(entity_j, entity_i);
    return true;
  }
  return false;
}

bool CollisionSystem::checkPlayerMeshCollision(Entity entity_i, Entity entity_j, Entity collisionMesh) {
  if (!registry.positions.has(entity_i) || !registry.positions.has(entity_j)) {
    return false;
  }
  Position& position_i = registry.positions.get(entity_i);
  Position& position_j = registry.positions.get(entity_j);
  if (box_collides(position_i, position_j)) {
    if (mesh_collides(collisionMesh, entity_j)) {
      registry.collisions.emplace_with_duplicates(entity_i, entity_j);
      registry.collisions.emplace_with_duplicates(entity_j, entity_i);
      return true;
    }
  }
  return false;
}

bool CollisionSystem::checkCircleCollision(Entity entity_i, Entity entity_j) {
  if (!registry.positions.has(entity_i) || !registry.positions.has(entity_j)) {
    return false;
  }
  Position& position_i = registry.positions.get(entity_i);
  Position& position_j = registry.positions.get(entity_j); 
  if (circle_collides(position_i, position_j)) {
    registry.collisions.emplace_with_duplicates(entity_i, entity_j);
    registry.collisions.emplace_with_duplicates(entity_j, entity_i);
    return true;
  }
  return false;
}

void CollisionSystem::step(float elapsed_ms) {
  collision_detection();
  collision_resolution();
}

/***********************************
Collision Detection (has precedence noted below)
************************************/
void CollisionSystem::collision_detection() {

  // 1. Detect player projectile collisions
  detectPlayerProjectileCollisions();

  // 2. Detect player collisions
  detectPlayerCollisions();

  // 3. Detect wall collisions
  detectWallCollisions();

  // 4. Detect door collisions
  detectDoorCollisions();
}

void CollisionSystem::detectPlayerProjectileCollisions() {
  ComponentContainer<PlayerProjectile>& playerproj_container =
      registry.playerProjectiles;
  ComponentContainer<Deadly>&       enemy_container      = registry.deadlys;
  ComponentContainer<ActiveWall>&   wall_container       = registry.activeWalls;

  for (uint i = 0; i < playerproj_container.components.size(); i++) {
    Entity entity_i = playerproj_container.entities[i];
    if (!registry.positions.has(entity_i) || 
        registry.playerProjectiles.get(entity_i).is_loaded) {
      continue;
    }

    // detect player projectile and wall collisions
    for (uint j = 0; j < wall_container.size(); j++) {
      Entity entity_j = wall_container.entities[j];
      checkBoxCollision(entity_i, entity_j);
    }

    // detect player projectile and enemy collisions
    for (uint j = 0; j < enemy_container.size(); j++) {
      Entity entity_j = enemy_container.entities[j];
      bool collided = checkCircleCollision(entity_i, entity_j);
      //if the projectile is single target and collided, don't check for anymore enemies.
      PlayerProjectile& playerproj_comp = playerproj_container.components[i];
      if (collided &&
          (playerproj_comp.type == PROJECTILES::HARPOON ||
           playerproj_comp.type == PROJECTILES::NET ||
           playerproj_comp.type == PROJECTILES::TORPEDO)) {
        break;
      }
    }
  }
}

void CollisionSystem::detectPlayerCollisions() {
  ComponentContainer<Player>&           player_container = registry.players;
  ComponentContainer<Deadly>&       enemy_container      = registry.deadlys;
  ComponentContainer<EnemyProjectile>   enemy_proj_container =
      registry.enemyProjectiles;
  ComponentContainer<Consumable>&   consumable_container = registry.consumables;
  ComponentContainer<Interactable>& interactable_container =
      registry.interactable;
      
  for (uint i = 0; i < player_container.components.size(); i++) {
    Entity entity_i = player_container.entities[i];
    if (!registry.positions.has(entity_i)) {
      continue;
    }
    Player    player_comp = registry.players.get(entity_i);

    // detect player and enemy collisions
    for (uint j = 0; j < enemy_container.size(); j++) {
      Entity entity_j = enemy_container.entities[j];
      // don't detect the enemy collision if their attack is on cooldown
      if (registry.modifyOxygenCd.has(entity_j)) {
        ModifyOxygenCD& modifyOxygenCd = registry.modifyOxygenCd.get(entity_j);
        if (modifyOxygenCd.curr_cd > 0.f) {
          continue;
        }
      }
      checkPlayerMeshCollision(entity_i, entity_j, player_comp.collisionMesh);
    }

    for (uint j = 0; j < enemy_proj_container.size(); j++) {
      Entity entity_j = enemy_proj_container.entities[j];
      checkPlayerMeshCollision(entity_i, entity_j, player_comp.collisionMesh);
    }

    for (uint j = 0; j < consumable_container.size(); j++) {
      Entity entity_j = consumable_container.entities[j];
      checkPlayerMeshCollision(entity_i, entity_j, player_comp.collisionMesh);
    }

    for (uint j = 0; j < interactable_container.size(); j++) {
      Entity entity_j = interactable_container.entities[j];
      // don't detect the interactable collision if their attack is on cooldown
      if (registry.modifyOxygenCd.has(entity_j)) {
        ModifyOxygenCD& modifyOxygenCd = registry.modifyOxygenCd.get(entity_j);
        if (modifyOxygenCd.curr_cd > 0.f) {
          continue;
        }
      }
      checkPlayerMeshCollision(entity_i, entity_j, player_comp.collisionMesh);
    }
  }
}

void CollisionSystem::detectWallCollisions() {
  ComponentContainer<Mass>&         mass_container       = registry.masses;
  ComponentContainer<Deadly>&       enemy_container      = registry.deadlys;
  ComponentContainer<EnemyProjectile> enemy_proj_container =
      registry.enemyProjectiles;
  ComponentContainer<ActiveWall>&   wall_container       = registry.activeWalls;  

  for (uint i = 0; i < wall_container.components.size(); i++) {
    Entity entity_i = wall_container.entities[i];
    if (!registry.positions.has(entity_i)) {
      continue;
    }

    for (uint j = 0; j < enemy_container.size(); j++) {
      Entity entity_j = enemy_container.entities[j];
      checkBoxCollision(entity_i, entity_j);
    }

    for (uint j = 0; j < enemy_proj_container.size(); j++) {
      Entity entity_j = enemy_proj_container.entities[j];
      checkBoxCollision(entity_i, entity_j);
    }

    for (uint j = 0; j < mass_container.size(); j++) {
      Entity entity_j = mass_container.entities[j];
      if (entity_i == entity_j) {
        continue;
      }
      if (registry.players.has(entity_j)) {
        Player&   player_comp = registry.players.get(entity_j);
        checkPlayerMeshCollision(entity_j, entity_i, player_comp.collisionMesh);
      } else {
        checkBoxCollision(entity_i, entity_j);
      }
    }
  }
}

void CollisionSystem::detectDoorCollisions() {
  ComponentContainer<ActiveDoor>&   door_container       = registry.activeDoors;
  ComponentContainer<Deadly>&       enemy_container      = registry.deadlys;
  ComponentContainer<Player>&           player_container = registry.players;

  for (uint i = 0; i < door_container.components.size(); i++) {
    Entity entity_i = door_container.entities[i];
    for (uint j = 0; j < enemy_container.size(); j++) {
      Entity entity_j = enemy_container.entities[j];
      checkBoxCollision(entity_i, entity_j);
    }

    for (uint j = 0; j < player_container.size(); j++) {
      Entity entity_j = player_container.entities[j];
      Player&   player_comp = registry.players.get(entity_j);
      checkPlayerMeshCollision(entity_j, entity_i, player_comp.collisionMesh);
    }
  }
}

void CollisionSystem::collision_resolution_debug_info(Entity entity,
                                                      Entity entity_other) {
  printf("Entity:\n");
  registry.list_all_components_of(entity);
  printf("Entity Other:\n");
  registry.list_all_components_of(entity_other);
}

void CollisionSystem::collision_resolution() {
  auto& collisionsRegistry = registry.collisions;
  // printf("Collisions size: %d\n", collisionsRegistry.components.size());
  for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
    Entity entity       = collisionsRegistry.entities[i];
    Entity entity_other = collisionsRegistry.components[i].other;

    // collision_resolution_debug_info(entity, entity_other);

    // Player Collision Handling
    if (registry.players.has(entity)) {
      routePlayerCollisions(entity, entity_other);
    }

    // Wall Collision Handling
    if (registry.activeWalls.has(entity)) {
      routeWallCollisions(entity, entity_other);
    }

    // Door Collision Handling
    if (registry.activeDoors.has(entity)) {
      // std::cout << "something collided" << std::endl;
      routeDoorCollisions(entity, entity_other);
    }

    // Enemy Collision Handling
    if (registry.deadlys.has(entity)) {
      routeEnemyCollisions(entity, entity_other);
    }

    // Player Projectile Collision Handling
    if (registry.playerProjectiles.has(entity)) {
      routePlayerProjCollisions(entity, entity_other);
    }

    // Consumable Collision Handling
    if (registry.consumables.has(entity)) {
      routeConsumableCollisions(entity, entity_other);
    }

    // Interactable Collision Handling
    if (registry.interactable.has(entity)) {
      routeInteractableCollisions(entity, entity_other);
    }
  }
  // Remove all collisions from this simulation step
  registry.collisions.clear();
}

/*********************************************
  Entity -> Other Entity Collision Routing
**********************************************/
void CollisionSystem::routePlayerCollisions(Entity player, Entity other) {
  if (registry.deadlys.has(other)) {
    resolvePlayerEnemyCollision(player, other);
  }
  if (registry.enemyProjectiles.has(other)) {
    resolvePlayerEnemyProjCollision(player, other);
  }
  if (registry.consumables.has(other)) {
    resolvePlayerConsumableCollision(player, other);
  }
  if (registry.activeWalls.has(other)) {
    resolveStopOnWall(other, player);
    Player& player_comp = registry.players.get(player);
    resolveStopOnWall(other, player_comp.collisionMesh);
  }
  if (registry.activeDoors.has(other)) {
    resolveDoorPlayerCollision(other, player);
  }
  if (registry.interactable.has(other)) {
    resolvePlayerInteractableCollision(player, other);
  }
}

void CollisionSystem::routeEnemyCollisions(Entity enemy, Entity other) {
  bool routed = false;
  if (registry.players.has(other)) {
    routed = true;
    resolvePlayerEnemyCollision(other, enemy);
  }
  if (registry.playerProjectiles.has(other)) {
    routed = true;
    resolveEnemyPlayerProjCollision(enemy, other);
  }
  if (registry.activeWalls.has(other)) {
    resolveWallEnemyCollision(other, enemy);
    routed = true;
  }

  // if an enemy is acting as a projectile and hits something, it
  // is no longer acting as a projectile and goes back to its regular ai
  if (routed && registry.actsAsProjectile.has(enemy)) {
    registry.actsAsProjectile.remove(enemy);
  }
}

void CollisionSystem::routeWallCollisions(Entity wall, Entity other) {
  if (!registry.motions.has(other)) {
    return;
  }

  if (registry.players.has(other)) {
    if (registry.masses.has(wall) && registry.masses.has(other)) {
      resolveMassCollision(wall, other);
    } else {
      resolveStopOnWall(wall, other);
      Player& player = registry.players.get(other);
      resolveStopOnWall(wall, player.collisionMesh);
    }
  }
  if (registry.activeWalls.has(other)) {
    if (registry.masses.has(wall) && registry.masses.has(other)) {
      resolveMassCollision(wall, other);
    } else {
      resolveStopOnWall(wall, other);
    }
  }
  if (registry.playerProjectiles.has(other)) {
    resolveWallPlayerProjCollision(wall, other);

    if (registry.breakables.has(wall)) {
      resolveBreakablePlayerProjCollision(wall, other);
    }
  }
  if (registry.enemyProjectiles.has(other)) {
    if (registry.breakables.has(wall)) {
      resolveBreakableEnemyProjCollision(wall, other);
    } else {
      resolveWallEnemyProjCollision(wall, other);
    }
  }
}

void CollisionSystem::routeDoorCollisions(Entity door, Entity other) {
  if (!registry.motions.has(other)) {
    return;
  }

  if (registry.players.has(other)) {
    resolveDoorPlayerCollision(door, other);
  }

  // Since enemies and projectiles can't enter different rooms, simply treat
  // their collisions like a wall.
  if (registry.playerProjectiles.has(other)) {
    resolveWallPlayerProjCollision(door, other);
  }
  if (registry.deadlys.has(other)) {
    resolveWallEnemyCollision(door, other);
  }
}

void CollisionSystem::routePlayerProjCollisions(Entity player_proj,
                                                Entity other) {
  if (registry.deadlys.has(other)) {
    resolveEnemyPlayerProjCollision(other, player_proj);
  }
  if (registry.activeWalls.has(other)) {
    resolveWallPlayerProjCollision(other, player_proj);
  }
  if (registry.breakables.has(other)) {
    modifyOxygen(other, player_proj);
  }

  PlayerProjectile& player_proj_component =
      registry.playerProjectiles.get(player_proj);
  bool checkWepSwapped = player_proj != player_projectile;

  // Remove render projectile if weapons have been swapped or collision just
  // occured, except for concussive (handled in debuff.cpp) & shrimp (handled in
  // resolveWallPlayerProj)
  if (checkWepSwapped &&
      player_proj_component.type != PROJECTILES::CONCUSSIVE &&
      player_proj_component.type != PROJECTILES::SHRIMP) {
    destroyGunOrProjectile(player_proj);
  }
}

void CollisionSystem::routeConsumableCollisions(Entity consumable,
                                                Entity other) {
  if (registry.players.has(other)) {
    resolvePlayerConsumableCollision(other, consumable);
  }
}

void CollisionSystem::routeInteractableCollisions(Entity interactable,
                                                  Entity other) {
  if (registry.players.has(other)) {
    resolvePlayerInteractableCollision(other, interactable);
  }
}

/*********************************************
    Entity <-> Entity Collision Resolutions
**********************************************/
void CollisionSystem::resolvePlayerEnemyCollision(Entity player, Entity enemy) {
  handle_debuffs(player, enemy);
  addDamageIndicatorTimer(player);
  modifyOxygen(player, enemy);
}

void CollisionSystem::resolvePlayerEnemyProjCollision(Entity player, Entity enemy_proj) {
  // For now it's almost equal to the above, but make a new function just to open it to changes
  handle_debuffs(player, enemy_proj);
  modifyOxygen(player, enemy_proj);

  // Assume the projectile should poof upon impact
  registry.remove_all_components_of(enemy_proj);
}

void CollisionSystem::resolvePlayerConsumableCollision(Entity player,
                                                       Entity consumable) {
  handle_consumable_collisions(player, consumable, renderer);
}

void CollisionSystem::resolvePlayerInteractableCollision(Entity player,
                                                         Entity interactable) {
  if (registry.deathTimers.has(player)) {
    return;
  }
  // TODO: add more affects M2+

  // will add oxygen to the player if it exists
  modifyOxygen(player, interactable);
}

void CollisionSystem::resolveEnemyPlayerProjCollision(Entity enemy,
                                                      Entity player_proj) {
  PlayerProjectile& playerproj_comp =
      registry.playerProjectiles.get(player_proj);

  if (!registry.motions.has(player_proj)) {
    return;
  }
  Motion& playerproj_motion = registry.motions.get(player_proj);

  modifyOxygen(enemy, player_proj);

  switch (playerproj_comp.type) {
    case PROJECTILES::HARPOON:
      break;
    case PROJECTILES::NET:
      handle_debuffs(enemy, player_proj);
      break;
    case PROJECTILES::CONCUSSIVE:
      // ignore boxes and jellyfish.
      if (!registry.activeWalls.has(enemy) && registry.motions.has(enemy)) {
        handle_debuffs(enemy, player_proj);
      }
      break;
    case PROJECTILES::TORPEDO:
      detectAndResolveExplosion(player_proj, enemy);
      break;
    case PROJECTILES::SHRIMP:
      /*detectAndResolveConeAOE(player_proj, enemy, SHRIMP_DAMAGE_ANGLE);*/
      break;
  }

  addDamageIndicatorTimer(enemy);

  // make enemies that track the player briefly start tracking them regardless
  // of range
  if (registry.trackPlayer.has(enemy)) {
    TracksPlayer& tracks = registry.trackPlayer.get(enemy);
    tracks.active_track  = true;
  }

  if (playerproj_comp.type != PROJECTILES::CONCUSSIVE && playerproj_comp.type != PROJECTILES::SHRIMP) {
    playerproj_motion.velocity = vec2(0.0f, 0.0f);
    playerproj_comp.is_loaded  = true;
  }
}

void CollisionSystem::resolveBreakablePlayerProjCollision(Entity breakable,
                                                          Entity player_proj) {
  if (!registry.motions.has(player_proj)) {
    return;
  }

  PlayerProjectile& playerproj_comp =
      registry.playerProjectiles.get(player_proj);

  modifyOxygen(breakable, player_proj);

  if (playerproj_comp.type == PROJECTILES::TORPEDO) {
    detectAndResolveExplosion(player_proj, breakable);
  }
}

void CollisionSystem::resolveBreakableEnemyProjCollision(Entity breakable,
                                                          Entity enemy_proj) {
  if (!registry.motions.has(enemy_proj)) {
    return;
  }

  modifyOxygen(breakable, enemy_proj);

  registry.remove_all_components_of(enemy_proj);
}

//hit_entity is the entity that got hit
void CollisionSystem::detectAndResolveExplosion(Entity proj, Entity hit_entity) {
  if (!registry.sounds.has(proj)) {
    registry.sounds.insert(proj, Sound(SOUND_ASSET_ID::EXPLOSION));
  }
  for (Entity enemy_check : registry.deadlys.entities) {
    if (enemy_check == hit_entity || !registry.positions.has(hit_entity)) {
      continue;
    }
    Position&     playerproj_position = registry.positions.get(proj);
    AreaOfEffect& playerproj_aoe      = registry.aoe.get(proj);
    Position&     enemy_position      = registry.positions.get(enemy_check);

    if (circle_box_collides(playerproj_position, playerproj_aoe.radius,
                            enemy_position)) {
      modifyOxygen(enemy_check, proj);
      addDamageIndicatorTimer(enemy_check);
    }
  }

  for (Entity breakable_check : registry.breakables.entities) {
    if (breakable_check == hit_entity || !registry.positions.has(hit_entity)) {
      continue;
    }
    Position&     playerproj_position = registry.positions.get(proj);
    AreaOfEffect& playerproj_aoe      = registry.aoe.get(proj);
    Position&     enemy_position      = registry.positions.get(breakable_check);

    if (circle_box_collides(playerproj_position, playerproj_aoe.radius,
                            enemy_position)) {
      modifyOxygen(breakable_check, proj);
      addDamageIndicatorTimer(breakable_check);
    }
  }
}

void CollisionSystem::detectAndResolveConeAOE(Entity proj, Entity enemy, float angle) {
  for (Entity enemy_check : registry.deadlys.entities) {
    if (enemy_check == enemy || !registry.positions.has(enemy)) {
      continue;
    }
    Position&     playerproj_position = registry.positions.get(proj);
    AreaOfEffect& playerproj_aoe      = registry.aoe.get(proj);
    Position&     enemy_position      = registry.positions.get(enemy_check);

    float circle_angle   = playerproj_position.angle;
    vec2  pos_diff       = playerproj_position.position - enemy_position.position;
    float proj_ent_angle = atan2(pos_diff.y, pos_diff.x);
    if (registry.playerProjectiles.get(proj).is_flipped) {
      circle_angle -= M_PI;
    }
    if (circle_angle < 0) {
      circle_angle += 2 * M_PI;
    }
    if (proj_ent_angle < 0) {
      proj_ent_angle += 2 * M_PI;
    }

    float anglediff =
        fmod((circle_angle - proj_ent_angle + 3 * M_PI), 2 * M_PI);

    float is_inbetween = abs(anglediff) <= angle;


    /*proj_ent_angle = std::fmod(2.f * M_PI + proj_ent_angle, 2.f * M_PI);
    float min      = std::fmod(20.f * M_PI + circle_angle - angle, 2.f * M_PI);
    float max      = std::fmod(20.f * M_PI + circle_angle + angle, 2.f * M_PI);

    bool is_inbetween = !(min <= proj_ent_angle) || (proj_ent_angle <= max);

    if (min < max) {
      is_inbetween = !(min <= proj_ent_angle) && (proj_ent_angle <= max);
    }*/

    /*if (circle_box_collides(playerproj_position, playerproj_aoe.radius,
                            enemy_position)) {
      printf("Proj angle %f\n Distance Angle %f\n", anglediff, angle);
    }*/

    if (circle_box_collides(playerproj_position, playerproj_aoe.radius,
                            enemy_position) && is_inbetween) {
      modifyOxygen(enemy_check, proj);
      addDamageIndicatorTimer(enemy_check);
    }
  }
}

void CollisionSystem::resolveWallPlayerProjCollision(Entity wall,
                                                     Entity player_proj) {
  if (!registry.motions.has(player_proj) ||
      !registry.playerProjectiles.has(player_proj)) {
    return;
  }
  Motion&           proj_motion = registry.motions.get(player_proj);
  PlayerProjectile& proj_component =
      registry.playerProjectiles.get(player_proj);
  Inventory& inventory = registry.inventory.get(player);

  bool check_wep_swap = player_projectile != player_proj;
  proj_motion.velocity     = vec2(0.f);
  proj_component.is_loaded = true;
  if (proj_component.type == PROJECTILES::SHRIMP) {
    if (check_wep_swap) {
      destroyGunOrProjectile(player_proj);
    }
    if (inventory.shrimp <= 0) {
      doWeaponSwap(harpoon, harpoon_gun, PROJECTILES::HARPOON);
      changeSelectedCounterColour(INVENTORY::HARPOON);
    }
  } else if (proj_component.type == PROJECTILES::CONCUSSIVE) {
    if (check_wep_swap) {
      destroyGunOrProjectile(player_proj);
    }
    if (inventory.concussors <= 0) {
      doWeaponSwap(harpoon, harpoon_gun, PROJECTILES::HARPOON);
      changeSelectedCounterColour(INVENTORY::HARPOON);
    }
  } else {
    if (proj_component.type == PROJECTILES::TORPEDO) {
      detectAndResolveExplosion(player_proj, wall);
    }
    proj_motion.velocity     = vec2(0.f);
    proj_component.is_loaded = true;
  }
}

void CollisionSystem::resolveWallEnemyProjCollision(Entity wall,
  Entity enemy_proj) {
  registry.remove_all_components_of(enemy_proj);
}

void CollisionSystem::resolveWallEnemyCollision(Entity wall, Entity enemy) {
  if (!registry.motions.has(enemy) || !registry.positions.has(enemy)) {
    return;
  }

  Motion&   enemy_motion   = registry.motions.get(enemy);
  Position& enemy_position = registry.positions.get(enemy);
  Position& wall_position  = registry.positions.get(wall);
  vec2 wall_dir = normalize(wall_position.position - enemy_position.position);
  vec2 temp_velocity = enemy_motion.velocity;

  resolveStopOnWall(wall, enemy);

  // if the enemy is actively tracking the player, route them around the wall
  if ((registry.trackPlayer.has(enemy) &&
       registry.trackPlayer.get(enemy).active_track) ||
      (registry.trackPlayer.has(enemy) &&
       registry.trackPlayer.get(enemy).active_track)) {
    vec2  enemy_dir = normalize(temp_velocity);
    float velocity  = sqrt(dot(temp_velocity, temp_velocity));
    float acceleration =
        sqrt(dot(enemy_motion.acceleration, enemy_motion.acceleration));
    vec2 new_dir              = normalize(enemy_dir - wall_dir);
    enemy_motion.velocity     = new_dir * velocity;
    enemy_motion.acceleration = new_dir * acceleration;
  } else {
    enemy_motion.velocity = temp_velocity;
    // adjust enemy ai
    enemy_motion.velocity *= -1.0f;
    enemy_motion.acceleration *= -1.0f;
  }
  enemy_position.scale.x = abs(enemy_position.scale.x);
  if (enemy_motion.velocity.x > 0) {
    enemy_position.scale.x *= -1.0f;
  }

  if (registry.bosses.has(enemy)) {
    Boss& boss = registry.bosses.get(enemy);
    if (boss.type == BossType::SHARKMAN) {
      // break crates if sharkman hits them while targeting player
      if (registry.breakables.has(wall) && registry.trackPlayer.has(enemy) &&
          is_tracking(enemy)) {
        modifyOxygenAmount(enemy, SHARKMAN_SELF_DMG);
        modifyOxygenAmount(wall, SHARKMAN_SELF_DMG);
        Motion& motion = registry.motions.get(enemy);
        float   speed  = sqrt(dot(motion.velocity, motion.velocity));
        motion.velocity =
            normalize(motion.velocity) * (speed + (float)SHARKMAN_MS_INC);

        // reset sharkman to wander
        printf("Sharkman hit a crate, resetting to wander\n");
        boss.curr_cd = SHARKMAN_AI_CD;
        removeFromAI(enemy);
        addSharkmanWander();
      }
      choose_new_direction(enemy, wall);
    }
  }
}

void CollisionSystem::resolveStopOnWall(Entity wall, Entity entity) {
  Position& wall_position   = registry.positions.get(wall);
  Position& entity_position = registry.positions.get(entity);

  vec4 wall_bounds = get_bounds(wall_position);

  float left_bound_wall  = wall_bounds[0];
  float right_bound_wall = wall_bounds[1];
  float top_bound_wall   = wall_bounds[2];
  float bot_bound_wall   = wall_bounds[3];

  vec4 entity_bounds = get_bounds(entity_position);

  float left_bound_entity  = entity_bounds[0];
  float right_bound_entity = entity_bounds[1];
  float top_bound_entity   = entity_bounds[2];
  float bot_bound_entity   = entity_bounds[3];

  float entitys_right_overlaps_left_wall =
      (right_bound_entity - left_bound_wall);
  float entitys_left_overlaps_right_wall =
      (right_bound_wall - left_bound_entity);
  float entitys_bot_overlaps_top_wall = (bot_bound_entity - top_bound_wall);
  float entitys_top_overlaps_bot_wall = (bot_bound_wall - top_bound_entity);

  // We need to find the smallest overlap horizontally and verticallly to
  // determine where the overlap happened.
  float overlapX =
      min(entitys_right_overlaps_left_wall, entitys_left_overlaps_right_wall);
  float overlapY =
      min(entitys_bot_overlaps_top_wall, entitys_top_overlaps_bot_wall);

  // If the overlap in the X direction is smaller, it means that the collision
  // occured here. Vice versa for overlap in Y direction.
  if (overlapX < overlapY) {
    // Respective to the wall,
    // If the entity is on the left of the wall, then we need to push him left.
    // If the entity is on the right of the wall, we need to push him right.
    overlapX = (entity_position.position.x < wall_position.position.x)
                   ? -1 * overlapX
                   : overlapX;
    entity_position.position.x += overlapX;
    if (registry.motions.has(entity) && !registry.players.has(entity)) {
      registry.motions.get(entity).velocity.x = 0;
    }
  } else {
    // Respective to the wall,
    // If the entity is above the wall, then we need to push up left.
    // If the entity is below the wall, then we need to push him down.
    overlapY = (entity_position.position.y < wall_position.position.y)
                   ? -1 * overlapY
                   : overlapY;
    entity_position.position.y += overlapY;
    if (registry.motions.has(entity) && !registry.players.has(entity)) {
      registry.motions.get(entity).velocity.y = 0;
    }
  }
}

void CollisionSystem::resolveMassCollision(Entity wall, Entity other) {
  Motion&   wall_motion  = registry.motions.get(wall);
  Motion&   other_motion = registry.motions.get(other);
  Position& wall_pos     = registry.positions.get(wall);
  Position& other_pos    = registry.positions.get(other);

  bool is_horizontal_collision = false;
  // Determine if this a horizontal or vertical collision
  vec2 pos_diff = wall_pos.position - other_pos.position;

  if (abs(pos_diff.x) > abs(pos_diff.y)) {
    is_horizontal_collision = true;
  }

  // This is here because sometimes it still counts as a collision, even if they
  // aren't colliding
  float wall_position =
      is_horizontal_collision ? wall_pos.position.x : wall_pos.position.y;
  float wall_velo =
      is_horizontal_collision ? wall_motion.velocity.x : wall_motion.velocity.y;
  float other_position =
      is_horizontal_collision ? other_pos.position.x : other_pos.position.y;
  float other_velo = is_horizontal_collision ? other_motion.velocity.x
                                             : other_motion.velocity.y;
  if ((other_position > wall_position && other_velo > wall_velo) ||
      (other_position < wall_position && other_velo < wall_velo)) {
    return;
  }

  // Assume these collisions are inelastic - that is, that kinetic energy is
  // preserved

  float wall_mass  = registry.masses.get(wall).mass;
  float other_mass = registry.masses.get(other).mass;
  // p = momentum
  float p_wall  = is_horizontal_collision ? wall_mass * wall_motion.velocity.x
                                          : wall_mass * wall_motion.velocity.y;
  float p_other = is_horizontal_collision
                      ? other_mass * other_motion.velocity.x
                      : other_mass * other_motion.velocity.y;

  float v_final = (p_wall + p_other) / (wall_mass + other_mass);

  if (is_horizontal_collision) {
    wall_motion.velocity.x  = v_final;
    other_motion.velocity.x = v_final;
  } else {
    wall_motion.velocity.y  = v_final;
    other_motion.velocity.y = v_final;
  }
}

void CollisionSystem::resolveDoorPlayerCollision(Entity door, Entity player) {
  rt_entity                      = Entity();
  RoomTransition& roomTransition = registry.roomTransitions.emplace(rt_entity);

  DoorConnection& door_connection = registry.doorConnections.get(door);
  roomTransition.door_connection  = door_connection;

  transitioning = true;

  registry.sounds.insert(rt_entity, Sound(SOUND_ASSET_ID::DOOR));

  PlayerProjectile& pp   = registry.playerProjectiles.get(player_projectile);
  Motion&           pp_m = registry.motions.get(player_projectile);
  pp.is_loaded           = true;
  pp_m.velocity          = {0.f, 0.f};
}
