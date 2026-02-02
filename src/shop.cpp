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
// UPGRADE FUNCTIONS
// ---------------------------------------------------------------------------

void PickUpgrade()
{
    pick_upgrade_level++;
    printf("Pick/Shovel upgraded to level %d!\n", pick_upgrade_level);
}

void OxygenUpgrade()
{
    oxygen_upgrade_level++;
    printf("Oxygen upgraded to level %d!\n", oxygen_upgrade_level);
}

void SanityUpgrade()
{
    sanity_upgrade_level++;
    printf("Sanity upgraded to level %d!\n", sanity_upgrade_level);
}

void TorchUpgrade()
{
    torch_upgrade_level++;
    printf("Torch/Light upgraded to level %d!\n", torch_upgrade_level);
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

    // Draw borders
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEMtx33 scale2, rotate2, translate2, transform2;
    AEMtx33Scale(&scale2, 1.0f, 1.0f);
    AEMtx33Rot(&rotate2, 0.0f);

    // Border 1
    if (hover_box1) AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    else AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
    AEMtx33Trans(&translate2, box1_x, box_y);
    AEMtx33Concat(&transform2, &rotate2, &scale2);
    AEMtx33Concat(&transform2, &translate2, &transform2);
    AEGfxSetTransform(transform2.m);
    AEGfxMeshDraw(upgradeBoxBorderMesh, AE_GFX_MDM_TRIANGLES);

    // Border 2
    if (hover_box2) AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    else AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
    AEMtx33Trans(&translate2, box2_x, box_y);
    AEMtx33Concat(&transform2, &rotate2, &scale2);
    AEMtx33Concat(&transform2, &translate2, &transform2);
    AEGfxSetTransform(transform2.m);
    AEGfxMeshDraw(upgradeBoxBorderMesh, AE_GFX_MDM_TRIANGLES);

    // Border 3
    if (hover_box3) AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    else AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
    AEMtx33Trans(&translate2, box3_x, box_y);
    AEMtx33Concat(&transform2, &rotate2, &scale2);
    AEMtx33Concat(&transform2, &translate2, &transform2);
    AEGfxSetTransform(transform2.m);
    AEGfxMeshDraw(upgradeBoxBorderMesh, AE_GFX_MDM_TRIANGLES);

    // Border 4
    if (hover_box4) AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    else AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
    AEMtx33Trans(&translate2, box4_x, box_y);
    AEMtx33Concat(&transform2, &rotate2, &scale2);
    AEMtx33Concat(&transform2, &translate2, &transform2);
    AEGfxSetTransform(transform2.m);
    AEGfxMeshDraw(upgradeBoxBorderMesh, AE_GFX_MDM_TRIANGLES);

    // Display levels
    if (shop_font_id >= 0)
    {
        char level_text[32];
        sprintf_s(level_text, 32, "Lv %d", pick_upgrade_level);
        AEGfxPrint(shop_font_id, level_text, -0.67f, -0.22f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);

        sprintf_s(level_text, 32, "Lv %d", oxygen_upgrade_level);
        AEGfxPrint(shop_font_id, level_text, -0.24f, -0.22f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);

        sprintf_s(level_text, 32, "Lv %d", sanity_upgrade_level);
        AEGfxPrint(shop_font_id, level_text, 0.18f, -0.22, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);

        sprintf_s(level_text, 32, "Lv %d", torch_upgrade_level);
        AEGfxPrint(shop_font_id, level_text, 0.61f, -0.22, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
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