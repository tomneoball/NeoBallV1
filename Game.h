#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <fstream>
#include "Structs.h"
#include "Paddle.h"
#include "Ball.h"
#include "Brick.h"

struct Particle { float x, y; float velX, velY; float life; Color color; };
struct PowerUp { SDL_FRect rect; PowerUpType type; bool active; };

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

    void loadSettings();
    void saveSettings();
    void changeResolution(int w, int h);
    void updateScaleFactor();

    void renderMenu();
    void renderSettings();
    void renderLevelComplete();
    void renderGameOver();

    bool drawButton(float x, float y, float w, float h, const char* text);
    void drawText(const char* text, float x, float y, float scale, Color c);
    void drawChar(char c, float x, float y, float scale, Color color);
    void drawNumber(int number, float x, float y, float scale);

    // 3D Helpers
    SDL_FPoint transform3D(float x, float y);
    SDL_FPoint transform3DWithHeight(float x, float y, float zHeight);

    // NEU: Spezialisierte Render-Funktionen für die Formen
    void renderBrickRect3D(SDL_Texture* tex, SDL_FRect rect, Color c);
    void renderBrickTriangle3D(SDL_Texture* tex, SDL_FRect rect, Color c);
    void renderBrickPenta3D(SDL_Texture* tex, SDL_FRect rect, Color c);

    void renderBillboard(SDL_Texture* tex, float x, float y, float w, float h, Color c);
    void renderArena3D();
    void drawQuad(SDL_FPoint p1, SDL_FPoint p2, SDL_FPoint p3, SDL_FPoint p4, Color c);

    void loadTextures();
    SDL_Texture* loadTexture(const char* file);

    void resetBall();
    void nextLevel();
    void loadLevel(int levelIndex);

    // createBrick angepasst für Shape
    void createBrick(float x, float y, int type, BrickShape shape);

    void spawnParticles(float x, float y, Color c);
    void trySpawnItem(float x, float y);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool isRunning = false;
    Uint64 lastTime = 0;

    int winWidth = 800;
    int winHeight = 600;
    float scaleFactor = 1.0f;
    float arenaTopBoundary = 0.0f;

    SDL_Texture* texBg = nullptr;
    SDL_Texture* texPaddle = nullptr;
    SDL_Texture* texBall = nullptr;
    SDL_Texture* texBrickWood = nullptr;
    SDL_Texture* texBrickStone = nullptr;
    SDL_Texture* texBrickGold = nullptr;
    SDL_Texture* texBrickGreen = nullptr;
    SDL_Texture* texItemPower = nullptr;
    SDL_Texture* texItemPoint = nullptr;
    SDL_Texture* texWhitePixel = nullptr;

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
    float mouseX = 0;
    float mouseY = 0;
    bool mousePressed = false;
};
