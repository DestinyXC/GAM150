// ---------------------------------------------------------------------------
// GroundBreaker-Style Mining Game
// 256x192 Spritesheet (16x16 tiles), Vertical Camera, 2 Tiles
// ---------------------------------------------------------------------------

#include <crtdbg.h>
#include "AEEngine.h"
#include <stdio.h>
#include <time.h>
#include "shop.hpp"

// ---------------------------------------------------------------------------
// CONFIGURATION

#define MAP_WIDTH 17       // Fixed screen width in tiles (25 * 64 = 1600)
#define MAP_HEIGHT 100        // Deep vertical map
#define TILE_SIZE 64.0f

// Spritesheet settings (256x192, 16x16 tiles)
#define SPRITESHEET_WIDTH 256
#define SPRITESHEET_HEIGHT 192
#define SPRITE_SIZE 16
#define SPRITESHEET_COLUMNS 16    // 256 / 16 = 16
#define SPRITESHEET_ROWS 12       // 192 / 16 = 12

// Screen
#define SCREEN_WIDTH 1600.0f
#define SCREEN_HEIGHT 900.0f

//Oxygen icon dimension
float oxygeniconwidth = 100.0f;
float oxygeniconheight = 100.0f;

//Sanity icon dimension
float sanityiconwidth = 100.0f;
float sanityiconheight = 100.0f;

//Boulder icon dimension
float bouldericonwidth = 100.0f;
float bouldericonheight = 100.0f;

//Map icon dimension
float mapiconwidth = 500.0f;
float mapiconheight = 200.0f;

// Oxygen System
float oxygen_percentage = 100.0f;
float oxygen_max = 100.0f;
float oxygen_drain_rate = 2.0f;  // Per second outside safezone
float oxygen_refill_rate = 2.0f; // Per second inside safezone
int oxygen_upgrade_level = 0;    // For future upgrades

// Safe Zone
float safezone_x_min = -400.0f;
float safezone_x_max = 400.0f;
float safezone_y_min = -3200.0f;
float safezone_y_max = -2600.0f;

//Font
s8 g_font_id = -1;


// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Shop System
int shop_is_open = 0;
int player_in_shop_zone = 0;
float shop_trigger_x = 325.0f;
float shop_trigger_y = -2675.0f;
float shop_trigger_width = 150.0f;
float shop_trigger_height = 150.0f;
//
//// Shop popup dimensions
//float shop_popup_width = 1600.0f;
//float shop_popup_height = 900.0f;  // Increased for title
//
//// Shop upgrade boxes (4 boxes)
//float upgrade_box_width = 180.0f;
//float upgrade_box_height = 180.0f;  // Square boxes
//float upgrade_box_spacing = 50.0f;
//float upgrade_border_thickness = 3.0f;
//
//// Title position
//float shop_title_y = 300.0f;
//
//// Label positions (below each box)
//float label_offset_y = -150.0f;
//
//
//


// --------------------------------------------------------------------------
// TILE TYPES

enum TileType
{
    TILE_EMPTY = -1,
    TILE_DIRT = 0,     // Your first tile (adjust position)
    TILE_STONE = 1,     // Your second tile (adjust position)
    TILE_WALL = 2
};

// SET THESE TO MATCH YOUR SPRITESHEET POSITIONS (0-191)
// Formula: position = (row * 16) + column
// Example: Row 2, Column 5 = (2 * 16) + 5 = 37
#define DIRT_SPRITE_POSITION 41    // Change to your dirt tile position
#define STONE_SPRITE_POSITION 191   // Change to your stone tile position
#define WALL_SPRITE_POSITION 22    // Change to your wall tile position

// ---------------------------------------------------------------------------
// GLOBALS

int tileMap[MAP_HEIGHT][MAP_WIDTH];

// Camera (vertical only)
float camera_x = 0.0f;
float camera_y = 0.0f;
float camera_smoothness = 0.15f;

// Player
float player_x = 0.0f;
float player_y = 0.0f;
float player_width = 48.0f;
float player_height = 48.0f;
float player_speed = 5.0f;

// Mining
float mining_range = 120.0f;
int currently_mining_row = -1;
int currently_mining_col = -1;

// Stats
int depth = 0;
int max_depth = 0;

// ---------------------------------------------------------------------------
// UV MAPPING

void GetTileUV(int sprite_position, float* u_min, float* v_min, float* u_max, float* v_max)
{
    if (sprite_position < 0)
    {
        *u_min = *v_min = *u_max = *v_max = 0.0f;
        return;
    }

    int col = sprite_position % SPRITESHEET_COLUMNS;
    int row = sprite_position / SPRITESHEET_COLUMNS;

    float tile_width_uv = (float)SPRITE_SIZE / (float)SPRITESHEET_WIDTH;
    float tile_height_uv = (float)SPRITE_SIZE / (float)SPRITESHEET_HEIGHT;

    *u_min = col * tile_width_uv;
    *v_min = row * tile_height_uv;
    *u_max = *u_min + tile_width_uv;
    *v_max = *v_min + tile_height_uv;
}

// ---------------------------------------------------------------------------
// MESH CREATION

AEGfxVertexList* CreateSpritesheetTileMesh(int sprite_position, float size)
{
    float u_min, v_min, u_max, v_max;
    GetTileUV(sprite_position, &u_min, &v_min, &u_max, &v_max);

    AEGfxMeshStart();

    float half = size / 2.0f;

    AEGfxTriAdd(
        -half, -half, 0xFFFFFFFF, u_min, v_max,
        half, -half, 0xFFFFFFFF, u_max, v_max,
        -half, half, 0xFFFFFFFF, u_min, v_min);

    AEGfxTriAdd(
        half, -half, 0xFFFFFFFF, u_max, v_max,
        half, half, 0xFFFFFFFF, u_max, v_min,
        -half, half, 0xFFFFFFFF, u_min, v_min);

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateRectangleMesh(float width, float height, unsigned int color)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    AEGfxTriAdd(
        -half_width, -half_height, color, 0.0f, 1.0f,
        half_width, -half_height, color, 1.0f, 1.0f,
        -half_width, half_height, color, 0.0f, 0.0f);

    AEGfxTriAdd(
        half_width, -half_height, color, 1.0f, 1.0f,
        half_width, half_height, color, 1.0f, 0.0f,
        -half_width, half_height, color, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateSideBlackoutMesh(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    AEGfxTriAdd(
        -half_width, -half_height, 0xFF000000, 0.0f, 1.0f,
        half_width, -half_height, 0xFF000000, 1.0f, 1.0f,
        -half_width, half_height, 0xFF000000, 0.0f, 0.0f);

    AEGfxTriAdd(
        half_width, -half_height, 0xFF000000, 1.0f, 1.0f,
        half_width, half_height, 0xFF000000, 1.0f, 0.0f,
        -half_width, half_height, 0xFF000000, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

AEGfxVertexList* Createoxygenicon(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    // Create a textured quad for the bottom image
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

AEGfxVertexList* Createsanityicon(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    // Create a textured quad for the bottom image
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

AEGfxVertexList* Createbouldericon(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    // Create a textured quad for the bottom image
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

AEGfxVertexList* Createmapicon(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    // Create a textured quad for the bottom image
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

AEGfxVertexList* CreateSafezoneBorder(float width, float height, float thickness)
{
    AEGfxMeshStart();

    float hw = width / 2.0f;   // half width
    float hh = height / 2.0f;  // half height
    float t = thickness;       // border thickness

    // Top border (horizontal bar)
    AEGfxTriAdd(
        -hw, hh - t, 0xFF00FF00, 0.0f, 0.0f,
        hw, hh - t, 0xFF00FF00, 1.0f, 0.0f,
        -hw, hh, 0xFF00FF00, 0.0f, 1.0f);
    AEGfxTriAdd(
        hw, hh - t, 0xFF00FF00, 1.0f, 0.0f,
        hw, hh, 0xFF00FF00, 1.0f, 1.0f,
        -hw, hh, 0xFF00FF00, 0.0f, 1.0f);

    // Bottom border (horizontal bar)
    AEGfxTriAdd(
        -hw, -hh, 0xFF00FF00, 0.0f, 0.0f,
        hw, -hh, 0xFF00FF00, 1.0f, 0.0f,
        -hw, -hh + t, 0xFF00FF00, 0.0f, 1.0f);
    AEGfxTriAdd(
        hw, -hh, 0xFF00FF00, 1.0f, 0.0f,
        hw, -hh + t, 0xFF00FF00, 1.0f, 1.0f,
        -hw, -hh + t, 0xFF00FF00, 0.0f, 1.0f);

    // Left border (vertical bar)
    AEGfxTriAdd(
        -hw, -hh, 0xFF00FF00, 0.0f, 0.0f,
        -hw + t, -hh, 0xFF00FF00, 1.0f, 0.0f,
        -hw, hh, 0xFF00FF00, 0.0f, 1.0f);
    AEGfxTriAdd(
        -hw + t, -hh, 0xFF00FF00, 1.0f, 0.0f,
        -hw + t, hh, 0xFF00FF00, 1.0f, 1.0f,
        -hw, hh, 0xFF00FF00, 0.0f, 1.0f);

    // Right border (vertical bar)
    AEGfxTriAdd(
        hw - t, -hh, 0xFF00FF00, 0.0f, 0.0f,
        hw, -hh, 0xFF00FF00, 1.0f, 0.0f,
        hw - t, hh, 0xFF00FF00, 0.0f, 1.0f);
    AEGfxTriAdd(
        hw, -hh, 0xFF00FF00, 1.0f, 0.0f,
        hw, hh, 0xFF00FF00, 1.0f, 1.0f,
        hw - t, hh, 0xFF00FF00, 0.0f, 1.0f);

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateShopTriggerMesh(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

    // Yellow trigger box
    AEGfxTriAdd(
        -half_width, -half_height, 0xFFFFFF00, 0.0f, 1.0f,
        half_width, -half_height, 0xFFFFFF00, 1.0f, 1.0f,
        -half_width, half_height, 0xFFFFFF00, 0.0f, 0.0f);

    AEGfxTriAdd(
        half_width, -half_height, 0xFFFFFF00, 1.0f, 1.0f,
        half_width, half_height, 0xFFFFFF00, 1.0f, 0.0f,
        -half_width, half_height, 0xFFFFFF00, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}
//
//AEGfxVertexList* CreateShopBackgroundMesh(float width, float height)
//{
//    AEGfxMeshStart();
//
//    float half_width = width / 2.0f;
//    float half_height = height / 2.0f;
//
//    // Semi-transparent dark overlay
//    AEGfxTriAdd(
//        -half_width, -half_height, 0xDD000000, 0.0f, 1.0f,
//        half_width, -half_height, 0xDD000000, 1.0f, 1.0f,
//        -half_width, half_height, 0xDD000000, 0.0f, 0.0f);
//
//    AEGfxTriAdd(
//        half_width, -half_height, 0xDD000000, 1.0f, 1.0f,
//        half_width, half_height, 0xDD000000, 1.0f, 0.0f,
//        -half_width, half_height, 0xDD000000, 0.0f, 0.0f);
//
//    return AEGfxMeshEnd();
//}
//
//AEGfxVertexList* CreateUpgradeBoxMesh(float width, float height)
//{
//    AEGfxMeshStart();
//
//    float half_width = width / 2.0f;
//    float half_height = height / 2.0f;
//
//    // Textured box for upgrade images
//    AEGfxTriAdd(
//        -half_width, -half_height, 0xFFFFFFFF, 0.0f, 1.0f,
//        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
//        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);
//
//    AEGfxTriAdd(
//        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
//        half_width, half_height, 0xFFFFFFFF, 1.0f, 0.0f,
//        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);
//
//    return AEGfxMeshEnd();
//}
//
//AEGfxVertexList* CreateUpgradeBoxBorderMesh(float width, float height, float thickness)
//{
//    AEGfxMeshStart();
//
//    float hw = width / 2.0f;   // half width
//    float hh = height / 2.0f;  // half height
//    float t = thickness;       // border thickness
//
//    // Top border (horizontal bar)
//    AEGfxTriAdd(
//        -hw, hh - t, 0xFFFFFFFF, 0.0f, 0.0f,
//        hw, hh - t, 0xFFFFFFFF, 1.0f, 0.0f,
//        -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);
//    AEGfxTriAdd(
//        hw, hh - t, 0xFFFFFFFF, 1.0f, 0.0f,
//        hw, hh, 0xFFFFFFFF, 1.0f, 1.0f,
//        -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);
//
//    // Bottom border (horizontal bar)
//    AEGfxTriAdd(
//        -hw, -hh, 0xFFFFFFFF, 0.0f, 0.0f,
//        hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f,
//        -hw, -hh + t, 0xFFFFFFFF, 0.0f, 1.0f);
//    AEGfxTriAdd(
//        hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f,
//        hw, -hh + t, 0xFFFFFFFF, 1.0f, 1.0f,
//        -hw, -hh + t, 0xFFFFFFFF, 0.0f, 1.0f);
//
//    // Left border (vertical bar)
//    AEGfxTriAdd(
//        -hw, -hh, 0xFFFFFFFF, 0.0f, 0.0f,
//        -hw + t, -hh, 0xFFFFFFFF, 1.0f, 0.0f,
//        -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);
//    AEGfxTriAdd(
//        -hw + t, -hh, 0xFFFFFFFF, 1.0f, 0.0f,
//        -hw + t, hh, 0xFFFFFFFF, 1.0f, 1.0f,
//        -hw, hh, 0xFFFFFFFF, 0.0f, 1.0f);
//
//    // Right border (vertical bar)
//    AEGfxTriAdd(
//        hw - t, -hh, 0xFFFFFFFF, 0.0f, 0.0f,
//        hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f,
//        hw - t, hh, 0xFFFFFFFF, 0.0f, 1.0f);
//    AEGfxTriAdd(
//        hw, -hh, 0xFFFFFFFF, 1.0f, 0.0f,
//        hw, hh, 0xFFFFFFFF, 1.0f, 1.0f,
//        hw - t, hh, 0xFFFFFFFF, 0.0f, 1.0f);
//
//    return AEGfxMeshEnd();
//}
//
//AEGfxVertexList* CreateShopBackgroundImageMesh(float width, float height)
//{
//    AEGfxMeshStart();
//
//    float half_width = width / 2.0f;
//    float half_height = height / 2.0f;
//
//    // Textured quad for background image
//    AEGfxTriAdd(
//        -half_width, -half_height, 0xFFFFFFFF, 0.0f, 1.0f,
//        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
//        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);
//
//    AEGfxTriAdd(
//        half_width, -half_height, 0xFFFFFFFF, 1.0f, 1.0f,
//        half_width, half_height, 0xFFFFFFFF, 1.0f, 0.0f,
//        -half_width, half_height, 0xFFFFFFFF, 0.0f, 0.0f);
//
//    return AEGfxMeshEnd();
//}





// ---------------------------------------------------------------------------
// WORLD GENERATION

void GenerateWorld()
{
    srand((unsigned int)time(NULL));

    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            // Surface area (rows 0-5) - empty (sky)
            if (row < 5)
            {
                tileMap[row][col] = TILE_DIRT;
            }
            // Shallow dirt layer (rows 5-30)
            else if (row < 20)
            {
                tileMap[row][col] = TILE_DIRT;
            }
            // Deep stone layer (rows 30+)
            else
            {
                tileMap[row][col] = TILE_STONE;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// WORLD HELPERS

void GetTileWorldPosition(int row, int col, float* out_x, float* out_y)
{
    *out_x = (col * TILE_SIZE) - (MAP_WIDTH * TILE_SIZE / 2.0f) + (TILE_SIZE / 2.0f);
    *out_y = (row * TILE_SIZE) - (MAP_HEIGHT * TILE_SIZE / 2.0f) + (TILE_SIZE / 2.0f);
}

void GetTileAtPosition(float world_x, float world_y, int* out_row, int* out_col)
{
    float offset_x = world_x + (MAP_WIDTH * TILE_SIZE / 2.0f);
    float offset_y = world_y + (MAP_HEIGHT * TILE_SIZE / 2.0f);

    *out_col = (int)(offset_x / TILE_SIZE);
    *out_row = (int)(offset_y / TILE_SIZE);
}

int IsTileValid(int row, int col)
{
    return (row >= 0 && row < MAP_HEIGHT && col >= 0 && col < MAP_WIDTH);
}

int IsTileSolid(int tile_type)
{
    return (tile_type == TILE_DIRT || tile_type == TILE_STONE);
}

// ---------------------------------------------------------------------------
// COLLISION

int CheckCollisionRectangle(float x1, float y1, float width1, float height1,
    float x2, float y2, float width2, float height2)
{
    float half_width1 = width1 / 2.0f;
    float half_height1 = height1 / 2.0f;
    float half_width2 = width2 / 2.0f;
    float half_height2 = height2 / 2.0f;

    return (x1 + half_width1 > x2 - half_width2 &&
        x1 - half_width1 < x2 + half_width2 &&
        y1 + half_height1 > y2 - half_height2 &&
        y1 - half_height1 < y2 + half_height2);
}

// ---------------------------------------------------------------------------
// PHYSICS (GroundBreaker style - no gravity, 4-directional)

void UpdatePhysics(float dt)
{
    // 4-directional movement
    float move_x = 0.0f;
    float move_y = 0.0f;

    if (AEInputCheckCurr(AEVK_A)) move_x -= player_speed;
    if (AEInputCheckCurr(AEVK_D)) move_x += player_speed;
    if (AEInputCheckCurr(AEVK_W)) move_y += player_speed;
    if (AEInputCheckCurr(AEVK_S)) move_y -= player_speed;

    // Normalize diagonal movement
    if (move_x != 0.0f && move_y != 0.0f)
    {
        float factor = 0.707f;
        move_x *= factor;
        move_y *= factor;
    }

    // Apply movement
    player_x += move_x;
    player_y += move_y;

    // Keep player within horizontal bounds
    float world_width = MAP_WIDTH * TILE_SIZE;
    float min_x = -world_width / 2.0f + player_width / 2.0f;
    float max_x = world_width / 2.0f - player_width / 2.0f;

    if (player_x < min_x) player_x = min_x;
    if (player_x > max_x) player_x = max_x;

    // Collision with tiles
    //int player_row, player_col;
    //GetTileAtPosition(player_x, player_y, &player_row, &player_col);

    //for (int dr = -2; dr <= 2; dr++)
    //{
    //    for (int dc = -2; dc <= 2; dc++)
    //    {
    //        int check_row = player_row + dr;
    //        int check_col = player_col + dc;

    //        if (!IsTileValid(check_row, check_col)) continue;

    //        int tile_type = tileMap[check_row][check_col];

    //        if (IsTileSolid(tile_type))
    //        {
    //            float tile_x, tile_y;
    //            GetTileWorldPosition(check_row, check_col, &tile_x, &tile_y);

    //            if (CheckCollisionRectangle(player_x, player_y, player_width, player_height,
    //                tile_x, tile_y, TILE_SIZE, TILE_SIZE))
    //            {
    //                float overlap_x = (player_width / 2 + TILE_SIZE / 2) - fabsf(player_x - tile_x);
    //                float overlap_y = (player_height / 2 + TILE_SIZE / 2) - fabsf(player_y - tile_y);

    //                if (overlap_x < overlap_y)
    //                {
    //                    if (player_x < tile_x)
    //                        player_x -= overlap_x;
    //                    else
    //                        player_x += overlap_x;
    //                }
    //                else
    //                {
    //                    if (player_y < tile_y)
    //                        player_y -= overlap_y;
    //                    else
    //                        player_y += overlap_y;
    //                }
    //            }
    //        }
    //    }
    //}

    //// Update depth
    //GetTileAtPosition(player_x, player_y, &player_row, &player_col);
    //depth = player_row - 5;  // Surface is row 5
    //if (depth < 0) depth = 0;
    //if (depth > max_depth) max_depth = depth;
}

// ---------------------------------------------------------------------------
// MINING SYSTEM

void UpdateMining(float dt)
{
    // Get mouse position in world space
    s32 mouse_screen_x, mouse_screen_y;
    AEInputGetCursorPosition(&mouse_screen_x, &mouse_screen_y);

    float mouse_world_x = (float)mouse_screen_x - SCREEN_WIDTH / 2 + camera_x;
    float mouse_world_y = SCREEN_HEIGHT / 2 - (float)mouse_screen_y + camera_y;

    int target_row, target_col;
    GetTileAtPosition(mouse_world_x, mouse_world_y, &target_row, &target_col);

    // Check if can mine
    int can_mine = 0;
    if (IsTileValid(target_row, target_col))
    {
        float tile_x, tile_y;
        GetTileWorldPosition(target_row, target_col, &tile_x, &tile_y);

        float dist = sqrtf((tile_x - player_x) * (tile_x - player_x) +
            (tile_y - player_y) * (tile_y - player_y));

        int tile_type = tileMap[target_row][target_col];

        if (dist <= mining_range && IsTileSolid(tile_type))
        {
            can_mine = 1;
        }
    }

    // Mine on click
    if (can_mine && AEInputCheckCurr(AEVK_LBUTTON))
    {
        currently_mining_row = target_row;
        currently_mining_col = target_col;

        // Instant mining (you can add mining time later)
        tileMap[target_row][target_col] = TILE_EMPTY;
    }
    else
    {
        currently_mining_row = -1;
        currently_mining_col = -1;
    }
}

// ---------------------------------------------------------------------------
// CAMERA (Vertical only, like GroundBreaker)

void UpdateCamera(float dt)
{
    // Follow player vertically only
    float target_y = player_y;

    camera_y += (target_y - camera_y) * camera_smoothness;

    // Camera stays centered horizontally
    camera_x = 0.0f;

    // Vertical bounds
    float world_height = MAP_HEIGHT * TILE_SIZE;
    float min_y = -world_height / 2.0f + SCREEN_HEIGHT / 2.0f;
    float max_y = world_height / 2.0f - SCREEN_HEIGHT / 2.0f;

    if (camera_y < min_y) camera_y = min_y;
    if (camera_y > max_y) camera_y = max_y;
}
// ---------------------------------------------------------------------------
// OXYGEN SYSTEM
void UpdateOxygenSystem(float dt)
{
    // Check if player is in safezone
    int in_safezone = (player_x >= safezone_x_min && player_x <= safezone_x_max &&
        player_y >= safezone_y_min && player_y <= safezone_y_max);

    // Upgrade-based oxygen system using switch-case
    switch (oxygen_upgrade_level)
    {
    case 0: // Default oxygen system
        if (in_safezone)
        {
            // Refill oxygen in safezone
            oxygen_percentage += oxygen_refill_rate * dt;
            if (oxygen_percentage > oxygen_max)
                oxygen_percentage = oxygen_max;
        }
        else
        {
            // Drain oxygen outside safezone
            oxygen_percentage -= oxygen_drain_rate * dt;
            if (oxygen_percentage < 0.0f)
                oxygen_percentage = 0.0f;
        }
        break;

    case 1: // Upgrade Level 1: Slower drain
        if (in_safezone)
        {
            oxygen_percentage += oxygen_refill_rate * dt;
            if (oxygen_percentage > oxygen_max)
                oxygen_percentage = oxygen_max;
        }
        else
        {
            // 50% slower drain
            oxygen_percentage -= (oxygen_drain_rate * 0.5f) * dt;
            if (oxygen_percentage < 0.0f)
                oxygen_percentage = 0.0f;
        }
        break;

    case 2: // Upgrade Level 2: Faster refill + slower drain
        if (in_safezone)
        {
            // 2x faster refill
            oxygen_percentage += (oxygen_refill_rate * 2.0f) * dt;
            if (oxygen_percentage > oxygen_max)
                oxygen_percentage = oxygen_max;
        }
        else
        {
            // 50% slower drain
            oxygen_percentage -= (oxygen_drain_rate * 0.5f) * dt;
            if (oxygen_percentage < 0.0f)
                oxygen_percentage = 0.0f;
        }
        break;

    case 3: // Upgrade Level 3: Extended max oxygen
        oxygen_max = 150.0f; // Increased max capacity

        if (in_safezone)
        {
            oxygen_percentage += (oxygen_refill_rate * 2.0f) * dt;
            if (oxygen_percentage > oxygen_max)
                oxygen_percentage = oxygen_max;
        }
        else
        {
            oxygen_percentage -= (oxygen_drain_rate * 0.5f) * dt;
            if (oxygen_percentage < 0.0f)
                oxygen_percentage = 0.0f;
        }
        break;

    case 4: // Upgrade Level 4: No oxygen drain
        if (in_safezone)
        {
            oxygen_percentage += oxygen_refill_rate * dt;
            if (oxygen_percentage > oxygen_max)
                oxygen_percentage = oxygen_max;
        }
        // No drain outside safezone
        break;

    default:
        // Fallback to case 0
        if (in_safezone)
        {
            oxygen_percentage += oxygen_refill_rate * dt;
            if (oxygen_percentage > oxygen_max)
                oxygen_percentage = oxygen_max;
        }
        else
        {
            oxygen_percentage -= oxygen_drain_rate * dt;
            if (oxygen_percentage < 0.0f)
                oxygen_percentage = 0.0f;
        }
        break;
    }

    // Player death when oxygen reaches 0
    if (oxygen_percentage <= 0.0f)
    {
        // Reset player to spawn position
        player_x = 0.0f;
        player_y = (5 * TILE_SIZE) - (MAP_HEIGHT * TILE_SIZE / 2.0f) - 100.0f;
        oxygen_percentage = 100.0f;
    }
}


// ---------------------------------------------------------------------------

// SHOP SYSTEM
void UpdateShopSystem(float dt)
{
    // Check if player is inside shop trigger box
    player_in_shop_zone = CheckCollisionRectangle(
        player_x, player_y, player_width, player_height,
        shop_trigger_x, shop_trigger_y, shop_trigger_width, shop_trigger_height
    );

    if (shop_is_open)
    {
        // Close shop with right click, ESC, or ENTER
        if (AEInputCheckTriggered(AEVK_RBUTTON) ||
            AEInputCheckTriggered(AEVK_ESCAPE) ||
            AEInputCheckTriggered(AEVK_RETURN))
        {
            shop_is_open = 0;
        }
    }
    else
    {
        // Only allow opening shop if player is inside the trigger box
        if (player_in_shop_zone)
        {
            // Open shop with left click or ENTER
            if (AEInputCheckTriggered(AEVK_LBUTTON) ||
                AEInputCheckTriggered(AEVK_RETURN))
            {
                shop_is_open = 1;
            }
        }
    }
}


// ---------------------------------------------------------------------------
// RENDERING

void RenderBackground(AEGfxTexture* tileset, AEGfxVertexList* dirtMesh, AEGfxVertexList* stoneMesh)
{
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(tileset, 0, 0);

    // Calculate visible tiles
    int start_col = 0;
    int end_col = MAP_WIDTH - 1;
    int start_row = (int)((camera_y - SCREEN_HEIGHT / 2 - TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;
    int end_row = (int)((camera_y + SCREEN_HEIGHT / 2 + TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;

    if (start_row < 0) start_row = 0;
    if (end_row >= MAP_HEIGHT) end_row = MAP_HEIGHT - 1;

    // Draw tiles
    for (int row = start_row; row <= end_row; row++)
    {
        for (int col = start_col; col <= end_col; col++)
        {
            int tile_type = tileMap[row][col];

            if (tile_type == TILE_EMPTY) continue;

            float tile_x, tile_y;
            GetTileWorldPosition(row, col, &tile_x, &tile_y);

            AEMtx33 scale, rotate, translate, transform;
            AEMtx33Scale(&scale, 1.0f, 1.0f);
            AEMtx33Rot(&rotate, 0.0f);
            AEMtx33Trans(&translate, tile_x, tile_y);
            AEMtx33Concat(&transform, &rotate, &scale);
            AEMtx33Concat(&transform, &translate, &transform);

            AEGfxSetTransform(transform.m);

            if (tile_type == TILE_DIRT)
            {
                AEGfxMeshDraw(dirtMesh, AE_GFX_MDM_TRIANGLES);
            }
            else if (tile_type == TILE_STONE)
            {
                AEGfxMeshDraw(stoneMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }
}

void RenderBackgroundFallback(AEGfxVertexList* dirtMesh, AEGfxVertexList* stoneMesh)
{
    // Fallback rendering without texture (colored rectangles)
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    int start_col = 0;
    int end_col = MAP_WIDTH - 1;
    int start_row = (int)((camera_y - SCREEN_HEIGHT / 2 - TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;
    int end_row = (int)((camera_y + SCREEN_HEIGHT / 2 + TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;

    if (start_row < 0) start_row = 0;
    if (end_row >= MAP_HEIGHT) end_row = MAP_HEIGHT - 1;

    for (int row = start_row; row <= end_row; row++)
    {
        for (int col = start_col; col <= end_col; col++)
        {
            int tile_type = tileMap[row][col];

            if (tile_type == TILE_EMPTY) continue;

            float tile_x, tile_y;
            GetTileWorldPosition(row, col, &tile_x, &tile_y);

            AEMtx33 scale, rotate, translate, transform;
            AEMtx33Scale(&scale, 1.0f, 1.0f);
            AEMtx33Rot(&rotate, 0.0f);
            AEMtx33Trans(&translate, tile_x, tile_y);
            AEMtx33Concat(&transform, &rotate, &scale);
            AEMtx33Concat(&transform, &translate, &transform);

            AEGfxSetTransform(transform.m);

            if (tile_type == TILE_DIRT)
            {
                AEGfxMeshDraw(dirtMesh, AE_GFX_MDM_TRIANGLES);
            }
            else if (tile_type == TILE_STONE)
            {
                AEGfxMeshDraw(stoneMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }
}

void RenderPlayer(AEGfxVertexList* playerMesh)
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, player_x, player_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(playerMesh, AE_GFX_MDM_TRIANGLES);
}

void RenderMiningCursor(AEGfxVertexList* cursorMesh)
{
    s32 mouse_screen_x, mouse_screen_y;
    AEInputGetCursorPosition(&mouse_screen_x, &mouse_screen_y);

    float mouse_world_x = (float)mouse_screen_x - SCREEN_WIDTH / 2 + camera_x;
    float mouse_world_y = SCREEN_HEIGHT / 2 - (float)mouse_screen_y + camera_y;

    int target_row, target_col;
    GetTileAtPosition(mouse_world_x, mouse_world_y, &target_row, &target_col);

    if (IsTileValid(target_row, target_col))
    {
        float tile_x, tile_y;
        GetTileWorldPosition(target_row, target_col, &tile_x, &tile_y);

        float dist = sqrtf((tile_x - player_x) * (tile_x - player_x) +
            (tile_y - player_y) * (tile_y - player_y));

        int tile_type = tileMap[target_row][target_col];

        if (dist <= mining_range && IsTileSolid(tile_type))
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            AEMtx33 scale, rotate, translate, transform;
            AEMtx33Scale(&scale, 1.1f, 1.1f);
            AEMtx33Rot(&rotate, 0.0f);
            AEMtx33Trans(&translate, tile_x, tile_y);
            AEMtx33Concat(&transform, &rotate, &scale);
            AEMtx33Concat(&transform, &translate, &transform);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(cursorMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
}

void RenderSideBlackout(AEGfxVertexList* leftMesh, AEGfxVertexList* rightMesh)
{
    // Reset camera to screen space
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    float playable_width = MAP_WIDTH * TILE_SIZE;
    float blackout_width = (SCREEN_WIDTH - playable_width) / 2.0f;

    // Draw left blackout bar
    AEMtx33 leftScale, leftRotate, leftTranslate, leftTransform;
    AEMtx33Scale(&leftScale, 1.0f, 1.0f);
    AEMtx33Rot(&leftRotate, 0.0f);
    AEMtx33Trans(&leftTranslate, -SCREEN_WIDTH / 2.0f + blackout_width / 2.0f, 0.0f);
    AEMtx33Concat(&leftTransform, &leftRotate, &leftScale);
    AEMtx33Concat(&leftTransform, &leftTranslate, &leftTransform);

    AEGfxSetTransform(leftTransform.m);
    AEGfxMeshDraw(leftMesh, AE_GFX_MDM_TRIANGLES);

    // Draw right blackout bar
    AEMtx33 rightScale, rightRotate, rightTranslate, rightTransform;
    AEMtx33Scale(&rightScale, 1.0f, 1.0f);
    AEMtx33Rot(&rightRotate, 0.0f);
    AEMtx33Trans(&rightTranslate, SCREEN_WIDTH / 2.0f - blackout_width / 2.0f, 0.0f);
    AEMtx33Concat(&rightTransform, &rightRotate, &rightScale);
    AEMtx33Concat(&rightTransform, &rightTranslate, &rightTransform);

    AEGfxSetTransform(rightTransform.m);
    AEGfxMeshDraw(rightMesh, AE_GFX_MDM_TRIANGLES);
}

void Renderoxygenicon(AEGfxTexture* texture, AEGfxVertexList* imageMesh, float width, float height)
{
    if (!texture || !imageMesh) return;

    // Reset camera to screen space (fixed position)
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(texture, 0, 0);

    // Position at bottom center of screen
    float image_x = 150.0f;
    float image_y = -350.0f;

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, image_x, image_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(imageMesh, AE_GFX_MDM_TRIANGLES);
}

void Rendersanityicon(AEGfxTexture* texture, AEGfxVertexList* imageMesh, float width, float height)
{
    if (!texture || !imageMesh) return;

    // Reset camera to screen space (fixed position)
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(texture, 0, 0);

    // Position at bottom center of screen
    float image_x = 450.0f;
    float image_y = -350.0f;

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, image_x, image_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(imageMesh, AE_GFX_MDM_TRIANGLES);
}

void Renderbouldericon(AEGfxTexture* texture, AEGfxVertexList* imageMesh, float width, float height)
{
    if (!texture || !imageMesh) return;

    // Reset camera to screen space (fixed position)
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(texture, 0, 0);

    // Position at bottom center of screen
    float image_x = 300.0f;
    float image_y = -350.0f;

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, image_x, image_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(imageMesh, AE_GFX_MDM_TRIANGLES);
}

void Rendermapicon(AEGfxTexture* texture, AEGfxVertexList* imageMesh, float width, float height)
{
    if (!texture || !imageMesh) return;

    // Reset camera to screen space (fixed position)
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(texture, 0, 0);

    // Position at bottom center of screen
    float image_x = 300.0f;
    float image_y = -350.0f;

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, image_x, image_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(imageMesh, AE_GFX_MDM_TRIANGLES);
}

void RenderOxygenUI(s8 g_font_id)
{
    if (g_font_id < 0)
    {
        printf("DEBUG: Font ID is invalid: %d\n", g_font_id);
        return;
    }

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    char text[64];
    sprintf_s(text, 64, "%.0f%%", oxygen_percentage);

    AEGfxPrint(g_font_id, text, 0.16f, -0.95f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);// adjustb for location of text
}



void RenderSafeZone(AEGfxVertexList* borderMesh)
{
    // Render in world space at fixed position (0, 0)
    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);  // Bright green
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // Safezone is centered at world position (0, 0)
    float safezone_center_x = 0.0f;
    float safezone_center_y = -2900.0f;

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, safezone_center_x, safezone_center_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_TRIANGLES);
}

void RenderShopTrigger(AEGfxVertexList* triggerMesh)
{
    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.7f);

    // Change color based on whether player is inside
    if (player_in_shop_zone)
    {
        // Green when player can interact
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
        // Yellow when player cannot interact
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
    }

    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Rot(&rotate, 0.0f);
    AEMtx33Trans(&translate, shop_trigger_x, shop_trigger_y);
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(triggerMesh, AE_GFX_MDM_TRIANGLES);
}
//
//
//void RenderShopPopup(AEGfxVertexList* backgroundImageMesh, AEGfxTexture* backgroundTexture,
//    AEGfxVertexList* upgradeBoxMesh, AEGfxVertexList* borderMesh,
//    AEGfxTexture* upgrade1Tex, AEGfxTexture* upgrade2Tex,
//    AEGfxTexture* upgrade3Tex, AEGfxTexture* upgrade4Tex, s8 font_id)
//{
//    // Switch to screen space
//    AEGfxSetCamPosition(0.0f, 0.0f);
//
//    // Draw background image ONLY (no overlay)
//    if (backgroundTexture && backgroundImageMesh)
//    {
//        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
//        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
//        AEGfxSetTransparency(1.0f);
//        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
//        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
//        AEGfxTextureSet(backgroundTexture, 0, 0);
//
//        AEMtx33 scale, rotate, translate, transform;
//        AEMtx33Scale(&scale, 1.0f, 1.0f);
//        AEMtx33Rot(&rotate, 0.0f);
//        AEMtx33Trans(&translate, 0.0f, 0.0f);
//        AEMtx33Concat(&transform, &rotate, &scale);
//        AEMtx33Concat(&transform, &translate, &transform);
//
//        AEGfxSetTransform(transform.m);
//        AEGfxMeshDraw(backgroundImageMesh, AE_GFX_MDM_TRIANGLES);
//    }
//
//   
//
//    // Calculate positions for 4 boxes
//    float total_width = (upgrade_box_width * 4) + (upgrade_box_spacing * 3);
//    float start_x = -total_width / 2.0f + upgrade_box_width / 2.0f;
//    float box_y = 50.0f;
//
//    AEMtx33 scale, rotate, translate, transform;
//    AEMtx33Scale(&scale, 1.0f, 1.0f);
//    AEMtx33Rot(&rotate, 0.0f);
//
//    //// Draw 4 upgrade boxes with textures AND borders
//    //AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
//    //AEGfxSetBlendMode(AE_GFX_BM_BLEND);
//    //AEGfxSetTransparency(1.0f);
//
//    //// Box 1 - Pick/Shovel & speed
//    //float box1_x = start_x;
//    //if (upgrade1Tex)
//    //{
//    //    AEGfxTextureSet(upgrade1Tex, 0, 0);
//    //    AEMtx33Trans(&translate, box1_x, box_y);
//    //    AEMtx33Concat(&transform, &rotate, &scale);
//    //    AEMtx33Concat(&transform, &translate, &transform);
//    //    AEGfxSetTransform(transform.m);
//    //    AEGfxMeshDraw(upgradeBoxMesh, AE_GFX_MDM_TRIANGLES);
//    //}
//    //AEGfxSetRenderMode(AE_GFX_RM_COLOR);
//    //AEGfxSetColorToMultiply(0.7f, 0.65f, 0.6f, 1.0f);
//    //AEGfxSetTransform(transform.m);
//    //AEGfxMeshDraw(borderMesh, AE_GFX_MDM_TRIANGLES);
//
//    //// Box 2 - Oxygen Level
//    //float box2_x = start_x + (upgrade_box_width + upgrade_box_spacing);
//    //AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
//    //if (upgrade2Tex)
//    //{
//    //    AEGfxTextureSet(upgrade2Tex, 0, 0);
//    //    AEMtx33Trans(&translate, box2_x, box_y);
//    //    AEMtx33Concat(&transform, &rotate, &scale);
//    //    AEMtx33Concat(&transform, &translate, &transform);
//    //    AEGfxSetTransform(transform.m);
//    //    AEGfxMeshDraw(upgradeBoxMesh, AE_GFX_MDM_TRIANGLES);
//    //}
//    //AEGfxSetRenderMode(AE_GFX_RM_COLOR);
//    //AEGfxSetColorToMultiply(0.7f, 0.65f, 0.6f, 1.0f);
//    //AEGfxSetTransform(transform.m);
//    //AEGfxMeshDraw(borderMesh, AE_GFX_MDM_TRIANGLES);
//
//    //// Box 3 - Sanity Level
//    //float box3_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 2;
//    //AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
//    //if (upgrade3Tex)
//    //{
//    //    AEGfxTextureSet(upgrade3Tex, 0, 0);
//    //    AEMtx33Trans(&translate, box3_x, box_y);
//    //    AEMtx33Concat(&transform, &rotate, &scale);
//    //    AEMtx33Concat(&transform, &translate, &transform);
//    //    AEGfxSetTransform(transform.m);
//    //    AEGfxMeshDraw(upgradeBoxMesh, AE_GFX_MDM_TRIANGLES);
//    //}
//    //AEGfxSetRenderMode(AE_GFX_RM_COLOR);
//    //AEGfxSetColorToMultiply(0.7f, 0.65f, 0.6f, 1.0f);
//    //AEGfxSetTransform(transform.m);
//    //AEGfxMeshDraw(borderMesh, AE_GFX_MDM_TRIANGLES);
//
//    //// Box 4 - Light Range
//    //float box4_x = start_x + (upgrade_box_width + upgrade_box_spacing) * 3;
//    //AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
//    //if (upgrade4Tex)
//    //{
//    //    AEGfxTextureSet(upgrade4Tex, 0, 0);
//    //    AEMtx33Trans(&translate, box4_x, box_y);
//    //    AEMtx33Concat(&transform, &rotate, &scale);
//    //    AEMtx33Concat(&transform, &translate, &transform);
//    //    AEGfxSetTransform(transform.m);
//    //    AEGfxMeshDraw(upgradeBoxMesh, AE_GFX_MDM_TRIANGLES);
//    //}
//    //AEGfxSetRenderMode(AE_GFX_RM_COLOR);
//    //AEGfxSetColorToMultiply(0.7f, 0.65f, 0.6f, 1.0f);
//    //AEGfxSetTransform(transform.m);
//    //AEGfxMeshDraw(borderMesh, AE_GFX_MDM_TRIANGLES);
//
//    //// Draw labels below each box
//    //if (font_id >= 0)
//    //{
//    //    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
//    //    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
//    //    AEGfxSetTransparency(1.0f);
//
//    //    // Label 1
//    //    AEGfxPrint(font_id, (char*)"Pick/Shovel &", -0.55f, -0.25f, 1.0f, 0.9f, 0.85f, 0.75f, 1.0f);
//    //    AEGfxPrint(font_id, (char*)"speed", -0.48f, -0.35f, 1.0f, 0.9f, 0.85f, 0.75f, 1.0f);
//
//    //    // Label 2
//    //    AEGfxPrint(font_id, (char*)"Oxygen", -0.20f, -0.25f, 1.0f, 0.9f, 0.85f, 0.75f, 1.0f);
//    //    AEGfxPrint(font_id, (char*)"Level", -0.20f, -0.35f, 1.0f, 0.9f, 0.85f, 0.75f, 1.0f);
//
//    //    // Label 3
//    //    AEGfxPrint(font_id, (char*)"Sanity", 0.09f, -0.25f, 1.0f, 0.9f, 0.85f, 0.75f, 1.0f);
//    //    AEGfxPrint(font_id, (char*)"Level", 0.09f, -0.35f, 1.0f, 0.9f, 0.85f, 0.75f, 1.0f);
//
//    //    // Label 4
//    //    AEGfxPrint(font_id, (char*)"Light", 0.38f, -0.25f, 1.0f, 0.9f, 0.85f, 0.75f, 1.0f);
//    //    AEGfxPrint(font_id, (char*)"Range", 0.38f, -0.35f, 1.0f, 0.9f, 0.85f, 0.75f, 1.0f);
//    //}
//}
//

void RenderShopPrompt(s8 font_id)
{
    if (font_id < 0 || !player_in_shop_zone || shop_is_open) return;

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // Display prompt at top of screen
    AEGfxPrint(font_id, (char*)"Press ENTER or LEFT CLICK to open shop", -0.4f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}



//void RenderUI()
//{
//    AEGfxSetCamPosition(0.0f, 0.0f);
//    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
//    AEGfxSetBlendColor(1.0f, 1.0f, 1.0f, 1.0f);
//
//    char text[128];
//
//    sprintf_s(text, 128, "Depth: %dm", depth);
//    AEGfxPrint(-750.0f, 400.0f, 1.5f, text);
//
//    sprintf_s(text, 128, "Max Depth: %dm", max_depth);
//    AEGfxPrint(-750.0f, 350.0f, 1.0f, text);
//
//    sprintf_s(text, 128, "Position: (%.0f, %.0f)", player_x, player_y);
//    AEGfxPrint(-750.0f, 300.0f, 1.0f, text);
//
//    AEGfxPrint(-650.0f, -420.0f, 1.0f, "WASD: Move  |  Click: Mine  |  ESC: Exit");
//}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// GAME STATE SYSTEM
// ---------------------------------------------------------------------------
enum GameState
{
    GS_MAIN_GAME,
    GS_SHOP
};

GameState current_state = GS_MAIN_GAME;
GameState next_state = GS_MAIN_GAME;





// MAIN

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int gGameRunning = 1;

    // Initialize
    AESysInit(hInstance, nCmdShow, (int)SCREEN_WIDTH, (int)SCREEN_HEIGHT, 1, 60, false, NULL);
    AESysSetWindowTitle("GroundBreaker Clone - Vertical Mining");


    // Load font
    g_font_id = AEGfxCreateFont("../Assets/liberation-mono.ttf", 24);
    if (g_font_id < 0) {
        // Font creation failed, use default or handle error
        g_font_id = 0;
    }

    // Generate world
    GenerateWorld();

    // Load texture
    AEGfxTexture* tilesetTexture = AEGfxTextureLoad("../Assets/tileset.png");
    AEGfxTexture* oxygeniconTexture = AEGfxTextureLoad("../Assets/o2icon.png");
	AEGfxTexture* sanityiconTexture = AEGfxTextureLoad("../Assets/sanityicon.png");
    AEGfxTexture* bouldericonTexture = AEGfxTextureLoad("../Assets/bouldericon.png");
    AEGfxTexture* mapiconTexture = AEGfxTextureLoad("../Assets/mapicon.png");

	//// Shop Upgrade Textures
 //   // Load shop textures (add after your existing texture loads)
 //   AEGfxTexture* upgrade1Texture = AEGfxTextureLoad("../Assets/pickaxeicon.png");
 //   AEGfxTexture* upgrade2Texture = AEGfxTextureLoad("../Assets/o2icon.png");
 //   AEGfxTexture* upgrade3Texture = AEGfxTextureLoad("../Assets/sanityicon.png");
 //   AEGfxTexture* upgrade4Texture = AEGfxTextureLoad("../Assets/torchicon.png");
 //   AEGfxTexture* shopBackgroundTexture = AEGfxTextureLoad("../Assets/shopui.png");


    AEGfxVertexList* dirtMesh;
    AEGfxVertexList* stoneMesh;
    int texture_loaded = 0;

    if (tilesetTexture)
    {
        // Texture loaded - use spritesheet
        dirtMesh = CreateSpritesheetTileMesh(DIRT_SPRITE_POSITION, TILE_SIZE);
        stoneMesh = CreateSpritesheetTileMesh(STONE_SPRITE_POSITION, TILE_SIZE);
        texture_loaded = 1;
    }
    else
    {
        // Fallback - use colored rectangles
        dirtMesh = CreateRectangleMesh(TILE_SIZE, TILE_SIZE, 0xFF8B4513);   // Brown
        stoneMesh = CreateRectangleMesh(TILE_SIZE, TILE_SIZE, 0xFF808080);  // Gray
        texture_loaded = 0;
    }

    // Create other meshes
    AEGfxVertexList* playerMesh = CreateRectangleMesh(player_width, player_height, 0xFF00AAFF);
    AEGfxVertexList* cursorMesh = CreateRectangleMesh(TILE_SIZE, TILE_SIZE, 0x80FFFF00);
    AEGfxVertexList* oxygeniconMesh = Createoxygenicon(oxygeniconwidth, oxygeniconheight);
	AEGfxVertexList* sanityiconMesh = Createsanityicon(sanityiconwidth, sanityiconheight);
	AEGfxVertexList* bouldericonMesh = Createbouldericon(bouldericonwidth, bouldericonheight);
    AEGfxVertexList* mapiconMesh = Createmapicon(mapiconwidth, mapiconheight);

	// Create side blackout meshes
    float playable_width = MAP_WIDTH * TILE_SIZE;  // 25 * 64 = 1600
    float blackout_width = (SCREEN_WIDTH - playable_width) / 2.0f;  // Width of each side bar

    AEGfxVertexList* leftBlackoutMesh = CreateSideBlackoutMesh(blackout_width, SCREEN_HEIGHT);
    AEGfxVertexList* rightBlackoutMesh = CreateSideBlackoutMesh(blackout_width, SCREEN_HEIGHT);

    // CREATE SAFEZONE BORDER MESH
    AEGfxVertexList* safezoneBorderMesh = CreateSafezoneBorder(
        800.0f,  // Width (safezone_x_max - safezone_x_min)
        600.0f,  // Height (safezone_y_max - safezone_y_min)
        1.0f     // Border thickness in pixels
    );

    //// Create shop meshes
    //AEGfxVertexList* shopTriggerMesh = CreateShopTriggerMesh(shop_trigger_width, shop_trigger_height);
    //AEGfxVertexList* shopBackgroundImageMesh = CreateShopBackgroundImageMesh(shop_popup_width, shop_popup_height);
    //AEGfxVertexList* upgradeBoxMesh = CreateUpgradeBoxMesh(upgrade_box_width, upgrade_box_height);
    //AEGfxVertexList* upgradeBoxBorderMesh = CreateUpgradeBoxBorderMesh(upgrade_box_width, upgrade_box_height, upgrade_border_thickness);
    AEGfxVertexList* shopTriggerMesh = CreateShopTriggerMesh(shop_trigger_width, shop_trigger_height);



    // Set initial player position (on surface)
    player_x = 0.0f;
    player_y = (5 * TILE_SIZE) - (MAP_HEIGHT * TILE_SIZE / 2.0f) - 100.0f;

    Shop_Load();

    // GAME LOOP
    while (gGameRunning)
    {
        AESysFrameStart();

        // Check for state change
        if (current_state != next_state)
        {
            // Free current state
            if (current_state == GS_SHOP)
            {
                Shop_Free();
            }

            // Initialize next state
            current_state = next_state;

            if (current_state == GS_SHOP)
            {
                Shop_Initialize();
            }
        }

        float dt = AEFrameRateControllerGetFrameTime();

        // Update current state
        if (current_state == GS_MAIN_GAME)
        {
            // Main game updates
            UpdatePhysics(dt);
            UpdateMining(dt);
            UpdateCamera(dt);
            UpdateOxygenSystem(dt);
            UpdateShopSystem(dt);

            // Check if player wants to enter shop
            if (player_in_shop_zone)
            {
                if (AEInputCheckTriggered(AEVK_RETURN) || AEInputCheckTriggered(AEVK_LBUTTON))
                {
                    next_state = GS_SHOP;
                }
            }

            // Render main game
            AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
            AEGfxSetCamPosition(camera_x, camera_y);

            if (texture_loaded)
                RenderBackground(tilesetTexture, dirtMesh, stoneMesh);
            else
                RenderBackgroundFallback(dirtMesh, stoneMesh);

            RenderMiningCursor(cursorMesh);
            RenderPlayer(playerMesh);
            RenderSideBlackout(leftBlackoutMesh, rightBlackoutMesh);
            RenderSafeZone(safezoneBorderMesh);

            // Draw icons
            Rendermapicon(mapiconTexture, mapiconMesh, mapiconwidth, mapiconheight);
            Renderoxygenicon(oxygeniconTexture, oxygeniconMesh, oxygeniconwidth, oxygeniconheight);
            Rendersanityicon(sanityiconTexture, sanityiconMesh, sanityiconwidth, sanityiconheight);
            Renderbouldericon(bouldericonTexture, bouldericonMesh, bouldericonwidth, bouldericonheight);

            // Draw shop trigger box
            RenderShopTrigger(shopTriggerMesh);
            RenderShopPrompt(g_font_id);

            RenderOxygenUI(g_font_id);
            //RenderUI();
        }
        else if (current_state == GS_SHOP)
        {
            // Shop state update
            Shop_Update();

            // Check if player exits shop
            if (AEInputCheckTriggered(AEVK_ESCAPE))
            {
                next_state = GS_MAIN_GAME;
            }

            // Render shop
            Shop_Draw();
        }

        // Exit game
        if (AEInputCheckTriggered(AEVK_ESCAPE) && current_state == GS_MAIN_GAME)
        {
            if (!AESysDoesWindowExist())
                gGameRunning = 0;
        }
        //Shop_Unload();
        AESysFrameEnd();
    }



   // Cleanup
    AEGfxMeshFree(dirtMesh);
    AEGfxMeshFree(stoneMesh);
    AEGfxMeshFree(playerMesh);
    AEGfxMeshFree(cursorMesh);
    AEGfxMeshFree(leftBlackoutMesh);
    AEGfxMeshFree(rightBlackoutMesh);
    AEGfxMeshFree(safezoneBorderMesh);  // FREE THE BORDER MESH
    AEGfxMeshFree(oxygeniconMesh);
    AEGfxMeshFree(sanityiconMesh);
    AEGfxMeshFree(bouldericonMesh);
    AEGfxMeshFree(mapiconMesh);
    //// Cleanup shop meshes
    //AEGfxMeshFree(shopTriggerMesh);
    //AEGfxMeshFree(upgradeBoxMesh);
    //AEGfxMeshFree(upgradeBoxBorderMesh);

    //// Unload shop textures
    //if (upgrade1Texture) AEGfxTextureUnload(upgrade1Texture);
    //if (upgrade2Texture) AEGfxTextureUnload(upgrade2Texture);
    //if (upgrade3Texture) AEGfxTextureUnload(upgrade3Texture);
    //if (upgrade4Texture) AEGfxTextureUnload(upgrade4Texture);



    if (tilesetTexture)
        AEGfxTextureUnload(tilesetTexture);
    
    if (oxygeniconTexture)
        AEGfxTextureUnload(oxygeniconTexture);

	if (sanityiconTexture)
		AEGfxTextureUnload(sanityiconTexture);

	if (bouldericonTexture)
		AEGfxTextureUnload(bouldericonTexture);

	if (mapiconTexture)
		AEGfxTextureUnload(mapiconTexture);

    if (g_font_id >= 0)
        AEGfxDestroyFont(g_font_id);

    AESysExit();

    return 0;
}
