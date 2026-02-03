#include "shop.hpp"
#include <stdio.h>
#include "AEEngine.h"

// ---------------------------------------------------------------------------
// SHOP SYSTEM GLOBALS
// ---------------------------------------------------------------------------

// Shop state
int shop_is_active = 0;

// Shop popup dimensions
float shop_popup_width = 1600.0f;
float shop_popup_height = 900.0f;

// Shop upgrade boxes (4 boxes)
float upgrade_box_width = 220.0f;
float upgrade_box_height = 50.0f;
float upgrade_box_spacing = 130.0f;

// Hover states for each box
int hover_box1 = 0;  // Pick/Shovel upgrade
int hover_box2 = 0;  // Oxygen upgrade
int hover_box3 = 0;  // Sanity upgrade
int hover_box4 = 0;  // Torch/Light upgrade

// Upgrade levels (shared with main.cpp)
int oxygen_upgrade_level = 0;
int pick_upgrade_level = 0;
int sanity_upgrade_level = 0;
int torch_upgrade_level = 0;

// Textures
AEGfxTexture* shopBackgroundTexture = nullptr;

// Meshes
AEGfxVertexList* shopBackgroundImageMesh = nullptr;
AEGfxVertexList* upgradeBoxMesh = nullptr;
AEGfxVertexList* upgradeBoxBorderMesh = nullptr;

// Font
s8 shop_font_id = -1;

// ---------------------------------------------------------------------------
// MESH CREATION FUNCTIONS
// ---------------------------------------------------------------------------

AEGfxVertexList* CreateShopBackgroundImageMesh(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    AEGfxTriAdd(
        -half_width, -half_height, 0xFFFFFFFF, 0.0f, 1.0f,
        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(
        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
        half_width, half_height, 0xFFFFFFFF, 1.0f, 0.0f,
        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateUpgradeBoxMesh(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    AEGfxTriAdd(
        -half_width, -half_height, 0xFFFFFFFF, 0.0f, 1.0f,
        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(
        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
        half_width, half_height, 0xFFFFFFFF, 1.0f, 0.0f,
        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateUpgradeBoxBorderMesh(float width, float height, float thickness)
{
    AEGfxMeshStart();

    float hw = width / 2.0f;
    float hh = height / 2.0f;
    float t = thickness;

    // Top border
    AEGfxTriAdd(-hw, hh - t, 0xFFFFFFFF, 0.0f, 0.0f, hw, hh - t, 0xFFFFFFFF, 1.0f, 0.0f, -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxTriAdd(hw, hh - t, 0xFFFFFFFF, 1.0f, 0.0f, hw, hh, 0xFFFFFFFF, 1.0f, 1.0f, -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);

    // Bottom border
    AEGfxTriAdd(-hw, -hh, 0xFFFFFFFF, 0.0f, 0.0f, hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f, -hw, -hh + t, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxTriAdd(hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f, hw, -hh + t, 0xFFFFFFFF, 1.0f, 1.0f, -hw, -hh + t, 0xFFFFFFFF, 0.0f, 1.0f);

    // Left border
    AEGfxTriAdd(-hw, -hh, 0xFFFFFFFF, 0.0f, 0.0f, -hw + t, -hh, 0xFFFFFFFF, 1.0f, 0.0f, -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxTriAdd(-hw + t, -hh, 0xFFFFFFFF, 1.0f, 0.0f, -hw + t, hh, 0xFFFFFFFF, 1.0f, 1.0f, -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);

    // Right border
    AEGfxTriAdd(hw - t, -hh, 0xFFFFFFFF, 0.0f, 0.0f, hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f, hw - t, hh, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxTriAdd(hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f, hw, hh, 0xFFFFFFFF, 1.0f, 1.0f, hw - t, hh, 0xFFFFFFFF, 0.0f, 1.0f);

    return AEGfxMeshEnd();
}

// ---------------------------------------------------------------------------
// COST FUNCTIONS
// ---------------------------------------------------------------------------

int GetPickUpgradeCost()
{
    switch (pick_upgrade_level)
    {
    case 0: return 1;
    case 1: return 2;
    case 2: return 3;
    case 3: return 120;
    default: return 999;
    }
}

int GetOxygenUpgradeCost()
{
    switch (oxygen_upgrade_level)
    {
    case 0: return 1;
    case 1: return 2;
    case 2: return 3;
    case 3: return 100;
    default: return 999;
    }
}

int GetSanityUpgradeCost()
{
    switch (sanity_upgrade_level)
    {
    case 0: return 20;
    case 1: return 40;
    case 2: return 80;
    default: return 999;
    }
}

int GetTorchUpgradeCost()
{
    switch (torch_upgrade_level)
    {
    case 0: return 12;
    case 1: return 24;
    case 2: return 48;
    default: return 999;
    }
}

// ---------------------------------------------------------------------------
// UPGRADE FUNCTIONS (WITH ROCK CURRENCY CHECK)
// ---------------------------------------------------------------------------

void PickUpgrade()
{
    int cost = GetPickUpgradeCost();

    if (rocks_mined >= cost && pick_upgrade_level < 4)
    {
        rocks_mined -= cost;
        pick_upgrade_level++;

        // Update mining speed
        switch (pick_upgrade_level)
        {
        case 1: rock_mining_duration = 1.5f; break;
        case 2: rock_mining_duration = 1.0f; break;
        case 3: rock_mining_duration = 0.5f; break;
        case 4: rock_mining_duration = 0.25f; break;
        }

        printf("Pick upgraded to level %d! (Cost: %d rocks)\n", pick_upgrade_level, cost);
    }
    else if (rocks_mined < cost)
    {
        printf("Not enough rocks! Need %d, have %d\n", cost, rocks_mined);
    }
    else
    {
        printf("Pick already at max level!\n");
    }
}

void OxygenUpgrade()
{
    int cost = GetOxygenUpgradeCost();

    if (rocks_mined >= cost && oxygen_upgrade_level < 4)
    {
        rocks_mined -= cost;
        oxygen_upgrade_level++;
        printf("Oxygen upgraded to level %d! (Cost: %d rocks)\n", oxygen_upgrade_level, cost);
    }
    else if (rocks_mined < cost)
    {
        printf("Not enough rocks! Need %d, have %d\n", cost, rocks_mined);
    }
    else
    {
        printf("Oxygen already at max level!\n");
    }
}

void SanityUpgrade()
{
    int cost = GetSanityUpgradeCost();

    if (rocks_mined >= cost && sanity_upgrade_level < 3)
    {
        rocks_mined -= cost;
        sanity_upgrade_level++;
        printf("Sanity upgraded to level %d! (Cost: %d rocks)\n", sanity_upgrade_level, cost);
    }
    else if (rocks_mined < cost)
    {
        printf("Not enough rocks! Need %d, have %d\n", cost, rocks_mined);
    }
    else
    {
        printf("Sanity already at max level!\n");
    }
}

void TorchUpgrade()
{
    int cost = GetTorchUpgradeCost();

    if (rocks_mined >= cost && torch_upgrade_level < 3)
    {
        rocks_mined -= cost;
        torch_upgrade_level++;
        printf("Torch upgraded to level %d! (Cost: %d rocks)\n", torch_upgrade_level, cost);
    }
    else if (rocks_mined < cost)
    {
        printf("Not enough rocks! Need %d, have %d\n", cost, rocks_mined);
    }
    else
    {
        printf("Torch already at max level!\n");
    }
}

// Helper function to check if mouse is inside a box
int IsMouseInBox(float mouse_x, float mouse_y, float box_x, float box_y, float box_width, float box_height)
{
    float half_width = box_width / 2.0f;
    float half_height = box_height / 2.0f;

    return (mouse_x >= box_x - half_width && mouse_x <= box_x + half_width &&
        mouse_y >= box_y - half_height && mouse_y <= box_y + half_height);
}

// ---------------------------------------------------------------------------
// SHOP STATE FUNCTIONS
// ---------------------------------------------------------------------------

void Shop_Load()
{
    shopBackgroundTexture = AEGfxTextureLoad("../Assets/shopui.png");
    shopBackgroundImageMesh = CreateShopBackgroundImageMesh(shop_popup_width, shop_popup_height);
    upgradeBoxMesh = CreateUpgradeBoxMesh(upgrade_box_width, upgrade_box_height);
    upgradeBoxBorderMesh = CreateUpgradeBoxBorderMesh(upgrade_box_width, upgrade_box_height, 3.0f);
    shop_font_id = AEGfxCreateFont("../Assets/fonts/liberation-mono.ttf", 24);
    printf("Shop state loaded!\n");
}

void Shop_Initialize()
{
    shop_is_active = 1;
    printf("Shop state initialized!\n");
}

void Shop_Update()
{
    s32 mouse_screen_x, mouse_screen_y;
    AEInputGetCursorPosition(&mouse_screen_x, &mouse_screen_y);

    float mouse_x = (float)mouse_screen_x - 800.0f;
    float mouse_y = 450.0f - (float)mouse_screen_y;

    float total_width = (upgrade_box_width * 4) + (upgrade_box_spacing * 3);
    float start_x = -total_width / 2.0f + upgrade_box_width / 2.0f;
    float box_y = -150.0f;

    float box1_x = start_x;
    float box2_x = start_x + (upgrade_box_width + upgrade_box_spacing);
    float box3_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 2;
    float box4_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 3;

    hover_box1 = IsMouseInBox(mouse_x, mouse_y, box1_x, box_y, upgrade_box_width, upgrade_box_height);
    hover_box2 = IsMouseInBox(mouse_x, mouse_y, box2_x, box_y, upgrade_box_width, upgrade_box_height);
    hover_box3 = IsMouseInBox(mouse_x, mouse_y, box3_x, box_y, upgrade_box_width, upgrade_box_height);
    hover_box4 = IsMouseInBox(mouse_x, mouse_y, box4_x, box_y, upgrade_box_width, upgrade_box_height);

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (hover_box1) PickUpgrade();
        else if (hover_box2) OxygenUpgrade();
        else if (hover_box3) SanityUpgrade();
        else if (hover_box4) TorchUpgrade();
    }
}

void Shop_Draw()
{
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Draw background
    if (shopBackgroundTexture && shopBackgroundImageMesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(shopBackgroundTexture, 0, 0);

        AEMtx33 scale, rotate, translate, transform;
        AEMtx33Scale(&scale, 1.0f, 1.0f);
        AEMtx33Rot(&rotate, 0.0f);
        AEMtx33Trans(&translate, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &rotate, &scale);
        AEMtx33Concat(&transform, &translate, &transform);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(shopBackgroundImageMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Calculate positions
    float total_width = (upgrade_box_width * 4) + (upgrade_box_spacing * 3);
    float start_x = -total_width / 2.0f + upgrade_box_width / 2.0f;
    float box_y = -150.0f;

    float box1_x = start_x;
    float box2_x = start_x + (upgrade_box_width + upgrade_box_spacing);
    float box3_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 2;
    float box4_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 3;

    // Get costs for color coding
    int pick_cost = GetPickUpgradeCost();
    int oxy_cost = GetOxygenUpgradeCost();
    int sanity_cost = GetSanityUpgradeCost();
    int torch_cost = GetTorchUpgradeCost();

    // Draw borders
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEMtx33 scale2, rotate2, translate2, transform2;
    AEMtx33Scale(&scale2, 1.0f, 1.0f);
    AEMtx33Rot(&rotate2, 0.0f);

    // Border 1 - Green if can afford, Red if cannot
    if (hover_box1)
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    else if (rocks_mined >= pick_cost && pick_upgrade_level < 4)
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow (can afford)
    else
        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);  // Red (cannot afford)
    AEMtx33Trans(&translate2, box1_x, box_y);
    AEMtx33Concat(&transform2, &rotate2, &scale2);
    AEMtx33Concat(&transform2, &translate2, &transform2);
    AEGfxSetTransform(transform2.m);
    AEGfxMeshDraw(upgradeBoxBorderMesh, AE_GFX_MDM_TRIANGLES);

    // Border 2
    if (hover_box2)
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    else if (rocks_mined >= oxy_cost && oxygen_upgrade_level < 4)
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
    else
        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
    AEMtx33Trans(&translate2, box2_x, box_y);
    AEMtx33Concat(&transform2, &rotate2, &scale2);
    AEMtx33Concat(&transform2, &translate2, &transform2);
    AEGfxSetTransform(transform2.m);
    AEGfxMeshDraw(upgradeBoxBorderMesh, AE_GFX_MDM_TRIANGLES);

    // Border 3
    if (hover_box3)
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    else if (rocks_mined >= sanity_cost && sanity_upgrade_level < 3)
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
    else
        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
    AEMtx33Trans(&translate2, box3_x, box_y);
    AEMtx33Concat(&transform2, &rotate2, &scale2);
    AEMtx33Concat(&transform2, &translate2, &transform2);
    AEGfxSetTransform(transform2.m);
    AEGfxMeshDraw(upgradeBoxBorderMesh, AE_GFX_MDM_TRIANGLES);

    // Border 4
    if (hover_box4)
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    else if (rocks_mined >= torch_cost && torch_upgrade_level < 3)
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
    else
        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
    AEMtx33Trans(&translate2, box4_x, box_y);
    AEMtx33Concat(&transform2, &rotate2, &scale2);
    AEMtx33Concat(&transform2, &translate2, &transform2);
    AEGfxSetTransform(transform2.m);
    AEGfxMeshDraw(upgradeBoxBorderMesh, AE_GFX_MDM_TRIANGLES);

    // Display levels and costs
    if (shop_font_id >= 0)
    {
        // Display current rock count at top
        char rocks_text[64];
        sprintf_s(rocks_text, 64, "Rocks: %d", rocks_mined);
        AEGfxPrint(shop_font_id, rocks_text, -0.2f, -0.85f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        char level_text[64];

        // Box 1
        sprintf_s(level_text, 64, "Lv %d (Cost:%d)", pick_upgrade_level, pick_cost);
        AEGfxPrint(shop_font_id, level_text, -0.75f, -0.33f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

        // Box 2
        sprintf_s(level_text, 64, "Lv %d (Cost:%d)", oxygen_upgrade_level, oxy_cost);
        AEGfxPrint(shop_font_id, level_text, -0.32f, -0.33f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

        // Box 3
        sprintf_s(level_text, 64, "Lv %d (Cost:%d)", sanity_upgrade_level, sanity_cost);
        AEGfxPrint(shop_font_id, level_text, 0.10f, -0.33f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

        // Box 4
        sprintf_s(level_text, 64, "Lv %d (Cost:%d)", torch_upgrade_level, torch_cost);
        AEGfxPrint(shop_font_id, level_text, 0.53f, -0.33f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void Shop_Free()
{
    shop_is_active = 0;
    printf("Shop state freed!\n");
}

void Shop_Unload()
{
    if (shopBackgroundImageMesh) AEGfxMeshFree(shopBackgroundImageMesh);
    if (upgradeBoxMesh) AEGfxMeshFree(upgradeBoxMesh);
    if (upgradeBoxBorderMesh) AEGfxMeshFree(upgradeBoxBorderMesh);
    if (shopBackgroundTexture) AEGfxTextureUnload(shopBackgroundTexture);
    if (shop_font_id >= 0) AEGfxDestroyFont(shop_font_id);
    printf("Shop state unloaded!\n");
}
