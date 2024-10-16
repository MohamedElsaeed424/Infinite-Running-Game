#include <ctime>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include "TextureBuilder.h"
#include <glut.h>
#include <windows.h>
#include <mmsystem.h>
#include <chrono>

#define M_PI 3.14159265358979323846

enum PowerUpType { SPEED_BOOST, INVINCIBILITY };
GLuint backgroundTexture;
GLuint obstacleTexture;
GLuint playerTexture;
GLuint cloudTexture;
struct GameObject {
    float x, y;
    bool active;
    int type; // 0: obstacle, 1: collectable, 2: power-up
};
std::vector<GameObject> gameObjects;
std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
int score = 0;
float playerY = 50.0f;
float playerVelocityY = 0.0f;
int playerLives = 5;
float gravity = -0.5f;
bool isJumping = false;
const float jumpStrength = 12.0f;
bool isDucking = false;
float gameTime = 60.0f;
float gameSpeed = 2.0f;
bool gameRunning = true;
float backgroundOffset = 0.0f;
const float scrollSpeed = 2.0f;
bool isBackgroundMusicPlaying = false;
int rep = 1;
PowerUpType activePowerUp = SPEED_BOOST;
float powerUpDuration = 0.0f;
static float cloudOffset = 1000.0f;
float backgroundColor[3] = { 1.0f, 1.0f, 1.0f };
bool isFlashing = false;
int flashCounter = 0;
const int maxFlashes = 10;  // Number of frames to flash
bool flashRed = true;  // Toggles between red and white

static float cloudOffsets[3] = { 1000.0f, 1200.0f, 1400.0f };  // Start offsets for clouds
float cloudSpeeds[3] = { 10.0f, 10.0f, 13.0f };  // Speeds for the clouds
float cloudYPositions[3] = { 500.0f, 400.0f, 300.0f };  // Y positions for each cloud

static float messageOpacity = 0.0f;  // Control the fading effect for messages
bool isGameOver = false;  // Track game over state
bool isGameWon = false;   // Track game won state


void drawClouds();
void updateCloudPositions();
void updatePlayerPosition();
LPCWSTR convertToLPCWSTR(const char* charArray);
void playSound(const char* filename, bool loop);
void drawPlayer();
void drawBoundaries();
void updateGameObjects();
void drawGameObjects();
void drawHealth();
void handleKeyPress(unsigned char key, int x, int y);
void handleKeyRelease(unsigned char key, int x, int y);
void update(int value);
void render();
void initGame();
void drawText(float x, float y, std::string text);
void spawnGameObject(int type);
void handleFlashing();
void drawFancyText(float x, float y, std::string text, float scale);

LPCWSTR convertToLPCWSTR(const char* charArray) {
    size_t newSize = strlen(charArray) + 1;
    wchar_t* wString = new wchar_t[newSize];
    mbstowcs(wString, charArray, newSize); // Convert to wide character
    return wString;
}
void playSound(const char* filename, bool loop) {
    LPCWSTR wideFilename = convertToLPCWSTR(filename);

    if (loop && !isBackgroundMusicPlaying) {
        // Play the background music in loop if it's not already playing
        PlaySound(wideFilename, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        isBackgroundMusicPlaying = true; // Set the flag that music is playing
    }
    else if (!loop) {
        // Play the sound effect without looping
        PlaySound(wideFilename, NULL, SND_FILENAME | SND_ASYNC);
    }

    delete[] wideFilename; // Clean up memory
}


void handleKeyPress(unsigned char key, int x, int y) {
    if (key == ' ' && !isJumping) {
        playSound("sounds/jump.wav",false);
        isJumping = true;
        playerVelocityY = jumpStrength;
    }

    if (key == 's') {
        isDucking = true;
    }
}
void handleKeyRelease(unsigned char key, int x, int y) {
    if (key == 's') {
        isDucking = false;
    }
}

void drawClouds() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, cloudTexture);
    glColor3f(1.0f, 1.0f, 1.0f);  // Ensure cloud color is white for proper texture rendering

    // Draw 3 clouds with different positions
    for (int i = 0; i < 3; i++) {
        glPushMatrix();

        // Translate each cloud based on its offset and vertical position
        glTranslatef(cloudOffsets[i], cloudYPositions[i], 0.0f);

        // Draw the cloud as a quad with texture
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-50, -30);  // Bottom-left corner
        glTexCoord2f(1.0f, 0.0f); glVertex2f(50, -30);   // Bottom-right corner
        glTexCoord2f(1.0f, 1.0f); glVertex2f(50, 30);    // Top-right corner
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-50, 30);   // Top-left corner
        glEnd();

        glPopMatrix();
    }

    // Disable texturing
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
void drawBoundaries() {
    // Upper boundary (4 different primitives)
    const float lightGray[3] = { 0.8f, 0.8f, 0.8f };
    const float mediumGray[3] = { 0.6f, 0.6f, 0.6f };
    const float darkGray[3] = { 0.4f, 0.4f, 0.4f };

    // 1. GL_TRIANGLE_STRIP for solid base with shading
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= WINDOW_WIDTH; i += 20) {
        float t = (float)i / WINDOW_WIDTH;
        float shade = 0.6f + 0.2f * sin(t * M_PI); // Subtle shading effect
        glColor3f(shade, shade, shade);
        glVertex2f(i, WINDOW_HEIGHT - 20 + 5 * sin(i * 0.05f));
        glColor3f(shade * 0.8f, shade * 0.8f, shade * 0.8f);
        glVertex2f(i, WINDOW_HEIGHT - 40);
    }
    glEnd();

    // 2. GL_POINTS for subtle texture
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < WINDOW_WIDTH; i += 10) {
        float t = (float)i / WINDOW_WIDTH;
        float alpha = 0.3f + 0.2f * sin(t * M_PI * 2); // Varying transparency
        glColor4f(lightGray[0], lightGray[1], lightGray[2], alpha);
        glVertex2f(i, WINDOW_HEIGHT - 30 + 3 * sin(i * 0.1f));
    }
    glEnd();
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);

    // 3. GL_LINE_STRIP for subtle curve
    glBegin(GL_LINE_STRIP);
    glColor3f(darkGray[0], darkGray[1], darkGray[2]);
    for (int i = 0; i <= WINDOW_WIDTH; i += 10) {
        float y = WINDOW_HEIGHT - 35 + 3 * sin(i * 0.03f);
        glVertex2f(i, y);
    }
    glEnd();

    // 4. GL_TRIANGLES for subtle top texture
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < WINDOW_WIDTH; i += 40) {
        float t = (float)i / WINDOW_WIDTH;
        float shade = 0.7f + 0.1f * sin(t * M_PI * 2); // Subtle shading
        glColor3f(shade, shade, shade);
        glVertex2f(i, WINDOW_HEIGHT - 20);
        glVertex2f(i + 20, WINDOW_HEIGHT - 15);
        glVertex2f(i + 40, WINDOW_HEIGHT - 20);
    }
    glEnd();

    // Lower boundary (4 different primitives)

    const float baseGreen[3] = { 0.55f, 0.27f, 0.07f };  // Base brown (medium brown, like earth)
    const float darkGreen[3] = { 0.36f, 0.25f, 0.20f };  // Darker brown for shadows
    const float lightGreen[3] = { 0.76f, 0.60f, 0.42f }; // Lighter brown for highlights


    // 1. GL_QUAD_STRIP for solid base
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= WINDOW_WIDTH; i += 20) {
        glColor3f(baseGreen[0], baseGreen[1], baseGreen[2]);
        glVertex2f(i, 30);
        glColor3f(darkGreen[0], darkGreen[1], darkGreen[2]);
        glVertex2f(i, 0);
    }
    glEnd();

    // 2. GL_LINES for subtle texture
    glBegin(GL_LINES);
    glColor3f(lightGreen[0], lightGreen[1], lightGreen[2]);
    for (int i = 0; i < WINDOW_WIDTH; i += 40) {
        glVertex2f(i, 0);
        glVertex2f(i + 20, 30);
    }
    glEnd();

    // 3. GL_TRIANGLES for subtle shapes
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < WINDOW_WIDTH; i += 100) {
        glColor3f(darkGreen[0], darkGreen[1], darkGreen[2]);
        glVertex2f(i, 0);
        glColor3f(baseGreen[0], baseGreen[1], baseGreen[2]);
        glVertex2f(i + 25, 20);
        glColor3f(darkGreen[0], darkGreen[1], darkGreen[2]);
        glVertex2f(i + 50, 0);
    }
    glEnd();

    // 4. GL_TRIANGLE_FAN for subtle circular details
    for (int i = 0; i < WINDOW_WIDTH; i += 200) {
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(lightGreen[0], lightGreen[1], lightGreen[2]);
        glVertex2f(i + 50, 15);  // center
        int segments = 16;
        for (int j = 0; j <= segments; j++) {
            float theta = 2.0f * M_PI * (float)j / (float)segments;
            float x = i + 50 + 10 * cosf(theta);
            float y = 15 + 10 * sinf(theta);
            if (j % 2 == 0) {
                glColor3f(baseGreen[0], baseGreen[1], baseGreen[2]);
            }
            else {
                glColor3f(darkGreen[0], darkGreen[1], darkGreen[2]);
            }
            glVertex2f(x, y);
        }
        glEnd();
    }
}
void drawHealth() {
    for (int i = 0; i < playerLives; ++i) {
        glPushMatrix();
        glTranslatef(50 + i * 40, WINDOW_HEIGHT - 50, 0);

        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        for (int j = 0; j < 360; j++) {
            float theta = j * 3.14159f / 180.0f;
            glVertex2f(10 * 16 * pow(sin(theta), 3) / 13,
                -10 * (13 * cos(theta) - 5 * cos(2 * theta) - 2 * cos(3 * theta) - cos(4 * theta)) / 13);
        }
        glEnd();

        glPopMatrix();
    }
}
void drawGameObjects() {
    // Iterate through all game objects
    for (const auto& obj : gameObjects) {
        glPushMatrix();
        glTranslatef(obj.x, obj.y, 0);

        if (obj.type == 0) { // Obstacle


            // Ensure you enable texturing
            glEnable(GL_TEXTURE_2D);

            // STEP 1: Render the background
            glColor3f(1.0f, 1.0f, 1.0f);  // White to ensure texture colors are not tinted

            // STEP 2: Render the obstacle on top of the background

            glBindTexture(GL_TEXTURE_2D, obstacleTexture);  // Bind the obstacle texture

            // Drawing the obstacle
            glBegin(GL_TRIANGLES);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-15, -10);  // Left vertex
            glTexCoord2f(1.0f, 0.0f); glVertex2f(15, -10);   // Right vertex
            glTexCoord2f(0.5f, 1.0f); glVertex2f(0, 20);     // Top vertex (centered)
            glEnd();

            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-15, -10);  // Bottom-left vertex
            glTexCoord2f(1.0f, 0.0f); glVertex2f(15, -10);   // Bottom-right vertex
            glTexCoord2f(1.0f, 1.0f); glVertex2f(15, -30);   // Bottom-right (lower)
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-15, -30);  // Bottom-left (lower)
            glEnd();

            // Unbind the obstacle texture
            glBindTexture(GL_TEXTURE_2D, 0);

            // Disable 2D texturing after drawing
            glDisable(GL_TEXTURE_2D);

        }
        else if (obj.type == 1) { // Collectable
            static float collectableRotationAngle = 0.0f;  // Rotation angle
            static float scaleFactor = 1.0f;               // Scaling factor
            static bool scaleUp = true;                    // Track if we are scaling up or down

            // Translate to the collectable's position
            //glPushMatrix();  // Save the current transformation matrix
            //glTranslatef(obj.x, obj.y, 0);

            // Apply scaling
            glScalef(scaleFactor, scaleFactor, 1.0f);

            // Apply rotation (counter-clockwise)
            glRotatef(collectableRotationAngle, 0.0f, 0.0f, -1.0f);

            // Draw the collectable with different primitives
            // Primitive 1: Central square (GL_QUADS)
            glColor3f(1.0f, 0.5f, 0.0f);  // Orange color
            glBegin(GL_QUADS);
            glVertex2f(-10, -10);
            glVertex2f(10, -10);
            glVertex2f(10, 10);
            glVertex2f(-10, 10);
            glEnd();

            // Primitive 2: Cross (GL_LINES)
            glColor3f(0.0f, 1.0f, 0.0f);  // Green color
            glBegin(GL_LINES);
            glVertex2f(0, -15);  // Vertical line
            glVertex2f(0, 15);
            glVertex2f(-15, 0);  // Horizontal line
            glVertex2f(15, 0);
            glEnd();

            // Primitive 3: Triangle at the top (GL_TRIANGLES)
            glColor3f(0.0f, 0.0f, 1.0f);  // Blue color
            glBegin(GL_TRIANGLES);
            glVertex2f(0, 20);    // Top vertex
            glVertex2f(-8, 10);   // Left vertex
            glVertex2f(8, 10);    // Right vertex
            glEnd();

            // Primitive 4: Circular fan (GL_TRIANGLE_FAN)
            glColor3f(1.0f, 1.0f, 0.0f);  // Yellow color
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0, 0);  // Center point
            for (int i = 0; i < 360; i += 36) {
                float theta = i * 3.14159f / 180.0f;
                glVertex2f(10 * cos(theta), 10 * sin(theta));
            }
            glEnd();

            // Update the rotation angle for the collectable
            collectableRotationAngle += 1.0f;
            if (collectableRotationAngle > 360.0f) {
                collectableRotationAngle -= 360.0f;
            }

            // Update the scaling factor to oscillate
            if (scaleUp) {
                scaleFactor += 0.01f;  // Scale up
                if (scaleFactor >= 1.5f) scaleUp = false;  // Reverse direction
            }
            else {
                scaleFactor -= 0.01f;  // Scale down
                if (scaleFactor <= 0.5f) scaleUp = true;  // Reverse direction
            }

        }
        else if (obj.type == 2) { // Power-up
            static float powerUpRotationAngle = 0.0f;
            //glTranslatef(obj.x, obj.y, 0);
            glRotatef(powerUpRotationAngle, 0.0f, 0.0f, 1.0f);

            if (activePowerUp == SPEED_BOOST) {
                glColor3f(0.0f, 1.0f, 1.0f);

                // Drawing the central square
                glBegin(GL_QUADS);
                glVertex2f(-8, -8);
                glVertex2f(8, -8);
                glVertex2f(8, 8);
                glVertex2f(-8, 8);
                glEnd();

                // Drawing the triangles pointing outwards (4 triangles)
                glBegin(GL_TRIANGLES);
                // Top triangle
                glVertex2f(0, 12);
                glVertex2f(-5, 5);
                glVertex2f(5, 5);
                // Bottom triangle
                glVertex2f(0, -12);
                glVertex2f(-5, -5);
                glVertex2f(5, -5);
                // Left triangle
                glVertex2f(-12, 0);
                glVertex2f(-5, 5);
                glVertex2f(-5, -5);
                // Right triangle
                glVertex2f(12, 0);
                glVertex2f(5, 5);
                glVertex2f(5, -5);
                glEnd();

                // Drawing cross lines through the center
                glBegin(GL_LINES);
                glVertex2f(-12, 0);
                glVertex2f(12, 0);
                glVertex2f(0, -12);
                glVertex2f(0, 12);
                glEnd();
            }
            else if (activePowerUp == INVINCIBILITY) {
                // Second Power-up: Hexagon with diagonal lines
                glBegin(GL_POLYGON);
                for (int i = 0; i < 6; i++) {
                    float theta = i * 3.14159f / 3.0f;
                    if (i % 2 == 0) {
                        glColor3f(0.0f, 1.0f, 0.0f);  // Green
                    }
                    else {
                        glColor3f(0.0f, 0.5f, 1.0f);  // Blueish
                    }

                    glVertex2f(10 * cos(theta), 10 * sin(theta));
                }
                glEnd();

                // Diagonal lines
                glColor3f(1.0f, 1.0f, 1.0f);  // White color
                glBegin(GL_LINES);
                glVertex2f(0, -10);
                glVertex2f(0, 10);
                glVertex2f(-8.66f, -5);  // -8.66 = 10 * cos(60 degrees)
                glVertex2f(8.66f, 5);
                glVertex2f(-8.66f, 5);
                glVertex2f(8.66f, -5);
                glEnd();
            }
            powerUpRotationAngle += 1.0f;  // Adjust the speed of rotation here
            if (powerUpRotationAngle > 360.0f) {
                powerUpRotationAngle -= 360.0f;  // Keep angle within the 0-360 degree range
            }
        }

        glPopMatrix();
    }
}
void drawPlayer() {
    glPushMatrix();
    glTranslatef(100, playerY, 0);

    // 1. Draw the body (Blue Overalls)
    glColor3f(0.0f, 0.0f, 1.0f); // Blue color for overalls
    glBegin(GL_QUADS);
    glVertex2f(-10, -10); // Bottom-left
    glVertex2f(10, -10);  // Bottom-right
    glVertex2f(10, 20);   // Top-right
    glVertex2f(-10, 20);  // Top-left
    glEnd();

    // 2. Draw the red shirt (upper part)
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for shirt
    glBegin(GL_QUADS);
    glVertex2f(-10, 10);  // Bottom-left
    glVertex2f(10, 10);   // Bottom-right
    glVertex2f(10, 20);   // Top-right
    glVertex2f(-10, 20);  // Top-left
    glEnd();

    // 3. Draw the head (Face with Hat)
    // Head (Circle)
    glColor3f(0.8f, 0.6f, 0.4f); // Skin color
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 30);  // Center of the circle
    for (int i = 0; i <= 360; i++) {
        float theta = i * 3.14159f / 180.0f;
        glVertex2f(5 * cos(theta), 30 + 5 * sin(theta));
    }
    glEnd();

    // Hat (Red arc)
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for hat
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 33); // Center above the head
    for (int i = 0; i <= 180; i++) { // Draw half circle (arc) for the hat
        float theta = i * 3.14159f / 180.0f;
        glVertex2f(5 * cos(theta), 30 + 5 * sin(theta));
    }
    glEnd();

    // 4. Draw the eyes
    glColor3f(0.0f, 0.0f, 0.0f); // Black eyes
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    glVertex2f(-2, 32);  // Left eye
    glVertex2f(2, 32);   // Right eye
    glEnd();

    // 5. Draw the mustache (Black line)
    glColor3f(0.0f, 0.0f, 0.0f); // Black for mustache
    glBegin(GL_LINES);
    glVertex2f(-3, 28);  // Left side of mustache
    glVertex2f(3, 28);   // Right side of mustache
    glEnd();

    // 6. Draw the arms (Rectangles for simplicity)
    // Left arm (Red for shirt)
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for shirt
    glBegin(GL_QUADS);
    glVertex2f(-12, 10); // Bottom-left
    glVertex2f(-10, 10); // Bottom-right
    glVertex2f(-10, 0);  // Top-right
    glVertex2f(-12, 0);  // Top-left
    glEnd();

    // Right arm
    glBegin(GL_QUADS);
    glVertex2f(10, 10);  // Bottom-left
    glVertex2f(12, 10);  // Bottom-right
    glVertex2f(12, 0);   // Top-right
    glVertex2f(10, 0);   // Top-left
    glEnd();

    // 7. Draw the legs (Rectangles for legs)
    glColor3f(0.0f, 0.0f, 1.0f); // Blue color for overalls/legs
    glBegin(GL_QUADS);
    glVertex2f(-10, -20);  // Bottom-left
    glVertex2f(-5, -20);   // Bottom-right
    glVertex2f(-5, -10);   // Top-right
    glVertex2f(-10, -10);  // Top-left
    glEnd();

    glBegin(GL_QUADS);
    glVertex2f(5, -20);    // Bottom-left
    glVertex2f(10, -20);   // Bottom-right
    glVertex2f(10, -10);   // Top-right
    glVertex2f(5, -10);    // Top-left
    glEnd();

    // 8. Draw the feet (brown shoes)
    glColor3f(0.4f, 0.2f, 0.0f); // Brown color for shoes
    glBegin(GL_QUADS);
    glVertex2f(-10, -25);  // Bottom-left shoe
    glVertex2f(-5, -25);   // Bottom-right shoe
    glVertex2f(-5, -20);   // Top-right shoe
    glVertex2f(-10, -20);  // Top-left shoe
    glEnd();

    glBegin(GL_QUADS);
    glVertex2f(5, -25);    // Bottom-left shoe
    glVertex2f(10, -25);   // Bottom-right shoe
    glVertex2f(10, -20);   // Top-right shoe
    glVertex2f(5, -20);    // Top-left shoe
    glEnd();

    glPopMatrix();
}
void drawText(float x, float y, std::string text) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}
void drawFancyText(float x, float y, std::string text, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);  // Scale the text to make it larger
    drawText(0, 0, text);
    glPopMatrix();
}


void handleFlashing() {
    if (isFlashing) {
        if (flashCounter < maxFlashes) {
            if (flashCounter % 2 == 0) {
                // Flash red
                backgroundColor[0] = 1.0f;  // Red
                backgroundColor[1] = 0.0f;  // Green
                backgroundColor[2] = 0.0f;  // Blue
            }
            else {
                // Flash white
                backgroundColor[0] = 1.0f;  // Red
                backgroundColor[1] = 1.0f;  // Green
                backgroundColor[2] = 1.0f;  // Blue
            }

            // Increment flash counter
            flashCounter++;
        }
        else {
            // Stop flashing after maxFlashes frames
            isFlashing = false;
            // Reset background to default color (for example, white)
            backgroundColor[0] = 1.0f;
            backgroundColor[1] = 1.0f;
            backgroundColor[2] = 1.0f;
        }
    }
}

void updateCloudPositions() {
    for (int i = 0; i < 3; i++) {
        // Move each cloud to the left based on its individual speed
        cloudOffsets[i] -= cloudSpeeds[i];

        // Reset the cloud position when it moves off the screen
        if (cloudOffsets[i] < -100.0f) {  // Assuming the cloud's width is 100
            cloudOffsets[i] = 1000.0f;  // Reset cloud to the right side of the screen
        }
    }
}
void updatePlayerPosition() {
    if (isJumping) {
        playerVelocityY += gravity;
        playerY += playerVelocityY;

        if (playerY <= 50.0f) {
            playerY = 50.0f;
            isJumping = false;
            playerVelocityY = 0.0f;
        }
    }

    if (isDucking) {
        playerY = 30.0f;
    }
    else if (!isJumping) {
        playerY = 50.0f;
    }
}
void updateGameObjects() {
    float currentGameSpeed = gameSpeed;  // Default game speed

    // Check if speed boost is active and modify speed accordingly
    if (activePowerUp == SPEED_BOOST && powerUpDuration > 0) {
        gameSpeed *= 1.0002f;  // Increase game speed by 50% during speed boost
    }
    if (powerUpDuration < 0) {
        gameSpeed = currentGameSpeed;  // Reset game speed to default
    }
    for (auto it = gameObjects.begin(); it != gameObjects.end();) {
        it->x -= gameSpeed;

        if (it->x < -50) {
            it = gameObjects.erase(it);
        }
        else {
            // Collision detection
            if (it->x < 110 && it->x > 90 && playerY < it->y + 20 && playerY + 30 > it->y) {
                if (it->type == 0 && activePowerUp != INVINCIBILITY) { // Obstacle collision
                    playerLives--;
                    playSound("sounds/collision.wav", false);

                    // Start the flashing effect
                    isFlashing = true;
                    flashCounter = 0;  // Reset the flash counter
                }
                else if (it->type == 1) { // Collectable
                    score += 10;
					playSound("sounds/collectable.wav", false);
                }
                else if (it->type == 2) { // Power-up
                    activePowerUp = static_cast<PowerUpType>(rand() % 2);
                    powerUpDuration = 5.0f; // 5 seconds duration
                    playSound("sounds/powerup.wav",false);
                    //playSound("sounds/background.wav", true);
                }
                it = gameObjects.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    // Spawn new objects
    if (rand() % 100 < 2) spawnGameObject(0); // Obstacle
    if (rand() % 100 < 1) spawnGameObject(1); // Collectable
    if (rand() % 800 < 1) spawnGameObject(2); // Power-up

    // Update power-up duration
    if (powerUpDuration > 0) {
        powerUpDuration -= 0.016f; // Assuming 60 FPS
        if (powerUpDuration <= 0) {
            activePowerUp = SPEED_BOOST; // Reset to default
        }
    }
}
void update(int value) {
    if (gameRunning) {
        gameTime -= 0.016f; // Assuming 60 FPS
        backgroundOffset -= scrollSpeed;
        updateCloudPositions();
        updatePlayerPosition();
        updateGameObjects();
        handleFlashing();
        gameSpeed += 0.003f; // Gradually increase game speed
        if (backgroundOffset <= -WINDOW_WIDTH) {
            backgroundOffset = 0.0f;
        }

        if (gameTime <= 0 || playerLives <= 0) {
            gameRunning = false;
        }

        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
    }
}

void spawnGameObject(int type) {
    GameObject obj;
    obj.x = WINDOW_WIDTH + 50;
    obj.y = 50 + rand() % 100;
    obj.active = true;
    obj.type = type;
    gameObjects.push_back(obj);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0f, 1.0f, 1.0f);
    glColor3f(backgroundColor[0], backgroundColor[1], backgroundColor[2]);
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0, 0, 0);
    glTexCoord2f(rep, 0.0f); glVertex3f(800, 0, 0);
    glTexCoord2f(rep, rep); glVertex3f(800, 600, 0);
    glTexCoord2f(0.0f, rep); glVertex3f(0, 600, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopMatrix();
    if (gameRunning) {
        drawBoundaries();
        drawPlayer();
        drawGameObjects();
        drawHealth();
        drawClouds();

        glColor3f(1.0f, 1.0f, 0.0f);
        drawText(WINDOW_WIDTH - 150, WINDOW_HEIGHT - 30, "Score: " + std::to_string(score));
        glColor3f(1.0f, 1.0f, 0.0f);
        drawText(WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT - 30, "Time: " + std::to_string((int)gameTime));

        if (powerUpDuration > 0) {
            std::string powerUpText = (activePowerUp == SPEED_BOOST) ? "Speed Boost" : "Invincibility";
            drawText(10, WINDOW_HEIGHT - 80, powerUpText + ": " + std::to_string((int)powerUpDuration));
        }
    }
    else {
        if (messageOpacity < 1.0f) {
            messageOpacity += 0.01f;  // Slowly increase opacity
        }

        // If the player has lost
        if (playerLives <= 0) {
            playSound("sounds/lose.wav", false);

            // Draw "Game Over" message with a dark red effect
            glColor4f(1.0f, 1.0f, 1.0f, messageOpacity);  // Dark red text with fading effect
            drawFancyText(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 80, "Game Over!", 2.0f);  // Larger font size

            // Draw the final score with dark teal color
            glColor4f(0.0f, 0.5f, 0.5f, messageOpacity);  // Dark teal fading text
            drawFancyText(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 40, "Final Score: " + std::to_string(score), 1.5f);

            // Additional descriptive message with dark blue-green color
            glColor4f(0.11f, 0.4f, 0.7f, messageOpacity);  // Blue-green color fading text
            drawFancyText(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 10, "Better luck next time!", 1.0f);
        }
        // If the player has won
        else {
            playSound("sounds/win.wav", false);

            // Draw "Good Job" message with a dark green glowing effect
            glColor4f(0.0f, 0.7f, 0.0f, messageOpacity);  // Dark green text with fading effect
            drawFancyText(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 80, "Good Job!", 2.0f);

            // Draw the final score with a medium teal color
            glColor4f(0.0f, 0.6f, 0.6f, messageOpacity);  // Medium teal fading text
            drawFancyText(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 40, "Final Score: " + std::to_string(score), 1.5f);

            // Additional descriptive message with a cool dark blue-green color
            glColor4f(0.0f, 0.5f, 0.8f, messageOpacity);  // Cool blue-green color fading text
            drawFancyText(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 10, "Congratulations, You Won!", 1.0f);
        }

    }

    glutSwapBuffers();
    //glfwPollEvents();
}
void initGame() {
    srand(static_cast<unsigned int>(time(0)));
    loadBMP(&backgroundTexture, "textures/background.bmp", false);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    loadBMP(&obstacleTexture, "textures/obs.bmp", false);
    glBindTexture(GL_TEXTURE_2D, obstacleTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    loadBMP(&playerTexture, "textures/player.bmp", false);
    glBindTexture(GL_TEXTURE_2D, playerTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    loadBMP(&cloudTexture, "textures/cloud.bmp", false);
    glBindTexture(GL_TEXTURE_2D, cloudTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gameObjects.clear();
    playerY = 50;
    score = 0;
    playerLives = 5;
    gameSpeed = 10.0f;
    gameTime = 10.0f;
    gameRunning = true;
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("2D Infinite Runner");

    glutDisplayFunc(render);
    glutKeyboardFunc(handleKeyPress);
    glutKeyboardUpFunc(handleKeyRelease);
    glutTimerFunc(16, update, 0);

    glMatrixMode(GL_PROJECTION);
    glEnable(GL_TEXTURE_2D);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
 

    initGame();
    playSound("sounds/background.wav", true);

    glutMainLoop();
    return 0;
}