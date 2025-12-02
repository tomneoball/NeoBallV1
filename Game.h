#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <fstream>
#include "Structs.h"
#include "Paddle.h"
#include "Ball.h"
#include "Brick.h"

struct Particle {
    float x, y;
    float velX, velY;
    float life;
    Color color;
};

struct PowerUp {
    SDL_FRect rect;
    PowerUpType type;
    bool active;
};

class Game {
public:
    Game();
    ~Game();

    bool init(const char* title);
    void run();

private:
    void processEvents();
    void update(float deltaTime);
    void render();

    // Management
    void loadSettings();
    void saveSettings();
    void changeResolution(int w, int h);

    // Highscore System
    void loadHighscore();
    void saveHighscore();

    // Rendering States
    void renderMenu();
    void renderSettings();
    void renderLevelComplete();
    void renderShop();
    void renderGameOver();

    // UI Helpers
    bool drawButton(float x, float y, float w, float h, const char* text);
    void drawText(const char* text, float x, float y, float scale, Color c);
    void drawChar(char c, float x, float y, float scale, Color color);
    void drawNumber(int number, float x, float y, float scale);

    // Resources
    void loadTextures();
    SDL_Texture* loadTexture(const char* file);

    // Gameplay
    void resetBall();
    void nextLevel();
    void loadLevel(int levelIndex);
    void createBrick(float x, float y, int type);
    void spawnParticles(float x, float y, Color c);
    void trySpawnItem(float x, float y);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool isRunning = false;
    Uint64 lastTime = 0;

    int winWidth = 800;
    int winHeight = 600;

    float scaleFactor = 1.0f;

    SDL_Texture* texBg = nullptr;
    SDL_Texture* texPaddle = nullptr;
    SDL_Texture* texBall = nullptr;
    SDL_Texture* texPilot = nullptr;
    SDL_Texture* texBrickWood = nullptr;
    SDL_Texture* texBrickStone = nullptr;
    SDL_Texture* texBrickGold = nullptr;
    SDL_Texture* texBrickGreen = nullptr;
    SDL_Texture* texItemPower = nullptr;
    SDL_Texture* texItemPoint = nullptr;

    Paddle* paddle = nullptr;

    std::vector<Ball> balls;
    std::vector<Brick> bricks;
    std::vector<Particle> particles;
    std::vector<PowerUp> powerups;
    std::vector<PointItem> pointItems;

    float shakeTime = 0.0f;
    int lives;
    bool ballStuckToPaddle;
    GameState gameState;

    int score;
    int highScore;
    int currentLevelIndex;

    // NEU: Währung für den Shop
    int collectedCoins = 0;

    // Upgrade Flags
    bool upgradeWidePaddle = false;
    bool upgradeFireball = false;

    float mouseX = 0;
    float mouseY = 0;
    bool mousePressed = false;
};