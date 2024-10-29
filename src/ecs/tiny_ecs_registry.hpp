#pragma once
#include <vector>

#include "abilities.hpp"
#include "ai.hpp"
#include "components.hpp"
#include "enemy.hpp"
#include "environment.hpp"
#include "items.hpp"
#include "misc.hpp"
#include "oxygen.hpp"
#include "physics.hpp"
#include "player.hpp"
#include "tiny_ecs.hpp"
#include "audio.hpp"
#include "status.hpp"

class ECSRegistry {
  // Callbacks to remove a particular or all entities in the system
  std::vector<ContainerInterface*> registry_list;

  public:
  // Manually created list of all components this game has
  // physics related
  ComponentContainer<Motion> motions;
  ComponentContainer<Position> positions;
  ComponentContainer<Collision> collisions;

  // player related
  ComponentContainer<DeathTimer>       deathTimers;
  ComponentContainer<Player>           players;
  ComponentContainer<PlayerWeapon>     playerWeapons;
  ComponentContainer<PlayerProjectile> playerProjectiles;
  ComponentContainer<Inventory>        inventory;
  ComponentContainer<PlayerHUD>        playerHUD;

  // enemy related
  ComponentContainer<Deadly>   deadlys;
  ComponentContainer<AttackCD> attackCD;

  // oxygen related
  ComponentContainer<Oxygen>         oxygen;
  ComponentContainer<OxygenModifier> oxygenModifiers;

  // ai related
  ComponentContainer<Wander> wanders;

  // abilities related
  ComponentContainer<Stun> stuns;

  // render related
  ComponentContainer<Mesh*>         meshPtrs;
  ComponentContainer<RenderRequest> renderRequests;
  ComponentContainer<vec3>          colors;
  ComponentContainer<ScreenState>   screenStates;

  // level related
  ComponentContainer<SpaceBoundingBox> bounding_boxes;
  ComponentContainer<Vector>           vectors;
  ComponentContainer<Space>            spaces;
  ComponentContainer<Adjacency>        adjacencies;
  ComponentContainer<ActiveWall>       activeWalls;
  ComponentContainer<Interactable>     interactable;

  // status related
  ComponentContainer<LowOxygen> lowOxygen;
  ComponentContainer<Stunned>   stunned;

  // audio related
  ComponentContainer<Sound> sounds;
  ComponentContainer<Music> musics;

  // other
  ComponentContainer<Consumable>     consumables;
  ComponentContainer<DebugComponent> debugComponents;

  // constructor that adds all containers for looping over them
  // IMPORTANT: Don't forget to add any newly added containers!
  ECSRegistry() {
    // physics related
    registry_list.push_back(&motions);
    registry_list.push_back(&collisions);
    registry_list.push_back(&positions);
    // player related
    registry_list.push_back(&deathTimers);
    registry_list.push_back(&players);
    registry_list.push_back(&playerWeapons);
    registry_list.push_back(&playerProjectiles);
    registry_list.push_back(&inventory);
    registry_list.push_back(&playerHUD);
    // enemy related
    registry_list.push_back(&deadlys);
    registry_list.push_back(&attackCD);
    // oxygen related
    registry_list.push_back(&oxygen);
    registry_list.push_back(&oxygenModifiers);
    // ai related
    registry_list.push_back(&wanders);
    // abilities related
    registry_list.push_back(&stuns);
    registry_list.push_back(&stunned);
    // render related
    registry_list.push_back(&meshPtrs);
    registry_list.push_back(&renderRequests);
    registry_list.push_back(&screenStates);
    registry_list.push_back(&colors);
    // level related
    registry_list.push_back(&bounding_boxes);
    registry_list.push_back(&vectors);
    registry_list.push_back(&spaces);
    registry_list.push_back(&adjacencies);
    registry_list.push_back(&activeWalls);
    // status related
    registry_list.push_back(&lowOxygen);
    // audio related
    registry_list.push_back(&sounds);
    registry_list.push_back(&musics);
    registry_list.push_back(&interactable);
    // other
    registry_list.push_back(&debugComponents);
    registry_list.push_back(&consumables);
  }

  void clear_all_components() {
    for (ContainerInterface* reg : registry_list) reg->clear();
  }

  void list_all_components() {
    printf("Debug info on all registry entries:\n");
    for (ContainerInterface* reg : registry_list)
      if (reg->size() > 0)
        printf("%4d components of type %s\n", (int)reg->size(),
               typeid(*reg).name());
  }

  void list_all_components_of(Entity e) {
    printf("Debug info on components of entity %u:\n", (unsigned int)e);
    for (ContainerInterface* reg : registry_list)
      if (reg->has(e)) printf("type %s\n", typeid(*reg).name());
  }

  void remove_all_components_of(Entity e) {
    for (ContainerInterface* reg : registry_list) reg->remove(e);
  }
};

extern ECSRegistry registry;
