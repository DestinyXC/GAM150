// ---------------------------------------------------------------------------
// GroundBreaker-Style Mining Game
// 256x192 Spritesheet (16x16 tiles), Vertical Camera, 2 Tiles
// ---------------------------------------------------------------------------

#include <crtdbg.h>
#include "AEEngine.h"
#include <stdio.h>
#include <time.h>

// ---------------------------------------------------------------------------
// CONFIGURATION

#define MAP_WIDTH 15        // Fixed screen width in tiles (25 * 64 = 1600)
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

// ---------------------------------------------------------------------------
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

    // Generate world
    GenerateWorld();

    // Load texture
    AEGfxTexture* tilesetTexture = AEGfxTextureLoad("Assets/tileset.png");

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

	// Create side blackout meshes
    float playable_width = MAP_WIDTH * TILE_SIZE;  // 25 * 64 = 1600
    float blackout_width = (SCREEN_WIDTH - playable_width) / 2.0f;  // Width of each side bar

    AEGfxVertexList* leftBlackoutMesh = CreateSideBlackoutMesh(blackout_width, SCREEN_HEIGHT);
    AEGfxVertexList* rightBlackoutMesh = CreateSideBlackoutMesh(blackout_width, SCREEN_HEIGHT);

    // Set initial player position (on surface)
    player_x = 0.0f;
    player_y = (5 * TILE_SIZE) - (MAP_HEIGHT * TILE_SIZE / 2.0f) - 100.0f;

    // ===== GAME LOOP =====
    while (gGameRunning)
    {
        AESysFrameStart();

        float dt = AEFrameRateControllerGetFrameTime();

        // Update
        UpdatePhysics(dt);
        UpdateMining(dt);
        UpdateCamera(dt);

        // Render
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);  // Black background
        AEGfxSetCamPosition(camera_x, camera_y);

        // Draw background
        if (texture_loaded)
        {
            RenderBackground(tilesetTexture, dirtMesh, stoneMesh);
        }
        /*else
        {
            RenderBackgroundFallback(dirtMesh, stoneMesh);
        }*/

        // Draw mining cursor
        RenderMiningCursor(cursorMesh);

        // Draw player
        RenderPlayer(playerMesh);

		// Draw side blackouts
		RenderSideBlackout(leftBlackoutMesh, rightBlackoutMesh);

        // Draw UI
        //RenderUI();

        // Exit
        if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
            gGameRunning = 0;

        AESysFrameEnd();
    }

    // Cleanup
    AEGfxMeshFree(dirtMesh);
    AEGfxMeshFree(stoneMesh);
    AEGfxMeshFree(playerMesh);
    AEGfxMeshFree(cursorMesh);
    AEGfxMeshFree(leftBlackoutMesh); 
    AEGfxMeshFree(rightBlackoutMesh);

    if (tilesetTexture)
        AEGfxTextureUnload(tilesetTexture);

    AESysExit();

    return 0;
}
