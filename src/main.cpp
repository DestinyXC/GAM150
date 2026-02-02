// ---------------------------------------------------------------------------
// GroundBreaker-Style Mining Game
// 256x192 Spritesheet (16x16 tiles), Vertical Camera, 2 Tiles
// ---------------------------------------------------------------------------

#include <crtdbg.h>
#include "AEEngine.h"
#include "shop.hpp"
#include <stdio.h>
#include <time.h>

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

// Lighting configurations
#define MAX_TORCHES 50
#define TORCH_GLOW_RADIUS 200.0f;
#define HEADLAMP_GLOW_RADIUS 180.0f;

// Mineable rocks configurations
#define MAX_ROCKS 500
#define ROCK_SPAWN_CHANCE 0.15f  // 5% chance to spawn a rock in stone tile

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Shop System - managed by shop.cpp
// Variables: shop_is_active, oxygen_upgrade_level, pick_upgrade_level, etc.
// See shop.hpp for interface
int player_in_shop_zone = 0;
float shop_trigger_x = 400.0f;
float shop_trigger_y = -2700.0f;
float shop_trigger_width = 150.0f;
float shop_trigger_height = 150.0f;

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
#define WALL_SPRITE_POSITION 16    // Change to your wall tile position

// ---------------------------------------------------------------------------
// TORCH STRUCTURE

typedef struct {
    float x;
    float y;
    int active;
    float glow_radius;
    float flicker_time;
    float flicker_offset;
} Torch;

// ---------------------------------------------------------------------------
// ROCK STRUCTURE

typedef struct {
    float x;
    float y;
    int active;
    int sprite_index;
} Rock;

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
float player_width = 75.0f;
float player_height = 75.0f;
float player_speed = 5.0f;

// Mining
float mining_range = 120.0f;
int currently_mining_row = -1;
int currently_mining_col = -1;

// Stats
int depth = 0;
int max_depth = 0;

// Lighting
Torch torches[MAX_TORCHES];
int torch_count = 0;
float game_time = 0.0f;

// Rocks
Rock rocks[MAX_ROCKS];
int rock_count = 0;

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

// Safe Zone
float safezone_x_min = -500.0f;
float safezone_x_max = 500.0f;
float safezone_y_min = -3200.0f;
float safezone_y_max = -2530.0f;

//Font
s8 g_font_id = -1;

// ---------------------------------------------------------------------------
// FILE-SCOPE TEXTURE & MESH POINTERS  (shared by Init / Draw / Kill)

static AEGfxTexture* g_tilesetTexture = NULL;
static AEGfxTexture* g_oxygeniconTexture = NULL;
static AEGfxTexture* g_sanityiconTexture = NULL;
static AEGfxTexture* g_bouldericonTexture = NULL;
static AEGfxTexture* g_mapiconTexture = NULL;
static AEGfxTexture* g_playerTexture = NULL;
static AEGfxTexture* g_glowTexture = NULL;
static AEGfxTexture* g_rockTexture = NULL;

static AEGfxVertexList* g_dirtMesh = NULL;
static AEGfxVertexList* g_stoneMesh = NULL;
static AEGfxVertexList* g_wallMesh = NULL;
static AEGfxVertexList* g_playerMesh = NULL;
static AEGfxVertexList* g_glowMesh = NULL;
static AEGfxVertexList* g_rockMesh = NULL;
static AEGfxVertexList* g_cursorMesh = NULL;
static AEGfxVertexList* g_oxygeniconMesh = NULL;
static AEGfxVertexList* g_sanityiconMesh = NULL;
static AEGfxVertexList* g_bouldericonMesh = NULL;
static AEGfxVertexList* g_mapiconMesh = NULL;
static AEGfxVertexList* g_leftBlackoutMesh = NULL;
static AEGfxVertexList* g_rightBlackoutMesh = NULL;
static AEGfxVertexList* g_safezoneBorderMesh = NULL;
static AEGfxVertexList* g_shopTriggerMesh = NULL;
// Shop meshes/textures moved to shop.cpp

static int g_texture_loaded = 0;

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

AEGfxVertexList* CreateGlowMesh(float size)
{
    AEGfxMeshStart();

    float half = size / 2.0f;

    AEGfxTriAdd(
        -half, -half, 0xFFFFFFFF, 0.0f, 1.0f,
        half, -half, 0xFFFFFFFF, 1.0f, 1.0f,
        -half, half, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(
        half, -half, 0xFFFFFFFF, 1.0f, 1.0f,
        half, half, 0xFFFFFFFF, 1.0f, 0.0f,
        -half, half, 0xFFFFFFFF, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

// Creates a mesh for rendering a texture with full UVs (player, enemy)
AEGfxVertexList* CreateTextureMesh(float width, float height)
{
    AEGfxMeshStart();

    float half_width = width / 2.0f;
    float half_height = height / 2.0f;

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

// ---------------------------------------------------------------------------
// WORLD GENERATION

void GenerateWorld()
{
    srand((unsigned int)time(NULL));

    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            if (col == MAP_WIDTH / 2 && row == 9) {
                tileMap[row][col] = TILE_EMPTY;
            }
            else if (col == MAP_WIDTH / 2 - 1 && row == 9) {
                tileMap[row][col] = TILE_EMPTY;
            }
            else if (col == 0) {
                tileMap[row][col] = TILE_WALL;
            }
            else if (col == MAP_WIDTH - 1) {
                tileMap[row][col] = TILE_WALL;
            }
            else if (row == 9) {
                tileMap[row][col] = TILE_WALL;
            }
            // Surface area (rows 0-5) - empty (sky)
            else if (row < 1)
            {
                tileMap[row][col] = TILE_WALL;
            }
            // Shallow dirt layer (rows 5-30)
            else if (row < 10)
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
    return (tile_type == TILE_WALL);
}

// ---------------------------------------------------------------------------
// TORCH SYSTEM

void InitializeTorches()
{
    torch_count = 0;

    // Scan the map for wall tiles and place torches on them
    for (int row = 3; row < MAP_HEIGHT; row += 5)  // Every 5 rows
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            if (torch_count >= MAX_TORCHES) break;

            // Place torches on left and right walls
            if ((col == 0 || col == MAP_WIDTH - 1) && tileMap[row][col] == TILE_WALL)
            {
                float torch_x, torch_y;
                GetTileWorldPosition(row, col, &torch_x, &torch_y);

                torches[torch_count].x = torch_x;
                torches[torch_count].y = torch_y;
                torches[torch_count].active = 1;
                torches[torch_count].glow_radius = TORCH_GLOW_RADIUS;
                torches[torch_count].flicker_time = 0.0f;
                torches[torch_count].flicker_offset = (float)(rand() % 100) / 100.0f * 6.28f;
                torch_count++;
            }
        }
    }
}

void InitializeRocks()
{
    rock_count = 0;

    for (int row = 20; row < MAP_HEIGHT; row += 5) {
        for (int col = 3; col < MAP_WIDTH; col += 2)
        {
            if (rock_count >= MAX_ROCKS) break;
            // Place rocks in stone layers
            if (tileMap[row][col] == TILE_STONE)
            {
                float random_value = (float)rand() / (float)RAND_MAX;
                if (random_value < ROCK_SPAWN_CHANCE)
                {
                    float rock_x, rock_y;
                    GetTileWorldPosition(row, col, &rock_x, &rock_y);

                    rocks[rock_count].x = rock_x;
                    rocks[rock_count].y = rock_y;
                    rocks[rock_count].active = 1;
                    rocks[rock_count].sprite_index = rand() % 3;  // Random rock variation (0-2)
                    rock_count++;
                }
            }
        }
    }
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

int CheckTileCollision(float x, float y)
{
    // Get the tile the player is in
    int player_row, player_col;
    GetTileAtPosition(x, y, &player_row, &player_col);

    // Check tiles around the player (3x3 grid)
    for (int dr = -1; dr <= 1; dr++)
    {
        for (int dc = -1; dc <= 1; dc++)
        {
            int check_row = player_row + dr;
            int check_col = player_col + dc;

            if (!IsTileValid(check_row, check_col)) continue;

            int tile_type = tileMap[check_row][check_col];

            // Check collision with solid tiles (DIRT, STONE, WALL)
            if (IsTileSolid(tile_type))
            {
                float tile_x, tile_y;
                GetTileWorldPosition(check_row, check_col, &tile_x, &tile_y);

                if (CheckCollisionRectangle(x, y, player_width, player_height,
                    tile_x, tile_y, TILE_SIZE, TILE_SIZE))
                {
                    return 1;  // Collision detected
                }
            }
        }
    }

    return 0;  // No collision
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

    // Try to move horizontally
    float new_x = player_x + move_x;
    float new_y = player_y;

    if (!CheckTileCollision(new_x, new_y))
    {
        player_x = new_x;
    }

    // Try to move vertically
    new_x = player_x;
    new_y = player_y + move_y;

    if (!CheckTileCollision(new_x, new_y))
    {
        player_y = new_y;
    }

    // Keep player within horizontal bounds
    float world_width = MAP_WIDTH * TILE_SIZE;
    float min_x = -world_width / 2.0f + player_width / 2.0f;
    float max_x = world_width / 2.0f - player_width / 2.0f;

    if (player_x < min_x) player_x = min_x;
    if (player_x > max_x) player_x = max_x;

    // Keep player within vertical bounds
    float world_height = MAP_HEIGHT * TILE_SIZE;
    float min_y = -world_height / 2.0f + player_height / 2.0f;
    float max_y = world_height / 2.0f - player_height / 2.0f;

    if (player_y < min_y) player_y = min_y;
    if (player_y > max_y) player_y = max_y;

    // Update depth tracking
    int player_row, player_col;
    GetTileAtPosition(player_x, player_y, &player_row, &player_col);
    depth = player_row - 9;  // Surface is row 9
    if (depth < 0) depth = 0;
    if (depth > max_depth) max_depth = depth;

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

    if (shop_is_active)
    {
        // Shop is open - handle shop updates
        Shop_Update();

        // Close shop with ESC or ENTER (shop.cpp handles clicks)
        if (AEInputCheckTriggered(AEVK_ESCAPE) ||
            AEInputCheckTriggered(AEVK_RETURN))
        {
            Shop_Free();  // This sets shop_is_active = 0
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
                Shop_Initialize();  // This sets shop_is_active = 1
            }
        }
    }
}


// ---------------------------------------------------------------------------
// RENDERING

void RenderBackground(AEGfxTexture* tileset, AEGfxVertexList* dirtMesh, AEGfxVertexList* stoneMesh, AEGfxVertexList* wallMesh)
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
            else if (tile_type == TILE_WALL)
            {
                AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }
}

void RenderLighting(AEGfxTexture* glowTexture, AEGfxVertexList* glowMesh)
{
    // Set up additive blending for glow effect
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_ADD);  // CRITICAL: Additive blending
    AEGfxTextureSet(glowTexture, 0, 0);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // Draw all torch lights
    for (int i = 0; i < torch_count; i++)
    {
        if (!torches[i].active) continue;

        float glow_scale = torches[i].glow_radius / 128.0f;  // 128 = half glow texture size

        AEGfxSetTransparency(0.5f);  // Adjust brightness

        AEMtx33 scale, rotate, translate, transform;
        AEMtx33Scale(&scale, glow_scale, glow_scale);
        AEMtx33Rot(&rotate, 0.0f);
        AEMtx33Trans(&translate, torches[i].x, torches[i].y);
        AEMtx33Concat(&transform, &rotate, &scale);
        AEMtx33Concat(&transform, &translate, &transform);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(glowMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void RenderRocks(AEGfxTexture* rockTexture, AEGfxVertexList* rockMesh)
{
    if (!rockTexture || !rockMesh) return;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(rockTexture, 0, 0);

    // Calculate visible range (same as tiles for culling)
    int start_row = (int)((camera_y - SCREEN_HEIGHT / 2 - TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;
    int end_row = (int)((camera_y + SCREEN_HEIGHT / 2 + TILE_SIZE) / TILE_SIZE) + MAP_HEIGHT / 2;

    if (start_row < 0) start_row = 0;
    if (end_row >= MAP_HEIGHT) end_row = MAP_HEIGHT - 1;

    // Draw rocks that are on screen
    for (int i = 0; i < rock_count; i++)
    {
        if (!rocks[i].active) continue;

        // Simple culling - check if rock is roughly in visible range
        int rock_row = (int)((rocks[i].y + (MAP_HEIGHT * TILE_SIZE / 2.0f)) / TILE_SIZE);
        if (rock_row < start_row || rock_row > end_row) continue;

        AEMtx33 scale, rotate, translate, transform;
        AEMtx33Scale(&scale, 1.0f, 1.0f);
        AEMtx33Rot(&rotate, 0.0f);
        AEMtx33Trans(&translate, rocks[i].x, rocks[i].y);
        AEMtx33Concat(&transform, &rotate, &scale);
        AEMtx33Concat(&transform, &translate, &transform);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(rockMesh, AE_GFX_MDM_TRIANGLES);
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

void RenderPlayer(AEGfxTexture* playerTexture, AEGfxVertexList* playerMesh)
{
    if (playerTexture)
    {
        // Render with player texture
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxTextureSet(playerTexture, 0, 0);  // Use player texture
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        // Fallback color mode
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }

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

void RenderShopPrompt(s8 font_id)
{
    if (font_id < 0 || !player_in_shop_zone || shop_is_active) return;

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // Display prompt at top of screen
    AEGfxPrint(font_id, (char*)"Press ENTER or LEFT CLICK to open shop", -0.4f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// GAME STATE FUNCTIONS  (called from mainmenu.cpp via GameStates.h)
// ---------------------------------------------------------------------------

void Game_Init(void)
{
    // Load font
    g_font_id = AEGfxCreateFont("../Assets/fonts/liberation-mono.ttf", 24);
    if (g_font_id < 0)
        g_font_id = 0;

    // Generate world
    GenerateWorld();
    InitializeTorches();
    InitializeRocks();

    // Load textures
    g_tilesetTexture = AEGfxTextureLoad("../Assets/tileset.png");
    g_oxygeniconTexture = AEGfxTextureLoad("../Assets/o2icon.png");
    g_sanityiconTexture = AEGfxTextureLoad("../Assets/sanityicon.png");
    g_bouldericonTexture = AEGfxTextureLoad("../Assets/bouldericon.png");
    g_mapiconTexture = AEGfxTextureLoad("../Assets/mapicon.png");
    g_playerTexture = AEGfxTextureLoad("../Assets/player.png");
    g_glowTexture = AEGfxTextureLoad("../Assets/glow_texture.png");
    g_rockTexture = AEGfxTextureLoad("../Assets/mineable_rock.png");

    // Create tile meshes
    g_texture_loaded = 0;
    if (g_tilesetTexture)
    {
        g_dirtMesh = CreateSpritesheetTileMesh(DIRT_SPRITE_POSITION, TILE_SIZE);
        g_stoneMesh = CreateSpritesheetTileMesh(STONE_SPRITE_POSITION, TILE_SIZE);
        g_wallMesh = CreateSpritesheetTileMesh(WALL_SPRITE_POSITION, TILE_SIZE);
        g_texture_loaded = 1;
    }
    else
    {
        g_dirtMesh = CreateRectangleMesh(TILE_SIZE, TILE_SIZE, 0xFF8B4513);
        g_stoneMesh = CreateRectangleMesh(TILE_SIZE, TILE_SIZE, 0xFF808080);
    }

    if (g_playerTexture)
    {
        g_playerMesh = CreateTextureMesh(player_width, player_height);
    }
    else
    {
        g_playerMesh = CreateRectangleMesh(player_width, player_height, 0xFF0000FF);
    }

    // Create other meshes
    g_cursorMesh = CreateRectangleMesh(TILE_SIZE, TILE_SIZE, 0x80FFFF00);
    g_glowMesh = CreateGlowMesh(256.0f);
    g_rockMesh = CreateGlowMesh(TILE_SIZE);
    g_oxygeniconMesh = Createoxygenicon(oxygeniconwidth, oxygeniconheight);
    g_sanityiconMesh = Createsanityicon(sanityiconwidth, sanityiconheight);
    g_bouldericonMesh = Createbouldericon(bouldericonwidth, bouldericonheight);
    g_mapiconMesh = Createmapicon(mapiconwidth, mapiconheight);

    float playable_width = MAP_WIDTH * TILE_SIZE;
    float blackout_width = (SCREEN_WIDTH - playable_width) / 2.0f;

    g_leftBlackoutMesh = CreateSideBlackoutMesh(blackout_width, SCREEN_HEIGHT);
    g_rightBlackoutMesh = CreateSideBlackoutMesh(blackout_width, SCREEN_HEIGHT);

    g_safezoneBorderMesh = CreateSafezoneBorder(800.0f, 600.0f, 1.0f);
    g_shopTriggerMesh = CreateShopTriggerMesh(shop_trigger_width, shop_trigger_height);

    // Load shop system (textures, meshes, font)
    Shop_Load();

    // Set initial player position
    player_x = 0.0f;
    player_y = (5 * TILE_SIZE) - (MAP_HEIGHT * TILE_SIZE / 2.0f) - 100.0f;

    // Reset oxygen
    oxygen_percentage = 100.0f;
    oxygen_max = 100.0f;
}

void Game_Update(void)
{
    float dt = AEFrameRateControllerGetFrameTime();

    UpdatePhysics(dt);
    UpdateMining(dt);
    UpdateCamera(dt);
    UpdateOxygenSystem(dt);
    UpdateShopSystem(dt);
}

void Game_Draw(void)
{
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetCamPosition(camera_x, camera_y);

    if (g_texture_loaded)
        RenderBackground(g_tilesetTexture, g_dirtMesh, g_stoneMesh, g_wallMesh);

    if (g_glowTexture)
        RenderLighting(g_glowTexture, g_glowMesh);
    if (g_rockTexture)
        RenderRocks(g_rockTexture, g_rockMesh);

    RenderMiningCursor(g_cursorMesh);
    RenderPlayer(g_playerTexture, g_playerMesh);
    RenderSideBlackout(g_leftBlackoutMesh, g_rightBlackoutMesh);
    RenderSafeZone(g_safezoneBorderMesh);

    if (g_mapiconMesh)
        Rendermapicon(g_mapiconTexture, g_mapiconMesh, mapiconwidth, mapiconheight);
    if (g_oxygeniconMesh)
        Renderoxygenicon(g_oxygeniconTexture, g_oxygeniconMesh, oxygeniconwidth, oxygeniconheight);
    if (g_sanityiconMesh)
        Rendersanityicon(g_sanityiconTexture, g_sanityiconMesh, sanityiconwidth, sanityiconheight);
    if (g_bouldericonMesh)
        Renderbouldericon(g_bouldericonTexture, g_bouldericonMesh, bouldericonwidth, bouldericonheight);

    if (!shop_is_active)
    {
        RenderShopTrigger(g_shopTriggerMesh);
        RenderShopPrompt(g_font_id);

        RenderOxygenUI(g_font_id);
    }
    else  // shop_is_active == 1
    {
        Shop_Draw();  // Render the shop UI from shop.cpp
    }
}

void Game_Kill(void)
{
    // Free meshes
    if (g_dirtMesh) { AEGfxMeshFree(g_dirtMesh); g_dirtMesh = NULL; }
    if (g_stoneMesh) { AEGfxMeshFree(g_stoneMesh); g_stoneMesh = NULL; }
    if (g_wallMesh) { AEGfxMeshFree(g_wallMesh); g_wallMesh = NULL; }
    if (g_glowMesh) { AEGfxMeshFree(g_glowMesh); g_glowMesh = NULL; }
    if (g_rockMesh) { AEGfxMeshFree(g_rockMesh); g_rockMesh = NULL; }
    if (g_playerMesh) { AEGfxMeshFree(g_playerMesh); g_playerMesh = NULL; }
    if (g_cursorMesh) { AEGfxMeshFree(g_cursorMesh); g_cursorMesh = NULL; }
    if (g_oxygeniconMesh) { AEGfxMeshFree(g_oxygeniconMesh); g_oxygeniconMesh = NULL; }
    if (g_sanityiconMesh) { AEGfxMeshFree(g_sanityiconMesh); g_sanityiconMesh = NULL; }
    if (g_bouldericonMesh) { AEGfxMeshFree(g_bouldericonMesh); g_bouldericonMesh = NULL; }
    if (g_mapiconMesh) { AEGfxMeshFree(g_mapiconMesh); g_mapiconMesh = NULL; }
    if (g_leftBlackoutMesh) { AEGfxMeshFree(g_leftBlackoutMesh); g_leftBlackoutMesh = NULL; }
    if (g_rightBlackoutMesh) { AEGfxMeshFree(g_rightBlackoutMesh); g_rightBlackoutMesh = NULL; }
    if (g_safezoneBorderMesh) { AEGfxMeshFree(g_safezoneBorderMesh); g_safezoneBorderMesh = NULL; }
    if (g_shopTriggerMesh) { AEGfxMeshFree(g_shopTriggerMesh); g_shopTriggerMesh = NULL; }
    // Shop meshes cleaned up in Shop_Unload()

    // Unload textures
    if (g_tilesetTexture) { AEGfxTextureUnload(g_tilesetTexture);     g_tilesetTexture = NULL; }
    if (g_oxygeniconTexture) { AEGfxTextureUnload(g_oxygeniconTexture);  g_oxygeniconTexture = NULL; }
    if (g_sanityiconTexture) { AEGfxTextureUnload(g_sanityiconTexture);  g_sanityiconTexture = NULL; }
    if (g_bouldericonTexture) { AEGfxTextureUnload(g_bouldericonTexture); g_bouldericonTexture = NULL; }
    if (g_mapiconTexture) { AEGfxTextureUnload(g_mapiconTexture);     g_mapiconTexture = NULL; }
    if (g_playerTexture) { AEGfxTextureUnload(g_playerTexture);      g_playerTexture = NULL; }
    if (g_glowTexture) { AEGfxTextureUnload(g_glowTexture);        g_glowTexture = NULL; }
    if (g_rockTexture) { AEGfxTextureUnload(g_rockTexture);        g_rockTexture = NULL; }
    // Shop textures cleaned up in Shop_Unload()

    // Destroy font
    if (g_font_id >= 0)
    {
        AEGfxDestroyFont(g_font_id);
        g_font_id = -1;
    }

    // Unload shop system
    Shop_Unload();
}