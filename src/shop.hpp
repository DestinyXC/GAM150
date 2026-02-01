#ifndef SHOP_HPP
#define SHOP_HPP

#include "AEEngine.h"

// Shop State Functions
void Shop_Load();
void Shop_Initialize();
void Shop_Update();
void Shop_Draw();
void Shop_Free();
void Shop_Unload();

// External reference to main game's oxygen upgrade level
extern int oxygen_upgrade_level;  // This refers to main.cpp's variable

#endif // SHOP_HPP
