// ---------------------------------------------------------------------------
// includes

#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"

// HELPER FUNCTIONS

// Collision detection function
int CheckCollisionAABB(float x1, float y1, float width1, float height1,
	float x2, float y2, float width2, float height2)
{
	// Calculate the half-widths and half-heights
	float halfWidth1 = width1 / 2.0f;
	float halfHeight1 = height1 / 2.0f;
	float halfWidth2 = width2 / 2.0f;
	float halfHeight2 = height2 / 2.0f;

	// Check if rectangles overlap on both X and Y
	if (x1 - halfWidth1 < x2 + halfWidth2 &&
		x1 + halfWidth1 > x2 - halfWidth2 &&
		y1 - halfHeight1 < y2 + halfHeight2 &&
		y1 + halfHeight1 > y2 - halfHeight2)
	{
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// main

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	int gGameRunning = 1;

	float blue_X = 0.0f;
	float blue_Y = 0.0f;
	float speed = 10.0f;
	float blue_width = 30.0f * 2.0f;
	float blue_height = 30.0f * 2.0f;

	float max_health = 100.0f;
	float current_health = 100.0f;
	float drain_rate = 0.3f;

	float red_X = 400.0f;
	float red_Y = 0.0f;
	float red_width = 200.0f * 2.0f;
	float red_height = 200.0f * 2.0f;

	float green_X = -400.0f;
	float green_Y = 0.0f;
	float green_width = 200.0f * 2.0f;
	float green_height = 200.0f * 2.0f;

	const int num_hp_segments = 10;
	float segment_spacing = 80.0f;

	// Using custom window procedure
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);

	// Changing the window title
	AESysSetWindowTitle("Solo Project 1!");

	//Create Mesh for Green Rectangle
	AEGfxMeshStart();
	AEGfxTriAdd(
		-100.0f, -100.0f, 0xFF00FF00, 0.0f, 1.0f,
		100.0f, -100.0f, 0xFF00FF00, 1.0f, 1.0f,
		-100.0f, 100.0f, 0xFF00FF00, 0.0f, 0.0f);
	AEGfxTriAdd(
		100.0f, -100.0f, 0xFF00FF00, 1.0f, 1.0f,
		100.0f, 100.0f, 0xFF00FF00, 1.0f, 0.0f,
		-100.0f, 100.0f, 0xFF00FF00, 0.0f, 0.0f);

	AEGfxVertexList* gRectMesh = AEGfxMeshEnd();

	// Create a scale matrix 
	AEMtx33 scalegRect = { 0 };
	AEMtx33Scale(&scalegRect, 2.0f, 2.0f);
	// Create a rotation matrix 
	AEMtx33 rotategRect = { 0 };
	AEMtx33Rot(&rotategRect, 0);
	// Create a translation matrix
	AEMtx33 translategRect = { 0 };
	AEMtx33Trans(&translategRect, -400.0f, 0.0f);

	AEMtx33 transformgRect = { 0 };
	AEMtx33Concat(&transformgRect, &rotategRect, &scalegRect);
	AEMtx33Concat(&transformgRect, &translategRect, &transformgRect);

	//Create Mesh for Red Rectangle
	AEGfxMeshStart();
	AEGfxTriAdd(
		-100.0f, -100.0f, 0xEEFF0000, 0.0f, 1.0f,
		100.0f, -100.0f, 0xEEFF0000, 1.0f, 1.0f,
		-100.0f, 100.0f, 0xEEFF0000, 0.0f, 0.0f);
	AEGfxTriAdd(
		100.0f, -100.0f, 0xEEFF0000, 1.0f, 1.0f,
		100.0f, 100.0f, 0xEEFF0000, 1.0f, 0.0f,
		-100.0f, 100.0f, 0xEEFF0000, 0.0f, 0.0f);

	AEGfxVertexList* rRectMesh = AEGfxMeshEnd();

	// Create a scale matrix 
	AEMtx33 scalerRect = { 0 };
	AEMtx33Scale(&scalerRect, 2.0f, 2.0f);

	// Create a rotation matrix 
	AEMtx33 rotaterRect = { 0 };
	AEMtx33Rot(&rotaterRect, 0);

	// Create a translation matrix
	AEMtx33 translaterRect = { 0 };
	AEMtx33Trans(&translaterRect, 400.0f, 0.0f);

	// Concatenate the matrices into the 'transform' variable.
	// We concatenate in the order of translation * rotation * scale
	// i.e. this means we scale, then rotate, then translate.
	AEMtx33 transformrRect = { 0 };
	AEMtx33Concat(&transformrRect, &rotaterRect, &scalerRect);
	AEMtx33Concat(&transformrRect, &translaterRect, &transformrRect);

	//Create Mesh for Player Rectangle
	AEGfxMeshStart();
	AEGfxTriAdd(
		-15.0f, -15.0f, 0xFF0000FF, 0.0f, 1.0f,
		15.0f, -15.0f, 0xFF0000FF, 1.0f, 1.0f,
		-15.0f, 15.0f, 0xFF0000FF, 0.0f, 0.0f);
	AEGfxTriAdd(
		15.0f, -15.0f, 0xFF0000FF, 1.0f, 1.0f,
		15.0f, 15.0f, 0xFF0000FF, 1.0f, 0.0f,
		-15.0f, 15.0f, 0xFF0000FF, 0.0f, 0.0f);

	AEGfxVertexList* pRectMesh = AEGfxMeshEnd();

	//Create Mesh for HP Background Bar
	AEGfxMeshStart();
	AEGfxTriAdd(
		-330.0f, -15.0f, 0xFFAA0000, 0.0f, 1.0f,
		330.0f, -15.0f, 0xFFAA0000, 1.0f, 1.0f,
		-330.0f, 15.0f, 0xFFAA0000, 0.0f, 0.0f);
	AEGfxTriAdd(
		330.0f, -15.0f, 0xFFAA0000, 1.0f, 1.0f,
		330.0f, 15.0f, 0xFFAA0000, 1.0f, 0.0f,
		-330.0f, 15.0f, 0xFFAA0000, 0.0f, 0.0f);

	AEGfxVertexList* pMesh = AEGfxMeshEnd();

	// Create a scale matrix for HP background
	AEMtx33 scale = { 0 };
	AEMtx33Scale(&scale, 2.0f, 2.0f);

	// Create a rotation matrix 
	AEMtx33 rotate = { 0 };
	AEMtx33Rot(&rotate, 0);

	// Create a translation matrix
	AEMtx33 translate = { 0 };
	AEMtx33Trans(&translate, 0, 330.0f);

	// Concatenate the matrices into the 'transform' variable.
	// We concatenate in the order of translation * rotation * scale
	// i.e. this means we scale, then rotate, then translate.
	AEMtx33 transform = { 0 };
	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);

	//Create Mesh for HP Bar
	AEGfxMeshStart();
	AEGfxTriAdd(
		-330.0f, -15.0f, 0xFFFF0000, 0.0f, 1.0f,
		330.0f, -15.0f, 0xFFFF0000, 1.0f, 1.0f,
		-330.0f, 15.0f, 0xFFFF0000, 0.0f, 0.0f);
	AEGfxTriAdd(
		330.0f, -15.0f, 0xFFFF0000, 1.0f, 1.0f,
		330.0f, 15.0f, 0xFFFF0000, 1.0f, 0.0f,
		-330.0f, 15.0f, 0xFFFF0000, 0.0f, 0.0f);

	AEGfxVertexList* HPMesh = AEGfxMeshEnd();

	AEGfxMeshStart();
	AEGfxTriAdd(
		-30.0f, -30.0f, 0xFFFF0000, 0.0f, 1.0f,
		30.0f, -30.0f, 0xFFFF0000, 1.0f, 1.0f,
		-30.0f, 30.0f, 0xFFFF0000, 0.0f, 0.0f);
	AEGfxTriAdd(
		30.0f, -30.0f, 0xFFFF0000, 1.0f, 1.0f,
		30.0f, 30.0f, 0xFFFF0000, 1.0f, 0.0f,
		-30.0f, 30.0f, 0xFFFF0000, 0.0f, 0.0f);

	AEGfxVertexList* HPSegmentMesh = AEGfxMeshEnd();

	// Game Loop
	while (gGameRunning)
	{

		// Informing the system about the loop's start
		AESysFrameStart();

		// Input handling (Blue Rectangle Movement)
		if (AEInputCheckCurr(AEVK_W)) {
			blue_Y += speed;
		}
		else if (AEInputCheckCurr(AEVK_A)) {
			blue_X -= speed;
		}
		else if (AEInputCheckCurr(AEVK_S)) {
			blue_Y -= speed;
		}
		else if (AEInputCheckCurr(AEVK_D)) {
			blue_X += speed;
		}

		// Collision Detection with Red Rectangle
		int isColliding_withRed = CheckCollisionAABB(blue_X, blue_Y, blue_width, blue_height, red_X, red_Y, red_width, red_height);
		if (isColliding_withRed) {
			current_health -= drain_rate;
			if (current_health < 0.0f) {
				current_health = 0.0f;
			}
		}

		int isColliding_withGreen = CheckCollisionAABB(blue_X, blue_Y, blue_width, blue_height, green_X, green_Y, green_width, green_height);
		if (isColliding_withGreen) {
			current_health += drain_rate;
			if (current_health > 100.0f) {
				current_health = 100.0f;
			}
		}

		// TRANSFORMATION FOR BLUE RECTANGLE WITH UPDATED POSITION
		// Create a scale matrix 
		AEMtx33 scalepRect = { 0 };
		AEMtx33Scale(&scalepRect, 2.0f, 2.0f);

		// Create a rotation matrix 
		AEMtx33 rotatepRect = { 0 };
		AEMtx33Rot(&rotatepRect, 0);

		// Create a translation matrix
		AEMtx33 translatepRect = { 0 };
		AEMtx33Trans(&translatepRect, blue_X, blue_Y);

		// Concatenate the matrices into the 'transform' variable.
		// We concatenate in the order of translation * rotation * scale
		// i.e. this means we scale, then rotate, then translate.
		AEMtx33 transformpRect = { 0 };
		AEMtx33Concat(&transformpRect, &rotatepRect, &scalepRect);
		AEMtx33Concat(&transformpRect, &translatepRect, &transformpRect);

		// TRANSFORMATION FOR HP BAR
		float hp_percentage = current_health / max_health;
		int hp_segments = (int)((current_health / max_health) * num_hp_segments);

		if (current_health > 0.0f && hp_segments == 0) {
			hp_segments = 1;
		}

		// Create a scale matrix 
		AEMtx33 scaleHP = { 0 };
		AEMtx33Scale(&scaleHP, 2.0f * hp_percentage, 2.0f);

		// Create a rotation matrix 
		AEMtx33 rotateHP = { 0 };
		AEMtx33Rot(&rotateHP, 0);

		// Create a translation matrix
		float hp_bar_offset = -330.0f * (1.0f - hp_percentage) * 2.0f;
		AEMtx33 translateHP = { 0 };
		AEMtx33Trans(&translateHP, hp_bar_offset, 330.0f);

		// Concatenate the matrices into the 'transform' variable.
		// We concatenate in the order of translation * rotation * scale
		// i.e. this means we scale, then rotate, then translate.
		AEMtx33 transformHP = { 0 };
		AEMtx33Concat(&transformHP, &rotateHP, &scaleHP);
		AEMtx33Concat(&transformHP, &translateHP, &transformHP);


		// RENDERING
		// Set background to gray
		AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);

		// Tell the Alpha Engine to get ready to draw something with colour.
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);

		// Set the the color to multiply to white, so that the sprite can 
		// display the full range of colors (default is black).
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

		// Set the color to add to nothing, so that we don't alter the sprite's color
		AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

		// Set blend mode to AE_GFX_BM_BLEND, which will allow transparency.
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(1.0f);

		AEGfxSetTransform(transformgRect.m);
		AEGfxMeshDraw(gRectMesh, AE_GFX_MDM_TRIANGLES);
		AEGfxSetTransform(transformrRect.m);
		AEGfxMeshDraw(rRectMesh, AE_GFX_MDM_TRIANGLES);
		AEGfxSetTransform(transformpRect.m);
		AEGfxMeshDraw(pRectMesh, AE_GFX_MDM_TRIANGLES);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
		AEGfxSetTransform(transformHP.m);
		AEGfxMeshDraw(HPMesh, AE_GFX_MDM_TRIANGLES);

		// Calculate the total width of all segments including spacing
		float total_segments_width = (60.0f * num_hp_segments) + (segment_spacing * (num_hp_segments - 1));
		// Starting X position (leftmost segment position)
		float segment_start_x = -total_segments_width / 2.0f + 30.0f;
		// Below HP Bar
		float segments_Y = 260.0f;

		// Draw each visible segment
		for (int i = 0; i < hp_segments; i++)
		{
			// Calculate X position for this segment
			float segment_x = segment_start_x + (i * (60.0f + segment_spacing));

			// Create transform for this segment
			AEMtx33 scaleSegment = { 0 };
			AEMtx33Scale(&scaleSegment, 1.0f, 1.0f);

			AEMtx33 rotateSegment = { 0 };
			AEMtx33Rot(&rotateSegment, 0);

			AEMtx33 translateSegment = { 0 };
			AEMtx33Trans(&translateSegment, segment_x, segments_Y);

			AEMtx33 transformSegment = { 0 };
			AEMtx33Concat(&transformSegment, &rotateSegment, &scaleSegment);
			AEMtx33Concat(&transformSegment, &translateSegment, &transformSegment);

			AEGfxSetTransform(transformSegment.m);
			AEGfxMeshDraw(HPSegmentMesh, AE_GFX_MDM_TRIANGLES);
		}

		// Basic way to trigger exiting the application
		// when ESCAPE is hit or when the window is closed
		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
			gGameRunning = 0;



		// Informing the system about the loop's end
		AESysFrameEnd();

	}


	// free the system
	AEGfxMeshFree(gRectMesh);
	AEGfxMeshFree(rRectMesh);
	AEGfxMeshFree(pRectMesh);
	AEGfxMeshFree(pMesh);
	AEGfxMeshFree(HPMesh);
	AESysExit();
}
