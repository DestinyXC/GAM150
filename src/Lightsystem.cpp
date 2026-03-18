#include "AEEngine.h"
#include "lightsystem.hpp"
#include <stdio.h>
#include <math.h>

// ---------------------------------------------------------------------------
// lightsystem.cpp  -  Darkness overlay + player torch for Core Break
// ---------------------------------------------------------------------------
//
// HOW IT WORKS
// ------------
// Alpha Engine has no stencil / render-target support, so we simulate a
// torch "cutout" with two draw passes every frame:
//
//   Pass 1 - Darkness rectangle
//     A single large black rectangle covers everything underground.
//     It is drawn with DARKNESS_ALPHA transparency so the tiles beneath
//     are still faintly visible (feels like deep shadow rather than a
//     void).  The safe-zone rows at the top of the map are left completely
//     uncovered so the surface area stays normally lit.
//
//   Pass 2 - Torch gradient (drawn ON TOP of the darkness)
//     A fan of triangles radiates from the player centre.  The inner
//     triangles use a bright warm colour at near-zero alpha (fully
//     transparent = "punched hole") and the outer rings step up in alpha
//     toward the same black as the darkness layer.  This creates a smooth
//     fade from "fully lit" at the centre to "fully dark" at the edge.
//
//     Because AE blends SRC_ALPHA / (1-SRC_ALPHA) and we draw the torch
//     AFTER the darkness rect, the transparent torch centre effectively
//     lets the world show through while the darkness ring at the edge
//     joins seamlessly with the outer darkness rect.
//
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// PRIVATE - Mesh storage
// ---------------------------------------------------------------------------

// Single large rectangle covering the underground area
static AEGfxVertexList* g_darknessMesh = NULL;

// Per-ring meshes for the torch gradient (TORCH_GRADIENT_RINGS rings,
// each ring is a triangle fan strip between two radii)
static AEGfxVertexList* g_torchRing[TORCH_GRADIENT_RINGS] = { NULL };

// ---------------------------------------------------------------------------
// PRIVATE - Helpers
// ---------------------------------------------------------------------------

// Build a solid colour rectangle centred at (0,0) with given width/height.
static AEGfxVertexList* MakeSolidRect(float w, float h, unsigned int col)
{
    float hw = w * 0.5f;
    float hh = h * 0.5f;

    AEGfxMeshStart();
    // Triangle 1
    AEGfxTriAdd(-hw, -hh, col, 0.0f, 1.0f,
        hw, -hh, col, 1.0f, 1.0f,
        -hw, hh, col, 0.0f, 0.0f);
    // Triangle 2
    AEGfxTriAdd(hw, -hh, col, 1.0f, 1.0f,
        hw, hh, col, 1.0f, 0.0f,
        -hw, hh, col, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

// ---------------------------------------------------------------------------
// Build one ring of the torch gradient.
//
//   inner_r  - inner radius of this ring (transparent / bright side)
//   outer_r  - outer radius of this ring (darker side)
//   inner_a  - alpha at the inner edge  (0 = fully transparent)
//   outer_a  - alpha at the outer edge  (1 = fully opaque black)
//
// Each ring is a triangle-strip-style fan of TORCH_RING_SEGMENTS quads.
// We bake the colour directly into the vertex colour so no texture is needed.
// ---------------------------------------------------------------------------
static AEGfxVertexList* MakeTorchRing(float inner_r, float outer_r,
    float inner_a, float outer_a)
{
    // Encode black with per-vertex alpha into ARGB unsigned int
    // AE vertex colour: 0xAARRGGBB
    unsigned int col_inner = (unsigned int)(inner_a * 255.0f) << 24 | 0x00FFFF00; // transparent yellow
    unsigned int col_outer = (unsigned int)(outer_a * 255.0f) << 24 | 0x00FFFF00; // opaque yellow

    // For the very centre, clamp to fully transparent
    if (inner_a <= 0.0f) col_inner = 0x00000000;

    AEGfxMeshStart();

    for (int i = 0; i < TORCH_RING_SEGMENTS; ++i)
    {
        float a0 = (float)i / (float)TORCH_RING_SEGMENTS * 6.28318530f;
        float a1 = (float)(i + 1) / (float)TORCH_RING_SEGMENTS * 6.28318530f;

        float ix0 = cosf(a0) * inner_r, iy0 = sinf(a0) * inner_r;
        float ox0 = cosf(a0) * outer_r, oy0 = sinf(a0) * outer_r;
        float ix1 = cosf(a1) * inner_r, iy1 = sinf(a1) * inner_r;
        float ox1 = cosf(a1) * outer_r, oy1 = sinf(a1) * outer_r;

        // Quad = 2 triangles
        // Triangle A: inner0, outer0, inner1
        AEGfxTriAdd(ix0, iy0, col_inner, 0, 0,
            ox0, oy0, col_outer, 0, 0,
            ix1, iy1, col_inner, 0, 0);
        // Triangle B: outer0, outer1, inner1
        AEGfxTriAdd(ox0, oy0, col_outer, 0, 0,
            ox1, oy1, col_outer, 0, 0,
            ix1, iy1, col_inner, 0, 0);
    }

    return AEGfxMeshEnd();
}

// ---------------------------------------------------------------------------
// LightSystem_Init
// ---------------------------------------------------------------------------
void LightSystem_Init(void)
{
    // ------------------------------------------------------------------
    // 1. Darkness rectangle
    //    Large enough to cover the entire underground map.
    //    Width  = full map width (with a little padding)
    //    Height = full map height (entire underground area)
    //    It is positioned at draw-time so we just make it unit-sized and
    //    scale it via the transform matrix each frame.
    // ------------------------------------------------------------------
    g_darknessMesh = MakeSolidRect(1.0f, 1.0f, 0xFF000000);  // opaque black, alpha set in draw

    if (!g_darknessMesh)
        printf("LightSystem_Init: failed to create darkness mesh!\n");
    else
        printf("LightSystem_Init: darkness mesh created.\n");

    // ------------------------------------------------------------------
    // 2. Torch gradient rings
    //    We distribute TORCH_GRADIENT_RINGS rings evenly from radius 0
    //    out to 1.0 (normalised).  At draw-time we scale by torch_radius.
    //    The innermost ring: fully transparent (alpha 0) at its centre.
    //    The outermost ring: alpha matches DARKNESS_ALPHA at its outer edge
    //    so it blends invisibly into the surrounding darkness rect.
    // ------------------------------------------------------------------
    for (int r = 0; r < TORCH_GRADIENT_RINGS; ++r)
    {
        float t_inner = (float)r / (float)TORCH_GRADIENT_RINGS;
        float t_outer = (float)(r + 1) / (float)TORCH_GRADIENT_RINGS;

        // Normalised radii (0..1), scaled at draw-time
        float inner_r = t_inner;
        float outer_r = t_outer;

        // Alpha ramps from 0 (centre, fully transparent) to DARKNESS_ALPHA
        // Use a smoothstep-like curve so the edge is gradual
        float inner_a = t_inner * t_inner * t_inner * DARKNESS_ALPHA; // cubic = stays transparent longer
        float outer_a = t_outer * t_outer * t_outer * DARKNESS_ALPHA;

        g_torchRing[r] = MakeTorchRing(inner_r, outer_r, inner_a, outer_a);

        if (!g_torchRing[r])
            printf("LightSystem_Init: failed to create torch ring %d!\n", r);
    }

    printf("LightSystem_Init: %d torch rings created.\n", TORCH_GRADIENT_RINGS);
}

// ---------------------------------------------------------------------------
// PRIVATE - Draw one darkness rectangle helper
// ---------------------------------------------------------------------------
static void DrawDarknessRect(float centre_x, float centre_y,
    float width, float height)
{
    AEMtx33 scale, trans, xf;
    AEMtx33Scale(&scale, width, height);
    AEMtx33Trans(&trans, centre_x, centre_y);
    AEMtx33Concat(&xf, &trans, &scale);
    AEGfxSetTransform(xf.m);
    AEGfxMeshDraw(g_darknessMesh, AE_GFX_MDM_TRIANGLES);
}

// ---------------------------------------------------------------------------
// LightSystem_DrawDarkness
// ---------------------------------------------------------------------------
// Covers the ENTIRE map with darkness EXCEPT the safe-zone band
// (LIGHT_SAFEZONE_Y_MIN to LIGHT_SAFEZONE_Y_MAX).
//
// Two rectangles are drawn every frame:
//   - "upper rect": from the map top down to LIGHT_SAFEZONE_Y_MAX
//   - "lower rect": from LIGHT_SAFEZONE_Y_MIN down to the map bottom
//
// The gap between the two rects is exactly the safe-zone, so it stays lit.
// ---------------------------------------------------------------------------
void LightSystem_DrawDarkness(float camera_x, float camera_y,
    float player_x, float player_y)
{
    if (!g_darknessMesh) return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(DARKNESS_ALPHA);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // Map world extents
    float map_world_w = (float)LIGHT_MAP_WIDTH * LIGHT_TILE_SIZE;
    float map_world_h = (float)LIGHT_MAP_HEIGHT * LIGHT_TILE_SIZE;
    float map_top = (map_world_h * 0.5f);   // world Y at top of map
    float map_bottom = -(map_world_h * 0.5f);   // world Y at bottom of map

    // Wide enough to cover the playable area + side blackout panels
    float darkness_w = map_world_w + 500.0f;

    // ------------------------------------------------------------------
    // Upper rect: map top  -->  safe-zone top (LIGHT_SAFEZONE_Y_MAX)
    // ------------------------------------------------------------------
    float upper_top = map_top;
    float upper_bottom = LIGHT_SAFEZONE_Y_MAX;
    float upper_h = upper_top - upper_bottom;          // positive value
    float upper_cy = upper_bottom + upper_h * 0.5f;

    if (upper_h > 0.0f)
        DrawDarknessRect(0.0f, upper_cy, darkness_w, upper_h);

    // ------------------------------------------------------------------
    // Lower rect: safe-zone bottom (-3200.0f)  -->  map bottom
    // ------------------------------------------------------------------
    float lower_top = -3200.0f;
    float lower_bottom = map_bottom;
    float lower_h = lower_top - lower_bottom;          // positive value
    float lower_cy = lower_bottom + lower_h * 0.5f;

    if (lower_h > 0.0f)
        DrawDarknessRect(0.0f, lower_cy, darkness_w, lower_h);
}

// ---------------------------------------------------------------------------
// LightSystem_DrawTorch
// ---------------------------------------------------------------------------
void LightSystem_DrawTorch(float camera_x, float camera_y,
    float player_x, float player_y,
    float torch_radius)
{
    // Only draw the torch when the player is in a darkened region.
    // The safe-zone band is fully lit, so no torch needed there.
    // Safe zone: y between -3200.0f (bottom) and -2530.0f (top)
    if (player_y <= LIGHT_SAFEZONE_Y_MAX && player_y >= -3200.0f)
        return;

    AEGfxSetCamPosition(camera_x, camera_y);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransparency(1.0f);

    for (int r = 0; r < TORCH_GRADIENT_RINGS; ++r)
    {
        if (!g_torchRing[r]) continue;

        AEMtx33 scale, trans, xf;
        AEMtx33Scale(&scale, torch_radius, torch_radius);
        AEMtx33Trans(&trans, player_x, player_y);
        AEMtx33Concat(&xf, &trans, &scale);
        AEGfxSetTransform(xf.m);

        AEGfxMeshDraw(g_torchRing[r], AE_GFX_MDM_TRIANGLES);
    }
}

// ---------------------------------------------------------------------------
// LightSystem_Kill
// ---------------------------------------------------------------------------
void LightSystem_Kill(void)
{
    if (g_darknessMesh)
    {
        AEGfxMeshFree(g_darknessMesh);
        g_darknessMesh = NULL;
    }

    for (int r = 0; r < TORCH_GRADIENT_RINGS; ++r)
    {
        if (g_torchRing[r])
        {
            AEGfxMeshFree(g_torchRing[r]);
            g_torchRing[r] = NULL;
        }
    }

    printf("LightSystem_Kill: all resources freed.\n");
}