#ifndef SHOP_HPP
#define SHOP_HPP

#include "AEEngine.h"

// ---------------------------------------------------------------------------
// SHOP STATE INTERFACE
// These functions are defined in shop.cpp and called from main.cpp
// ---------------------------------------------------------------------------

// Shop state management
extern int shop_is_active;  // 1 when shop is open, 0 when closed

// Upgrade levels (shared with main.cpp oxygen system)
extern int oxygen_upgrade_level;
extern int pick_upgrade_level;
extern int sanity_upgrade_level;
extern int torch_upgrade_level;

// Shop state functions (Load/Unload happen once, Init/Free happen each time shop opens/closes)
void Shop_Load();        // Load textures and create meshes (call in Game_Init)
void Shop_Initialize();  // Set shop_is_active = 1 (call when entering shop)
void Shop_Update();      // Handle shop input and upgrade clicks
void Shop_Draw();        // Render the shop UI
void Shop_Free();        // Set shop_is_active = 0 (call when exiting shop)
void Shop_Unload();      // Free all resources (call in Game_Kill)

#endif // SHOP_HPP