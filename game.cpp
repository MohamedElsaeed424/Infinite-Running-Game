#include <ctime>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include "TextureBuilder.h"
#include <glut.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;


float playerY = 50.0f;
float playerVelocityY = 0.0f;
float gravity = -0.5f;
bool isJumping = false;
bool isDucking = false;
const float jumpStrength = 12.0f;


int playerLives = 5;
int score = 0;
float gameTime = 60.0f;
float gameSpeed = 2.0f;
bool gameRunning = true;

GLuint backgroundTexture;
GLuint obstacleTexture;
float backgroundOffset = 0.0f;
const float scrollSpeed = 2.0f;
int rep = 1;


struct GameObject {
    float x, y;
    bool active;
    int type; // 0: obstacle, 1: collectable, 2: power-up
};

std::vector<GameObject> gameObjects;


enum PowerUpType { SPEED_BOOST, INVINCIBILITY };
PowerUpType activePowerUp = SPEED_BOOST;
float powerUpDuration = 0.0f;


void drawText(float x, float y, std::string text);
void spawnGameObject(int type);
void updateGameObjects();
void drawGameObjects();

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

void drawPlayer() {
    glPushMatrix();
    glTranslatef(100, playerY, 0);

    // Body (Quad)
    glColor3f(0.0f, 0.8f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-10, -10);
    glVertex2f(10, -10);
    glVertex2f(10, 20);
    glVertex2f(-10, 20);
    glEnd();

    // Head (Circle)
    glColor3f(0.8f, 0.6f, 0.4f);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < 360; i++) {
        float theta = i * 3.14159f / 180.0f;
        glVertex2f(5 * cos(theta), 25 + 5 * sin(theta));
    }
    glEnd();

    // Eyes (Points)
    glColor3f(0.0f, 0.0f, 0.0f);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    glVertex2f(-3, 26);
    glVertex2f(3, 26);
    glEnd();

    // Mouth (Line)
    glBegin(GL_LINES);
    glVertex2f(-3, 23);
    glVertex2f(3, 23);
    glEnd();

    glPopMatrix();
}

void drawBoundaries() {
    // Upper boundary (gradient effect)
    glBegin(GL_QUADS);
    for (int i = 0; i < WINDOW_WIDTH; i += 50) {
        // Calculate gradient colors based on position
        float t = (float)i / WINDOW_WIDTH;
        float red = 0.8f * (1 - t) + 0.6f * t;    // Interpolating between light and dark gray for red
        float green = 0.8f * (1 - t) + 0.4f * t;  // Interpolating between light and darker green
        float blue = 0.8f * (1 - t) + 0.4f * t;   // Interpolating between light and darker blue

        glColor3f(red, green, blue);  // Set color at the top-left
        glVertex2f(i, WINDOW_HEIGHT - 20 + 5 * sin(i * 0.05f));  // Top-left
        glVertex2f(i + 50, WINDOW_HEIGHT - 20 + 5 * sin((i + 50) * 0.05f));  // Top-right
        glColor3f(0.6f, 0.6f, 0.6f);  // Bottom part has a constant color
        glVertex2f(i + 50, WINDOW_HEIGHT - 30);  // Bottom-right
        glVertex2f(i, WINDOW_HEIGHT - 30);  // Bottom-left
    }
    glEnd();

    // Lower boundary (gradient effect)
    glBegin(GL_QUADS);
    for (int i = 0; i < WINDOW_WIDTH; i += 50) {
        // Calculate gradient colors for the lower boundary
        float t = (float)i / WINDOW_WIDTH;
        float red = 0.6f * (1 - t) + 0.4f * t;    // Interpolating between dark and lighter gray
        float green = 0.6f * (1 - t) + 0.3f * t;  // Interpolating between dark and lighter green
        float blue = 0.6f * (1 - t) + 0.3f * t;   // Interpolating between dark and lighter blue

        glColor3f(red, green, blue);  // Set color at the top-left
        glVertex2f(i, 20 + 5 * sin(i * 0.05f + 3.14159f));  // Top-left
        glVertex2f(i + 50, 20 + 5 * sin((i + 50) * 0.05f + 3.14159f));  // Top-right
        glColor3f(0.4f, 0.4f, 0.4f);  // Bottom part has a constant color
        glVertex2f(i + 50, 30);  // Bottom-right
        glVertex2f(i, 30);  // Bottom-left
    }
    glEnd();

    // Additional decorative elements (triangles)
    glColor3f(0.7f, 0.7f, 0.7f);  // Gray color for triangles
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < WINDOW_WIDTH; i += 100) {
        glVertex2f(i, 0);  // Bottom-left
        glVertex2f(i + 50, 0);  // Bottom-right
        glVertex2f(i + 25, 10);  // Top-middle
    }
    glEnd();
}



void updateGameObjects() {
    float currentGameSpeed = gameSpeed;  // Default game speed

    // Check if speed boost is active and modify speed accordingly
    if (activePowerUp == SPEED_BOOST && powerUpDuration > 0) {
        gameSpeed *= 1.0002f;  // Increase game speed by 50% during speed boost
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(WINDOW_WIDTH / 2 - 70, WINDOW_HEIGHT / 2 - 30, "current game speed = " + std::to_string(currentGameSpeed));
    }
    if (powerUpDuration < 0) {
        gameSpeed = currentGameSpeed;  // Reset game speed to default
        glColor3f(1.0f, 0.0f, 1.0f);
        drawText(WINDOW_WIDTH / 2 - 70, WINDOW_HEIGHT / 2 - 30, "current game speed = " + std::to_string(currentGameSpeed));
    }
    for (auto it = gameObjects.begin(); it != gameObjects.end();) {
        it->x -= gameSpeed;

        if (it->x < -50) {
            it = gameObjects.erase(it);
        }
        else {
            // Collision detection
            if (it->x < 110 && it->x > 90 && playerY < it->y + 20 && playerY + 30 > it->y) {
                if (it->type == 0 && activePowerUp != INVINCIBILITY) { // Obstacle
                    playerLives--;
                }
                else if (it->type == 1) { // Collectable
                    score += 10;
                }
                else if (it->type == 2) { // Power-up
                    activePowerUp = static_cast<PowerUpType>(rand() % 2);
                    powerUpDuration = 5.0f; // 5 seconds duration
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
    if (rand() % 50 < 1) spawnGameObject(2); // Power-up

    // Update power-up duration
    if (powerUpDuration > 0) {
        powerUpDuration -= 0.016f; // Assuming 60 FPS
        if (powerUpDuration <= 0) {
            activePowerUp = SPEED_BOOST; // Reset to default
        }
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
            glColor3f(1.0f, 1.0f, 0.0f);
            glBegin(GL_TRIANGLE_FAN);
            for (int i = 0; i < 360; i += 36) {
                float theta = i * 3.14159f / 180.0f;
                glVertex2f(10 * cos(theta), 10 * sin(theta));
            }
            glEnd();
        }
        else if (obj.type == 2) { // Power-up

            if (activePowerUp == SPEED_BOOST) {
                //glColor3f(1.0f, 1.0f, 1.0f);
                //drawText(WINDOW_WIDTH / 2 - 70, WINDOW_HEIGHT / 2 - 30, "SPEED_BOOST Now" );
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
            else if (activePowerUp == INVINCIBILITY) {  // Second Power-up: Hexagon with diagonal lines
                // Set color for the text to bright yellow
                //glColor3f(1.0f, 1.0f, 0.0f);  // Yellow color
                //drawText(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 30, "INVINCIBILITY NOW");  // Center the text better

                // Now change the hexagon and line colors

                // Set a color for the hexagon (gradient-like effect by changing vertex colors)
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

                // Set color for the diagonal lines (contrasting color, white in this case)
                glColor3f(1.0f, 1.0f, 1.0f);  // White color

                glBegin(GL_LINES);
                // Line 1
                glVertex2f(0, -10);
                glVertex2f(0, 10);
                // Line 2
                glVertex2f(-8.66f, -5);  // -8.66 = 10 * cos(60 degrees)
                glVertex2f(8.66f, 5);
                // Line 3
                glVertex2f(-8.66f, 5);
                glVertex2f(8.66f, -5);
                glEnd();

            }
        }

        glPopMatrix();
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

void handleKeyPress(unsigned char key, int x, int y) {
    if (key == ' ' && !isJumping) {
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

void update(int value) {
    if (gameRunning) {
        gameTime -= 0.016f; // Assuming 60 FPS
        backgroundOffset -= scrollSpeed;
        updatePlayerPosition();
        updateGameObjects();
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
// to draw the game objects and player
void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
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
        if (playerLives <= 0) {
            glColor3f(1.0f, 0.0f, 0.0f);
            drawText(WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT / 2, "Game Over");
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText(WINDOW_WIDTH / 2 - 70, WINDOW_HEIGHT / 2 - 30, "Final Score: " + std::to_string(score));
        }
        else {
            glColor3f(0.0f, 1.0f, 0.0f);
            drawText(WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT / 2, "Good job !"); // if the player wins good j
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText(WINDOW_WIDTH / 2 - 70, WINDOW_HEIGHT / 2 - 30, "Final Score: " + std::to_string(score));
        }
    }

    glutSwapBuffers();
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
    gameObjects.clear();
    playerY = 50;
    score = 0;
    playerLives = 5;
    gameSpeed = 10.0f;
    gameTime = 50.0f;
    gameRunning = true;
}

void drawText(float x, float y, std::string text) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
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

    glutMainLoop();
    return 0;
}