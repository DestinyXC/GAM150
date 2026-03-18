#pragma once
#ifndef LIGHTSYSTEM_H
#define LIGHTSYSTEM_H
#include "AEEngine.h"

#pragma once

// ---------------------------------------------------------------------------
// lightsystem.hpp  -  Darkness overlay + player torch for Core Break
// ---------------------------------------------------------------------------
//
// Two-layer approach (draw both AFTER the world, BEFORE the HUD):
//
//   1. LightSystem_DrawDarkness(camera_x, camera_y, player_x, player_y)
//        - Covers the ENTIRE map below the safe-zone start row with a
//          near-opaque black rectangle.
//        - The safe-zone row (surface area) is left fully lit.
//        - Alpha is configurable via DARKNESS_ALPHA below.
//
//   2. LightSystem_DrawTorch(camera_x, camera_y, player_x, player_y, radius)
//        - Draws a soft radial "hole" centred on the player using a
//          multi-ring additive / subtractive colour mesh so it punches
//          through the darkness layer above it.
//        - Call this AFTER DrawDarkness in the same frame so the bright
//          circle appears on top.
//
// Lifecycle:
//   Game_Init  -> LightSystem_Init()
//   Game_Draw  -> LightSystem_DrawDarkness(...)
//                 LightSystem_DrawTorch(...)
//   Game_Kill  -> LightSystem_Kill()
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CONFIGURATION - tweak these to taste
// ---------------------------------------------------------------------------

// How opaque the darkness is (0.0 = invisible, 1.0 = fully black)
#define DARKNESS_ALPHA          0.85f

// Default torch radius in world units (can be overridden per-call)
#define TORCH_RADIUS_DEFAULT    180.0f

// How many ring segments are used to build the soft torch edge
#define TORCH_RING_SEGMENTS     64

// How many concentric rings fade the torch edge (more = softer gradient)
#define TORCH_GRADIENT_RINGS    12

// The safe-zone Y boundary: rows ABOVE this Y are NOT darkened
// Matches main.cpp:  safezone_y_max = -2530.0f
#define LIGHT_SAFEZONE_Y_MAX   -2530.0f

// Map dimensions - must match main.cpp
#define LIGHT_MAP_WIDTH         17
#define LIGHT_MAP_HEIGHT        100
#define LIGHT_TILE_SIZE         64.0f

// ---------------------------------------------------------------------------
// PUBLIC FUNCTIONS
// ---------------------------------------------------------------------------

// Allocate all meshes. Call once during Game_Init.
void LightSystem_Init(void);

// Draw the darkness overlay that covers the underground area.
// Call this AFTER drawing the world tiles and enemies but BEFORE the HUD.
void LightSystem_DrawDarkness(float camera_x, float camera_y,
    float player_x, float player_y);

// Draw the player torch circle that brightens the area around the player.
// Call this IMMEDIATELY AFTER LightSystem_DrawDarkness each frame.
// Pass the current torch radius (respects torch_upgrade_level from shop.hpp).
void LightSystem_DrawTorch(float camera_x, float camera_y,
    float player_x, float player_y,
    float torch_radius);

// Free all meshes. Call during Game_Kill.
void LightSystem_Kill(void);

#endif // LIGHTSYSTEM_H