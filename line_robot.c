#include "raylib.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define USER_POINTS 5
#define MAX_POINTS 6      // 5 user points + 1 fixed endpoint (10,10)
#define ROBOT_SPEED 5.0f 
#define GRID_SCALE 25.0f  // Scaled down slightly to fit a clean 15x15 layout neatly on screen

typedef struct {
    float x;
    float y;
} GraphPoint;

// Converts a 2D Graph Coordinate to explicit Screen Pixels (Centered on canvas grid)
Vector2 GraphToScreen(GraphPoint pt, int screenWidth, int screenHeight) {
    Vector2 screenPos;
    screenPos.x = (screenWidth / 2.0f) + (pt.x * GRID_SCALE);
    screenPos.y = (screenHeight / 2.0f) - (pt.y * GRID_SCALE); 
    return screenPos;
}

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Raylib - 15x15 Graph Line Follower");

    GraphPoint graphPoints[MAX_POINTS];
    int userPointCount = 0;
    
    // Typing state management buffers
    char inputBuffer[16] = "\0";
    int letterCount = 0;
    bool typingX = true; 
    float tempX = 0.0f;
    char errorMessage[64] = "\0";

    GraphPoint robotGraphPos = { 0, 0 };
    int currentTargetIdx = 1;
    bool isSimulating = false;
    bool pathComplete = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        
        // --- 1. GRAPHICAL TEXT INPUT LOGIC WITH ERROR LIMIT CHECKING ---
        if (userPointCount < USER_POINTS) {
            int key = GetCharPressed();
            
            while (key > 0) {
                if (((key >= '0') && (key <= '9')) || (key == '.') || (key == '-')) {
                    if (letterCount < 15) {
                        inputBuffer[letterCount] = (char)key;
                        letterCount++;
                        inputBuffer[letterCount] = '\0';
                        errorMessage[0] = '\0'; // Clear error on typing
                    }
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                letterCount--;
                if (letterCount < 0) letterCount = 0;
                inputBuffer[letterCount] = '\0';
            }

            if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                float value = (float)atof(inputBuffer);
                
                // Bounding constraint logic: Values must be strictly less than 10
                if (value >= 10.0f) {
                    snprintf(errorMessage, sizeof(errorMessage), "Value must be less than 10!");
                    inputBuffer[0] = '\0';
                    letterCount = 0;
                } else {
                    if (typingX) {
                        tempX = value;
                        typingX = false; 
                    } else {
                        graphPoints[userPointCount].x = tempX;
                        graphPoints[userPointCount].y = value;
                        userPointCount++;
                        typingX = true; 
                        
                        // Once 5 user points are in, lock point 6 as the fixed endpoint (10,10)
                        if (userPointCount == USER_POINTS) {
                            graphPoints[USER_POINTS].x = 10.0f;
                            graphPoints[USER_POINTS].y = 10.0f;
                            
                            robotGraphPos = graphPoints[0];
                            isSimulating = true;
                        }
                    }
                    inputBuffer[0] = '\0';
                    letterCount = 0;
                }
            }
        }

        // Reset system space
        if (IsKeyPressed(KEY_R)) {
            userPointCount = 0;
            isSimulating = false;
            pathComplete = false;
            currentTargetIdx = 1;
            typingX = true;
            inputBuffer[0] = '\0';
            letterCount = 0;
            errorMessage[0] = '\0';
        }

        // --- 2. TRAJECTORY LOGIC ---
        if (isSimulating) {
            GraphPoint target = graphPoints[currentTargetIdx];
            
            float dx = target.x - robotGraphPos.x;
            float dy = target.y - robotGraphPos.y;
            float distance = sqrtf(dx * dx + dy * dy);

            if (distance > 0.05f) {
                float dt = GetFrameTime();
                robotGraphPos.x += (dx / distance) * ROBOT_SPEED * dt;
                robotGraphPos.y += (dy / distance) * ROBOT_SPEED * dt;
            } else {
                robotGraphPos = target; 
                currentTargetIdx++;
                
                if (currentTargetIdx >= MAX_POINTS) {
                    isSimulating = false;
                    pathComplete = true;
                }
            }
        }

        // --- 3. RENDERING ENGINE ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        int halfWidth = screenWidth / 2;
        int halfHeight = screenHeight / 2;

        // Draw clean, explicit 15x15 graph grid system boundaries (-15 to 15 grid ranges)
        for (int i = -15; i <= 15; i++) {
            // Vertical line slices
            float screenX = halfWidth + (i * GRID_SCALE);
            DrawLine(screenX, halfHeight - (15 * GRID_SCALE), screenX, halfHeight + (15 * GRID_SCALE), LIGHTGRAY);
            
            // Horizontal line slices
            float screenY = halfHeight - (i * GRID_SCALE);
            DrawLine(halfWidth - (15 * GRID_SCALE), screenY, halfWidth + (15 * GRID_SCALE), screenY, LIGHTGRAY);
            
            // Numerical index coordinate stamps along axes
            if (i % 5 == 0 && i != 0) {
                DrawText(TextFormat("%d", i), screenX - 5, halfHeight + 5, 10, DARKGRAY);
                DrawText(TextFormat("%d", i), halfWidth + 5, screenY - 5, 10, DARKGRAY);
            }
        }

        // Thick central core Axes reference lines
        DrawLine(halfWidth - (15 * GRID_SCALE), halfHeight, halfWidth + (15 * GRID_SCALE), halfHeight, GRAY); 
        DrawLine(halfWidth, halfHeight - (15 * GRID_SCALE), halfWidth, halfHeight + (15 * GRID_SCALE), GRAY); 

        // Vector track lines (always draws towards the fixed endpoint)
        int totalRenderPoints = (userPointCount < USER_POINTS) ? userPointCount : MAX_POINTS;
        if (totalRenderPoints > 1) {
            for (int i = 0; i < totalRenderPoints - 1; i++) {
                Vector2 p1 = GraphToScreen(graphPoints[i], screenWidth, screenHeight);
                Vector2 p2 = GraphToScreen(graphPoints[i + 1], screenWidth, screenHeight);
                DrawLineEx(p1, p2, 4.0f, DARKBLUE);
            }
        }

        // Render user registered waypoints
        for (int i = 0; i < userPointCount; i++) {
            Vector2 screenPos = GraphToScreen(graphPoints[i], screenWidth, screenHeight);
            DrawCircleV(screenPos, 8, MAROON);
            DrawText(TextFormat("P%d(%.1f, %.1f)", i + 1, graphPoints[i].x, graphPoints[i].y), screenPos.x + 12, screenPos.y - 10, 12, DARKGRAY);
        }

        // Render static target destination endpoint node at (10, 10)
        GraphPoint endTarget = { 10.0f, 10.0f };
        Vector2 endScreenPos = GraphToScreen(endTarget, screenWidth, screenHeight);
        DrawCircleV(endScreenPos, 10, RED);
        DrawText("END (10, 10)", endScreenPos.x + 12, endScreenPos.y - 10, 13, RED);

        // Animated Robot Sprite Node
        if (isSimulating || pathComplete) {
            Vector2 robotScreenPos = GraphToScreen(robotGraphPos, screenWidth, screenHeight);
            DrawCircleV(robotScreenPos, 14, DARKGREEN);
            DrawCircleV(robotScreenPos, 10, LIME);
            DrawText(TextFormat("Robot: (%.1f, %.1f)", robotGraphPos.x, robotGraphPos.y), robotScreenPos.x - 40, robotScreenPos.y - 30, 13, BLACK);
        }

        // Interface Instruction Panels
        if (userPointCount < USER_POINTS) {
            DrawRectangle(15, 15, 420, 130, Fade(SKYBLUE, 0.5f));
            DrawRectangleLines(15, 15, 420, 130, BLUE);
            
            DrawText(TextFormat("Entering Point %d of %d (Must be < 10)", userPointCount + 1, USER_POINTS), 30, 25, 18, DARKGRAY);
            DrawText(TextFormat("Type value for %s: %s", typingX ? "X" : "Y", inputBuffer), 30, 55, 18, BLACK);
            DrawText("Press [ENTER] to confirm value", 30, 85, 14, DARKGRAY);
            
            if (errorMessage[0] != '\0') {
                DrawText(errorMessage, 30, 110, 14, RED);
            }
        } else if (isSimulating) {
            DrawText("Robot tracing trajectory lines to End Point...", 20, 20, 20, DARKGREEN);
        } else if (pathComplete) {
            DrawText("End Target Reached! Press 'R' to reset.", 20, 20, 20, GOLD);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
