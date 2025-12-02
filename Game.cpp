#include "Game.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>

// Hilfsfunktion für Kreise (Filled + Outline)
void DrawCircle(SDL_Renderer* renderer, float cx, float cy, float radius, Color fill, Color outline) {
    // 1. Füllung
    SDL_SetRenderDrawColorFloat(renderer, fill.r, fill.g, fill.b, fill.a);
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            float dx = radius - w;
            float dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderPoint(renderer, cx + dx - radius, cy + dy - radius);
            }
        }
    }
    // 2. Umriss
    SDL_SetRenderDrawColorFloat(renderer, outline.r, outline.g, outline.b, outline.a);
    float r = radius;
    for (float angle = 0; angle < 360; angle += 2.0f) {
        float rad = angle * 3.14159f / 180.0f;
        SDL_RenderPoint(renderer, cx + cos(rad) * r, cy + sin(rad) * r);
    }
}

// Konstruktor
Game::Game() : lives(3), ballStuckToPaddle(true), gameState(STATE_MENU), score(0), highScore(0), currentLevelIndex(1) {
    upgradeWidePaddle = false;
    upgradeFireball = false;
    collectedCoins = 0; // Reset Coins

    loadSettings();
    loadHighscore();
}

Game::~Game() {
    if (texBg) SDL_DestroyTexture(texBg);
    if (texPaddle) SDL_DestroyTexture(texPaddle);
    if (texBall) SDL_DestroyTexture(texBall);
    if (texPilot) SDL_DestroyTexture(texPilot);
    if (texBrickWood) SDL_DestroyTexture(texBrickWood);
    if (texBrickStone) SDL_DestroyTexture(texBrickStone);
    if (texBrickGold) SDL_DestroyTexture(texBrickGold);
    if (texBrickGreen) SDL_DestroyTexture(texBrickGreen);
    if (texItemPower) SDL_DestroyTexture(texItemPower);
    if (texItemPoint) SDL_DestroyTexture(texItemPoint);

    delete paddle;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::loadSettings() {
    std::ifstream file("settings.cfg");
    if (file.is_open()) {
        file >> winWidth >> winHeight;
        file.close();
    }
    else {
        winWidth = 800; winHeight = 600;
    }
}

void Game::saveSettings() {
    std::ofstream file("settings.cfg");
    if (file.is_open()) {
        file << winWidth << " " << winHeight;
        file.close();
    }
}

void Game::loadHighscore() {
    std::ifstream file("highscore.dat");
    if (file.is_open()) { file >> highScore; file.close(); }
    else { highScore = 0; }
}

void Game::saveHighscore() {
    std::ofstream file("highscore.dat");
    if (file.is_open()) { file << highScore; file.close(); }
}

void Game::changeResolution(int w, int h) {
    winWidth = w; winHeight = h;
    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    float scaleFactor = (float)winWidth / 800.0f;
    delete paddle;
    paddle = new Paddle((float)winWidth / 2.0f - (50.0f * scaleFactor), (float)winHeight - (50.0f * scaleFactor), scaleFactor);
    loadLevel(currentLevelIndex);
    resetBall();
    saveSettings();
}

bool Game::init(const char* title) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;
    window = SDL_CreateWindow(title, winWidth, winHeight, 0);
    renderer = SDL_CreateRenderer(window, NULL);
    srand((unsigned int)time(0));
    loadTextures();

    float scaleFactor = (float)winWidth / 800.0f;
    paddle = new Paddle((float)winWidth / 2.0f - (50.0f * scaleFactor), (float)winHeight - (50.0f * scaleFactor), scaleFactor);

    lives = 3; score = 0; collectedCoins = 0; currentLevelIndex = 1;
    gameState = STATE_MENU;
    upgradeWidePaddle = false; upgradeFireball = false;

    loadLevel(currentLevelIndex);
    resetBall();
    isRunning = true;
    return true;
}

SDL_Texture* Game::loadTexture(const char* file) {
    char path[256];
    snprintf(path, sizeof(path), "graphics/%s", file);
    SDL_Surface* surf = SDL_LoadBMP(path);
    if (!surf) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    return tex;
}

void Game::loadTextures() {
    texBg = loadTexture("bg.bmp");
    texPaddle = loadTexture("paddle.bmp");
    texBall = loadTexture("ball.bmp");
    texPilot = loadTexture("pilot.bmp");
    texBrickWood = loadTexture("brick_wood.bmp");
    texBrickStone = loadTexture("brick_stone.bmp");
    texBrickGold = loadTexture("brick_gold.bmp");
    texBrickGreen = loadTexture("brick_green.bmp");
    texItemPower = loadTexture("item_power.bmp");
    texItemPoint = loadTexture("item_point.bmp");
}

void Game::resetBall() {
    float scaleFactor = (float)winWidth / 800.0f;
    balls.clear();
    float startWidth = upgradeWidePaddle ? 150.0f : 100.0f;
    paddle->setWidth(startWidth);

    SDL_FRect pRect = paddle->getRect();
    float ballSize = 16.0f * scaleFactor;
    Ball b(pRect.x + pRect.w / 2.0f - ballSize / 2.0f, pRect.y - ballSize - 2.0f, 0.0f, 0.0f, scaleFactor);
    if (upgradeFireball) b.setFireball(true);
    balls.push_back(b);
    ballStuckToPaddle = true;
}

void Game::createBrick(float x, float y, int type) {
    float scaleFactor = (float)winWidth / 800.0f;
    int health = (type == 1) ? 1 : (type == 2 ? 2 : 3);
    float r, g, b;
    if (type == 1) { r = 0.8f; g = 0.5f; b = 0.2f; }
    else if (type == 2) { r = 0.5f; g = 0.5f; b = 0.6f; }
    else if (type == 3) { r = 0.9f; g = 0.8f; b = 0.1f; }
    else { r = 0.2f; g = 0.8f; b = 0.2f; }
    bricks.push_back(Brick(x, y, health, type, { r, g, b, 1.0f }, scaleFactor));
}

void Game::loadLevel(int levelIndex) {
    float scaleFactor = (float)winWidth / 800.0f;
    bricks.clear(); powerups.clear(); pointItems.clear(); particles.clear();

    float levelDesignWidth = 800.0f;
    float offsetX = (float)(winWidth - (levelDesignWidth * scaleFactor)) / 2.0f;
    float bW = 60.0f * scaleFactor;
    float spacingX = 70.0f * scaleFactor;
    float spacingY = 40.0f * scaleFactor;
    float startY = 50.0f * scaleFactor;

    if (levelIndex == 1) {
        for (int row = 0; row < 8; row++) {
            float rowOffset = (float)(8 - row) * (bW / 2.0f);
            for (int col = 0; col <= row; col++)
                createBrick(offsetX + (150.0f * scaleFactor) + rowOffset + (float)col * spacingX, startY + (float)row * spacingY, (row % 2) + 1);
        }
    }
    else if (levelIndex == 2) {
        float bX = offsetX + 200.0f * scaleFactor;
        for (int x = 0; x < 5; x++) for (int y = 2; y < 6; y++)
            if (x == 0 || x == 4 || y == 2 || y == 5) createBrick(bX + (float)x * spacingX, startY + (float)y * spacingY, 2);
        createBrick(offsetX + 340.0f * scaleFactor, startY, 3);
        createBrick(offsetX + 270.0f * scaleFactor, startY + 40.0f * scaleFactor, 2); createBrick(offsetX + 410.0f * scaleFactor, startY + 40.0f * scaleFactor, 2);
        createBrick(offsetX + 270.0f * scaleFactor, startY + 120.0f * scaleFactor, 1); createBrick(offsetX + 410.0f * scaleFactor, startY + 120.0f * scaleFactor, 1);
        createBrick(offsetX + 340.0f * scaleFactor, startY + 160.0f * scaleFactor, 1);
        createBrick(offsetX + 270.0f * scaleFactor, startY + 200.0f * scaleFactor, 1); createBrick(offsetX + 410.0f * scaleFactor, startY + 200.0f * scaleFactor, 1);
    }
    else if (levelIndex == 3) {
        for (int y = 0; y < 8; y++) createBrick(offsetX + 350.0f * scaleFactor, startY + (float)y * spacingY, 2);
        for (int x = 0; x < 7; x++) createBrick(offsetX + 140.0f * scaleFactor + (float)x * spacingX, startY + 120.0f * scaleFactor, 3);
        createBrick(offsetX + 280.0f * scaleFactor, startY + 280.0f * scaleFactor, 1); createBrick(offsetX + 420.0f * scaleFactor, startY + 280.0f * scaleFactor, 1);
        createBrick(offsetX + 350.0f * scaleFactor, startY - 30.0f * scaleFactor, 3);
    }
    else if (levelIndex == 4) {
        float startYTrain = 360.0f * scaleFactor;
        for (int i = 0; i < 4; i++) createBrick(offsetX + 100.0f * scaleFactor + (float)i * 100.0f * scaleFactor, startYTrain + 40.0f * scaleFactor, 2);
        for (int x = 0; x < 7; x++) createBrick(offsetX + 80.0f * scaleFactor + (float)x * spacingX, startYTrain, 2);
        for (int x = 4; x < 7; x++) for (int y = 0; y < 3; y++) createBrick(offsetX + 80.0f * scaleFactor + (float)x * spacingX, startYTrain - 120.0f * scaleFactor + (float)y * spacingY, 1);
        for (int x = 0; x < 4; x++) for (int y = 1; y < 3; y++) createBrick(offsetX + 80.0f * scaleFactor + (float)x * spacingX, startYTrain - 120.0f * scaleFactor + (float)y * spacingY, 3);
        createBrick(offsetX + 150.0f * scaleFactor, startYTrain - 160.0f * scaleFactor, 2);
    }
}

void Game::nextLevel() {
    currentLevelIndex++;
    if (currentLevelIndex > 4) currentLevelIndex = 1;
    loadLevel(currentLevelIndex);
    resetBall();
    gameState = STATE_LEVEL_COMPLETE;
}

void Game::trySpawnItem(float x, float y) {
    float scaleFactor = (float)winWidth / 800.0f;
    if ((powerups.size() + pointItems.size()) >= 4) return;
    int roll = rand() % 100;
    float size = 20.0f * scaleFactor;

    if (roll < 20) {
        PowerUp pu; pu.rect = { x, y, size, size }; pu.active = true;
        pu.type = (PowerUpType)(rand() % 3);
        powerups.push_back(pu);
    }
    else if (roll < 60) {
        PointItem pi; pi.x = x; pi.y = y; pi.active = true;
        int pRoll = rand() % 100;
        // Punkte Items sind jetzt auch Währung
        if (pRoll < 10) { pi.value = 50; pi.radius = 6.0f * scaleFactor; pi.velY = 250.0f * scaleFactor; pi.color = { 1,0,0,1 }; }
        else if (pRoll < 35) { pi.value = 25; pi.radius = 9.0f * scaleFactor; pi.velY = 180.0f * scaleFactor; pi.color = { 1,0.5f,0,1 }; }
        else if (pRoll < 65) { pi.value = 20; pi.radius = 10.0f * scaleFactor; pi.velY = 150.0f * scaleFactor; pi.color = { 1,1,0,1 }; }
        else { pi.value = 10; pi.radius = 14.0f * scaleFactor; pi.velY = 100.0f * scaleFactor; pi.color = { 0.2f,1,0.2f,1 }; }
        pointItems.push_back(pi);
    }
}

void Game::spawnParticles(float x, float y, Color c) {
    float scaleFactor = (float)winWidth / 800.0f;
    for (int i = 0; i < 6; i++) {
        Particle p; p.x = x; p.y = y;
        p.velX = (float)(rand() % 200 - 100) * scaleFactor;
        p.velY = (float)(rand() % 200 - 100) * scaleFactor;
        p.life = 1.0f; p.color = c;
        particles.push_back(p);
    }
}

void Game::processEvents() {
    float scaleFactor = (float)winWidth / 800.0f;
    SDL_Event event;
    mousePressed = false;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) isRunning = false;
        if (event.type == SDL_EVENT_MOUSE_MOTION) { mouseX = event.motion.x; mouseY = event.motion.y; }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) { if (event.button.button == SDL_BUTTON_LEFT) mousePressed = true; }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) { if (gameState != STATE_MENU) gameState = STATE_MENU; }
            if (gameState == STATE_PLAYING) {
                if (ballStuckToPaddle && event.key.key == SDLK_SPACE) {
                    ballStuckToPaddle = false;
                    if (!balls.empty()) balls[0].setVelocity(0.0f, -360.0f * scaleFactor);
                }
            }
            else if (gameState == STATE_GAME_OVER) {
                if (event.key.key == SDLK_SPACE) {
                    lives = 3; score = 0; collectedCoins = 0; currentLevelIndex = 1;
                    upgradeWidePaddle = false; upgradeFireball = false;
                    loadLevel(1); resetBall();
                    gameState = STATE_PLAYING;
                }
            }
        }
    }
}

void Game::drawChar(char c, float x, float y, float s, Color color) {
    static const int fontMap[][15] = {
        {0,1,0,1,0,1,1,1,1,1,0,1,1,0,1}, {1,1,0,1,0,1,1,1,0,1,0,1,1,1,0}, {0,1,1,1,0,0,1,0,0,1,0,0,0,1,1},
        {1,1,0,1,0,1,1,0,1,1,0,1,1,1,0}, {1,1,1,1,0,0,1,1,0,1,0,0,1,1,1}, {1,1,1,1,0,0,1,1,0,1,0,0,1,0,0},
        {0,1,1,1,0,0,1,0,1,1,0,1,0,1,1}, {1,0,1,1,0,1,1,1,1,1,0,1,1,0,1}, {1,1,1,0,1,0,0,1,0,0,1,0,1,1,1},
        {0,0,1,0,0,1,0,0,1,1,0,1,0,1,0}, {1,0,1,1,0,1,1,1,0,1,0,1,1,0,1}, {1,0,0,1,0,0,1,0,0,1,0,0,1,1,1},
        {1,0,1,1,1,1,1,0,1,1,0,1,1,0,1}, {1,0,1,1,1,1,1,1,1,1,0,1,1,0,1}, {0,1,0,1,0,1,1,0,1,1,0,1,0,1,0},
        {1,1,0,1,0,1,1,1,0,1,0,0,1,0,0}, {0,1,0,1,0,1,1,0,1,0,1,0,0,0,1}, {1,1,0,1,0,1,1,1,0,1,0,1,1,0,1},
        {0,1,1,1,0,0,0,1,0,0,0,1,1,1,0}, {1,1,1,0,1,0,0,1,0,0,1,0,0,1,0}, {1,0,1,1,0,1,1,0,1,1,0,1,0,1,1},
        {1,0,1,1,0,1,1,0,1,0,1,0,0,1,0}, {1,0,1,1,0,1,1,0,1,1,1,1,1,0,1}, {1,0,1,0,1,0,0,1,0,0,1,0,1,0,1},
        {1,0,1,1,0,1,0,1,0,0,1,0,0,1,0}, {1,1,1,0,0,1,0,1,0,1,0,0,1,1,1},
    };
    int index = -1;
    if (c >= 'A' && c <= 'Z') index = c - 'A';

    if (index >= 0) {
        SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
        for (int i = 0; i < 15; i++) {
            if (fontMap[index][i]) {
                int col = i % 3; int row = i / 3;
                SDL_FRect r = { x + (float)col * s, y + (float)row * s, s, s };
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
}

void Game::drawText(const char* text, float x, float y, float scale, Color c) {
    float cursorX = x;
    while (*text) {
        char ch = *text;
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        if (ch >= 'A' && ch <= 'Z') {
            drawChar(ch, cursorX, y, scale, c);
        }
        else if (ch >= '0' && ch <= '9') {
            drawNumber(ch - '0', cursorX, y, scale / 10.0f * 1.5f);
        }
        cursorX += (3.0f * scale) + (1.0f * scale);
        text++;
    }
}

bool Game::drawButton(float x, float y, float w, float h, const char* text) {
    float scaleFactor = (float)winWidth / 800.0f;
    bool hovered = (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);
    Color bg = hovered ? Color{ 0.3f, 0.7f, 0.3f, 1 } : Color{ 0.2f, 0.2f, 0.2f, 1 };
    SDL_SetRenderDrawColorFloat(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_FRect rect = { x, y, w, h };
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColorFloat(renderer, 1, 1, 1, 1);
    SDL_RenderRect(renderer, &rect);

    float textLen = 0; const char* t = text; while (*t++) textLen++;
    float tScale = 4.0f * scaleFactor;
    float tWidth = textLen * (4.0f * tScale);
    drawText(text, x + (w - tWidth) / 2.0f, y + h / 2.0f - (2.5f * tScale), tScale, { 1,1,1,1 });
    return (hovered && mousePressed);
}

void Game::update(float dt) {
    float scaleFactor = (float)winWidth / 800.0f;

    if (score > highScore) { highScore = score; saveHighscore(); }

    if (gameState == STATE_PLAYING) {
        paddle->update(dt, winWidth);

        if (ballStuckToPaddle && !balls.empty()) {
            SDL_FRect pRect = paddle->getRect();
            SDL_FRect bRect = balls[0].getRect();
            balls[0].setPosition(pRect.x + (pRect.w / 2.0f) - (bRect.w / 2.0f), pRect.y - bRect.h);
        }

        for (auto& pi : pointItems) {
            if (!pi.active) continue;
            pi.y += pi.velY * dt;
            SDL_FRect pRect = paddle->getRect();
            float cx = std::max(pRect.x, std::min(pi.x, pRect.x + pRect.w));
            float cy = std::max(pRect.y, std::min(pi.y, pRect.y + pRect.h));
            if (((pi.x - cx) * (pi.x - cx) + (pi.y - cy) * (pi.y - cy)) < (pi.radius * pi.radius)) {
                pi.active = false;
                score += pi.value; // Score erhöhen
                collectedCoins++;  // NEU: Coin für Shop sammeln
            }
            if (pi.y > (float)winHeight) pi.active = false;
        }

        for (auto& p : powerups) {
            if (!p.active) continue;
            p.rect.y += 150.0f * scaleFactor * dt;
            SDL_FRect pRect = paddle->getRect();
            if (SDL_HasRectIntersectionFloat(&p.rect, &pRect)) {
                p.active = false;
                if (p.type == PU_WIDE) { paddle->setWidth(150.0f); upgradeWidePaddle = true; }
                if (p.type == PU_FIRE) { for (auto& b : balls) b.setFireball(true); upgradeFireball = true; }
                if (p.type == PU_MULTI) {
                    ballStuckToPaddle = false;
                    float baseSpeed = 360.0f * scaleFactor;
                    float spawnOffset = 30.0f * scaleFactor;
                    if (!balls.empty()) balls[0].setVelocity(balls[0].getRect().x > (float)winWidth / 2.0f ? -baseSpeed / 2.0f : baseSpeed / 2.0f, -baseSpeed);
                    balls.push_back(Ball(pRect.x, pRect.y - spawnOffset, -baseSpeed / 2.0f, -baseSpeed, scaleFactor));
                    balls.push_back(Ball(pRect.x, pRect.y - spawnOffset, baseSpeed / 2.0f, -baseSpeed, scaleFactor));
                }
            }
            if (p.rect.y > (float)winHeight) p.active = false;
        }

        bool anyBallActive = false;
        int activeBricksCount = 0;
        for (const auto& br : bricks) if (br.isActive()) activeBricksCount++;

        for (auto& ball : balls) {
            if (!ball.isActive()) continue;
            anyBallActive = true;
            if (!ballStuckToPaddle) ball.update(dt, winWidth, winHeight);

            SDL_FRect bRect = ball.getRect();
            SDL_FRect pRect = paddle->getRect();

            if (!ballStuckToPaddle && SDL_HasRectIntersectionFloat(&bRect, &pRect)) {
                if (ball.getVelY() > 0) {
                    ball.setPosition(bRect.x, pRect.y - bRect.h - 0.1f);
                    ball.invertY();
                    float hitPos = (bRect.x + bRect.w / 2.0f) - (pRect.x + pRect.w / 2.0f);
                    ball.setVelX(hitPos * 4.5f);
                }
            }

            for (auto& brick : bricks) {
                if (!brick.isActive()) continue;
                brick.updateAnimation(dt);
                SDL_FRect brRect = brick.getRect();
                if (SDL_HasRectIntersectionFloat(&bRect, &brRect)) {
                    if (!ball.isFire()) ball.invertY();
                    brick.hit();
                    if (!brick.isActive()) {
                        shakeTime = 0.1f;
                        score += 10;
                        spawnParticles(brRect.x + brRect.w / 2.0f, brRect.y + brRect.h / 2.0f, brick.getColor());
                        trySpawnItem(brRect.x + brRect.w / 2.0f, brRect.y);
                        activeBricksCount--;
                    }
                    break;
                }
            }
        }

        if (activeBricksCount == 0 && !bricks.empty()) { nextLevel(); return; }

        if (!anyBallActive && !ballStuckToPaddle) {
            lives--;
            upgradeFireball = false;
            if (lives > 0) resetBall(); else gameState = STATE_GAME_OVER;
        }

        for (auto& p : particles) {
            p.x += p.velX * dt; p.y += p.velY * dt;
            p.velY += 300.0f * scaleFactor * dt;
            p.life -= 2.0f * dt;
        }
        if (shakeTime > 0) shakeTime -= dt;
    }
}

void Game::renderMenu() {
    float scaleFactor = (float)winWidth / 800.0f;
    float cx = (float)winWidth / 2.0f;
    float cy = (float)winHeight / 2.0f;
    float btnW = 200.0f * scaleFactor;
    float btnH = 50.0f * scaleFactor;

    drawText("NEOBALL", cx - 120.0f * scaleFactor, 100.0f * scaleFactor, 8.0f * scaleFactor, { 0,1,1,1 });
    std::string hsText = "BEST: " + std::to_string(highScore);
    drawText(hsText.c_str(), cx - 80.0f * scaleFactor, 180.0f * scaleFactor, 3.0f * scaleFactor, { 1, 0.8f, 0, 1 });
    const char* playText = (score > 0 || currentLevelIndex > 1) ? "RESUME" : "PLAY";
    if (drawButton(cx - btnW / 2.0f, cy - 60.0f * scaleFactor, btnW, btnH, playText)) gameState = STATE_PLAYING;
    if (drawButton(cx - btnW / 2.0f, cy + 10.0f * scaleFactor, btnW, btnH, "SETTINGS")) gameState = STATE_SETTINGS;
    if (drawButton(cx - btnW / 2.0f, cy + 80.0f * scaleFactor, btnW, btnH, "EXIT")) isRunning = false;
}

void Game::renderSettings() {
    float scaleFactor = (float)winWidth / 800.0f;
    float cx = (float)winWidth / 2.0f;
    float btnW = 300.0f * scaleFactor;
    float btnH = 40.0f * scaleFactor;
    float startY = 120.0f * scaleFactor;
    float gap = 55.0f * scaleFactor;

    drawText("RESOLUTION", cx - 120.0f * scaleFactor, 50.0f * scaleFactor, 6.0f * scaleFactor, { 1,1,1,1 });
    if (drawButton(cx - btnW / 2.0f, startY, btnW, btnH, "800 X 600")) changeResolution(800, 600);
    if (drawButton(cx - btnW / 2.0f, startY + gap, btnW, btnH, "1280 X 720")) changeResolution(1280, 720);
    if (drawButton(cx - btnW / 2.0f, startY + gap * 2, btnW, btnH, "1680 X 1050")) changeResolution(1680, 1050);
    if (drawButton(cx - btnW / 2.0f, startY + gap * 3, btnW, btnH, "1920 X 1080")) changeResolution(1920, 1080);
    if (drawButton(cx - btnW / 2.0f, startY + gap * 4, btnW, btnH, "2560 X 1440")) changeResolution(2560, 1440);
    float listEnd = startY + (gap * 5);
    if (drawButton(cx - (200.0f * scaleFactor) / 2.0f, listEnd + 20.0f * scaleFactor, 200.0f * scaleFactor, 50.0f * scaleFactor, "BACK")) gameState = STATE_MENU;
}

// NEUER SHOP: Mit Items Leben kaufen
void Game::renderShop() {
    float scaleFactor = (float)winWidth / 800.0f;
    float cx = (float)winWidth / 2.0f;
    float startY = 120.0f * scaleFactor;
    float btnW = 350.0f * scaleFactor;
    float btnH = 50.0f * scaleFactor;

    drawText("ITEM SHOP", cx - 100.0f * scaleFactor, 40.0f * scaleFactor, 5.0f * scaleFactor, { 1, 0.8f, 0, 1 });

    // Zeige gesammelte Coins (Items) an
    std::string coinTxt = "COINS: " + std::to_string(collectedCoins);
    drawText(coinTxt.c_str(), cx - 80.0f * scaleFactor, 80.0f * scaleFactor, 3.0f * scaleFactor, { 1, 1, 0, 1 });

    // ITEM: Extra Leben (Preis: 4 Items)
    if (collectedCoins >= 4) {
        if (drawButton(cx - btnW / 2.0f, startY, btnW, btnH, "BUY EXTRA LIFE (4 COINS)")) {
            collectedCoins -= 4;
            lives++;
            SDL_Delay(200);
        }
    }
    else {
        // Ausgegraut wenn nicht genug Geld
        drawText("BUY EXTRA LIFE (4 COINS)", cx - btnW / 2.0f + 20.0f, startY + 15.0f * scaleFactor, 3.0f * scaleFactor, { 0.4f, 0.4f, 0.4f, 1 });
    }

    // Zurück Button
    if (drawButton(cx - btnW / 2.0f, startY + 100.0f * scaleFactor, btnW, btnH, "BACK TO MENU")) {
        gameState = STATE_LEVEL_COMPLETE;
    }
}

void Game::renderLevelComplete() {
    float scaleFactor = (float)winWidth / 800.0f;
    float cx = (float)winWidth / 2.0f;
    float cy = (float)winHeight / 2.0f;
    float btnW = 200.0f * scaleFactor;
    float btnH = 50.0f * scaleFactor;

    drawText("LEVEL COMPLETE", cx - 200.0f * scaleFactor, cy - 100.0f * scaleFactor, 6.0f * scaleFactor, { 0,1,0,1 });

    if (drawButton(cx - btnW / 2.0f, cy, btnW, btnH, "NEXT LEVEL")) gameState = STATE_PLAYING;
    if (drawButton(cx - btnW / 2.0f, cy + 70.0f * scaleFactor, btnW, btnH, "ITEM SHOP")) gameState = STATE_SHOP;
    if (drawButton(cx - btnW / 2.0f, cy + 140.0f * scaleFactor, btnW, btnH, "QUIT")) isRunning = false;
}

void Game::renderGameOver() {
    float scaleFactor = (float)winWidth / 800.0f;
    SDL_SetRenderDrawColorFloat(renderer, 0.2f, 0, 0, 1);
    SDL_RenderClear(renderer);

    float cx = (float)winWidth / 2.0f;
    float cy = (float)winHeight / 2.0f;
    drawText("GAME OVER", cx - 140.0f * scaleFactor, cy - 100.0f * scaleFactor, 8.0f * scaleFactor, { 1,0,0,1 });

    std::string scoreTxt = "SCORE: " + std::to_string(score);
    std::string hiTxt = "HIGH:  " + std::to_string(highScore);
    drawText(scoreTxt.c_str(), cx - 100.0f * scaleFactor, cy, 4.0f * scaleFactor, { 1,1,1,1 });
    drawText(hiTxt.c_str(), cx - 100.0f * scaleFactor, cy + 40.0f * scaleFactor, 4.0f * scaleFactor, { 1, 0.8f, 0, 1 });

    drawText("PRESS SPACE TO RESTART", cx - 220.0f * scaleFactor, cy + 120.0f * scaleFactor, 4.0f * scaleFactor, { 1,1,1,1 });
}

void Game::render() {
    float scaleFactor = (float)winWidth / 800.0f;
    SDL_SetRenderDrawColorFloat(renderer, 0.1f, 0.1f, 0.15f, 1.0f);
    SDL_RenderClear(renderer);

    if (gameState == STATE_MENU) renderMenu();
    else if (gameState == STATE_SETTINGS) renderSettings();
    else if (gameState == STATE_LEVEL_COMPLETE) renderLevelComplete();
    else if (gameState == STATE_SHOP) renderShop();
    else if (gameState == STATE_GAME_OVER) renderGameOver();
    else {
        // PLAYING STATE
        if (texBg) { SDL_RenderTexture(renderer, texBg, NULL, NULL); }

        float sX = (shakeTime > 0) ? (float)(rand() % 6 - 3) * scaleFactor : 0.0f;
        float sY = (shakeTime > 0) ? (float)(rand() % 6 - 3) * scaleFactor : 0.0f;
        auto ApplyShake = [&](SDL_FRect r) { r.x += sX; r.y += sY; return r; };

        // Bricks
        for (const auto& b : bricks) {
            if (b.isActive()) {
                SDL_FRect br = ApplyShake(b.getRect());
                br.x += b.getZOffset() * 0.5f;
                br.y += b.getZOffset() * 0.5f;
                SDL_Texture* t = nullptr;
                if (b.getType() == 1) t = texBrickWood;
                else if (b.getType() == 2) t = texBrickStone;
                else if (b.getType() == 3) t = texBrickGold;
                else t = texBrickGreen;
                if (t) SDL_RenderTexture(renderer, t, NULL, &br);
                else {
                    Color c = b.getColor();
                    SDL_SetRenderDrawColorFloat(renderer, c.r, c.g, c.b, c.a);
                    SDL_RenderFillRect(renderer, &br);
                }
            }
        }

        SDL_FRect pr = ApplyShake(paddle->getRect());

        // --- 1. PADDLE ZUERST ZEICHNEN (HINTERGRUND) ---
        if (texPaddle) SDL_RenderTexture(renderer, texPaddle, NULL, &pr);
        else {
            SDL_SetRenderDrawColorFloat(renderer, 0.6f, 0.6f, 0.8f, 1.0f);
            SDL_RenderFillRect(renderer, &pr);
        }

        // --- 2. PILOT & KNÖPFE (VORDERGRUND, ÜBERLAPPEN) ---

        // Pilot
        float pilotSize = 48.0f * scaleFactor;
        float pX = pr.x + (pr.w - pilotSize) / 2.0f;
        // pY so setzen, dass er unten überlappt, aber im Vordergrund ist
        float pY = pr.y - pilotSize + (30.0f * scaleFactor);
        SDL_FRect pilotRect = { pX, pY, pilotSize, pilotSize };

        if (texPilot) {
            SDL_RenderTexture(renderer, texPilot, NULL, &pilotRect);
        }
        else {
            SDL_SetRenderDrawColorFloat(renderer, 1.0f, 0.6f, 0.2f, 1.0f);
            SDL_RenderFillRect(renderer, &pilotRect);
        }

        // Knöpfe
        const bool* keys = SDL_GetKeyboardState(NULL);
        Color leftBtnColor = { 0.2f, 0.2f, 0.2f, 1.0f }; // Grau
        Color rightBtnColor = { 0.2f, 0.2f, 0.2f, 1.0f };
        Color blackOutline = { 0, 0, 0, 1 };

        if (gameState == STATE_PLAYING) {
            if (keys[SDL_SCANCODE_LEFT]) leftBtnColor = { 0.0f, 1.0f, 0.0f, 1.0f }; // Grün
            if (keys[SDL_SCANCODE_RIGHT]) rightBtnColor = { 0.0f, 1.0f, 0.0f, 1.0f };
        }

        float btnRadius = 6.0f * scaleFactor;
        // Knöpfe direkt auf das Paddle setzen (oben)
        float btnY = pr.y + (5.0f * scaleFactor);
        float btnOffset = 45.0f * scaleFactor;

        // Linker Knopf
        DrawCircle(renderer, pr.x + pr.w / 2.0f - btnOffset, btnY, btnRadius, leftBtnColor, blackOutline);
        // Rechter Knopf
        DrawCircle(renderer, pr.x + pr.w / 2.0f + btnOffset, btnY, btnRadius, rightBtnColor, blackOutline);

        // --- ITEMS & BÄLLE ---
        for (const auto& pi : pointItems) {
            if (pi.active) {
                SDL_FRect ir = { pi.x - pi.radius, pi.y - pi.radius, pi.radius * 2.0f, pi.radius * 2.0f };
                if (texItemPoint) SDL_RenderTexture(renderer, texItemPoint, NULL, &ir);
                else {
                    Color c = pi.color; SDL_SetRenderDrawColorFloat(renderer, c.r, c.g, c.b, c.a);
                    SDL_RenderFillRect(renderer, &ir);
                }
            }
        }
        for (const auto& p : powerups) {
            if (p.active) {
                SDL_FRect pr = p.rect;
                if (texItemPower) SDL_RenderTexture(renderer, texItemPower, NULL, &pr);
                else {
                    Color c = { 0,1,0,1 }; if (p.type == PU_FIRE) c = { 1,0,0,1 }; if (p.type == PU_WIDE) c = { 0,0,1,1 };
                    SDL_SetRenderDrawColorFloat(renderer, c.r, c.g, c.b, 1);
                    SDL_RenderFillRect(renderer, &pr);
                }
            }
        }
        for (const auto& b : balls) {
            if (!b.isActive()) continue;
            SDL_FRect br = b.getRect();
            if (texBall) {
                if (b.isFire()) SDL_SetTextureColorMod(texBall, 255, 100, 100);
                else SDL_SetTextureColorMod(texBall, 255, 255, 255);
                SDL_RenderTexture(renderer, texBall, NULL, &br);
            }
            else {
                Color bc = b.isFire() ? Color{ 1, 0.2f, 0, 1 } : Color{ 1, 1, 1, 1 };
                SDL_SetRenderDrawColorFloat(renderer, bc.r, bc.g, bc.b, 1.0f);
                SDL_RenderFillRect(renderer, &br);
            }
        }
        for (const auto& p : particles) {
            if (p.life > 0) {
                SDL_SetRenderDrawColorFloat(renderer, p.color.r, p.color.g, p.color.b, p.life);
                SDL_FRect pr = { p.x, p.y, 4.0f * scaleFactor, 4.0f * scaleFactor };
                SDL_RenderFillRect(renderer, &pr);
            }
        }
        for (int i = 0; i < lives; i++) {
            SDL_FRect heart = { 10.0f * scaleFactor + (float)i * 25.0f * scaleFactor, (float)winHeight - 30.0f * scaleFactor, 20.0f * scaleFactor, 20.0f * scaleFactor };
            if (texBall) SDL_RenderTexture(renderer, texBall, NULL, &heart);
            else {
                SDL_SetRenderDrawColorFloat(renderer, 0.8f, 0.2f, 0.2f, 1);
                SDL_RenderFillRect(renderer, &heart);
            }
        }

        // HUD
        drawText("HI:", 20.0f * scaleFactor, 20.0f * scaleFactor, 1.0f * scaleFactor, { 1, 0.8f, 0, 1 });
        drawNumber(highScore, 60.0f * scaleFactor, 20.0f * scaleFactor, 1.0f * scaleFactor);

        // Coins Anzeige im Spiel
        drawText("COINS:", (float)winWidth - 250.0f * scaleFactor, 50.0f * scaleFactor, 1.0f * scaleFactor, { 1, 1, 0, 1 });
        drawNumber(collectedCoins, (float)winWidth - 140.0f * scaleFactor, 50.0f * scaleFactor, 1.0f * scaleFactor);

        drawText("SCORE", (float)winWidth - 250.0f * scaleFactor, 20.0f * scaleFactor, 1.5f * scaleFactor, { 1, 1, 1, 1 });
        drawNumber(score, (float)winWidth - 140.0f * scaleFactor, 20.0f * scaleFactor, 1.5f * scaleFactor);
        drawText("LVL", 20.0f * scaleFactor, (float)winHeight - 60.0f * scaleFactor, 1.0f * scaleFactor, { 0,1,1,1 });
        drawNumber(currentLevelIndex, 70.0f * scaleFactor, (float)winHeight - 60.0f * scaleFactor, 1.0f * scaleFactor);
    }

    SDL_RenderPresent(renderer);
}

void Game::drawNumber(int number, float x, float y, float scale) {
    std::string s = std::to_string(number);
    float cursorX = x;
    for (char c : s) {
        int digit = c - '0';
        float w = 10.0f * scale; float h = 20.0f * scale; float t = 2.0f * scale;
        SDL_SetRenderDrawColorFloat(renderer, 1, 1, 1, 1);
        bool segs[7] = { false };
        if (digit != 1 && digit != 4) segs[0] = true;
        if (digit != 1 && digit != 2 && digit != 3 && digit != 7) segs[1] = true;
        if (digit != 5 && digit != 6) segs[2] = true;
        if (digit != 0 && digit != 1 && digit != 7) segs[3] = true;
        if (digit == 0 || digit == 2 || digit == 6 || digit == 8) segs[4] = true;
        if (digit != 2) segs[5] = true;
        if (digit != 1 && digit != 4 && digit != 7) segs[6] = true;
        SDL_FRect r;
        if (segs[0]) { r = { cursorX, y, w, t }; SDL_RenderFillRect(renderer, &r); }
        if (segs[1]) { r = { cursorX, y, t, h / 2.0f }; SDL_RenderFillRect(renderer, &r); }
        if (segs[2]) { r = { cursorX + w - t, y, t, h / 2.0f }; SDL_RenderFillRect(renderer, &r); }
        if (segs[3]) { r = { cursorX, y + h / 2.0f - t / 2.0f, w, t }; SDL_RenderFillRect(renderer, &r); }
        if (segs[4]) { r = { cursorX, y + h / 2.0f, t, h / 2.0f }; SDL_RenderFillRect(renderer, &r); }
        if (segs[5]) { r = { cursorX + w - t, y + h / 2.0f, t, h / 2.0f }; SDL_RenderFillRect(renderer, &r); }
        if (segs[6]) { r = { cursorX, y + h - t, w, t }; SDL_RenderFillRect(renderer, &r); }
        cursorX += w + 5.0f * scale;
    }
}

void Game::run() {
    lastTime = SDL_GetTicks();
    while (isRunning) {
        Uint64 current = SDL_GetTicks();
        float deltaTime = (float)(current - lastTime) / 1000.0f;
        lastTime = current;
        processEvents();
        update(deltaTime);
        render();
    }
}