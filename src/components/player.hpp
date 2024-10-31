#pragma once
// Player HUD component
struct PlayerHUD {};

// Projectile component
struct PlayerProjectile {
  bool is_loaded = false;  // flags whether to follow a weapon's position or act
                           // as a projectile
  int type;
};

// Weapon component
struct PlayerWeapon {
  Entity projectile;
};

// Consumable inventory
struct Inventory {
  // TODO: Change this when pickups are added for each consumable
  unsigned int nets       = 3;
  unsigned int concussors = 3;
  unsigned int torpedos   = 3;
  unsigned int shrimp     = 3;
  unsigned int keys       = 0;
};

// Player component
struct Player {
  Entity weapon;

  // Controls
  bool upHeld    = false;
  bool downHeld  = false;
  bool leftHeld  = false;
  bool rightHeld = false;
  bool dashing   = false;
};