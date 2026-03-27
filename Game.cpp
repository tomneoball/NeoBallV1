#include "Game.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>

Game::Game() : lives(3), ballStuckToPaddle(true), gameState(STATE_MENU), score(0), highScore(0), currentLevelIndex(1) {
    loadSettings();
    updateScaleFactor();
}

Game::~Game() {
    if (texBg) SDL_DestroyTexture(texBg);
    if (texPaddle) SDL_DestroyTexture(texPaddle);
    if (texBall) SDL_DestroyTexture(texBall);
    if (texBrickWood) SDL_DestroyTexture(texBrickWood);
    if (texBrickStone) SDL_DestroyTexture(texBrickStone);
    if (texBrickGold) SDL_DestroyTexture(texBrickGold);
    if (texBrickGreen) SDL_DestroyTexture(texBrickGreen);
    if (texItemPower) SDL_DestroyTexture(texItemPower);
    if (texItemPoint) SDL_DestroyTexture(texItemPoint);
    if (texWhitePixel) SDL_DestroyTexture(texWhitePixel);

    delete paddle;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::updateScaleFactor() {
    float scaleX = (float)winWidth / 800.0f;
    float scaleY = (float)winHeight / 600.0f;
    scaleFactor = std::min(scaleX, scaleY);
    arenaTopBoundary = (float)winHeight * 0.12f;
}

void Game::loadSettings() {
    std::ifstream file("settings.cfg");
    if (file.is_open()) {
        if (!(file >> winWidth >> winHeight)) { winWidth = 800; winHeight = 600; }
        if (!(file >> highScore)) { highScore = 0; }
        file.close();
    }
    else { winWidth = 800; winHeight = 600; highScore = 0; }
}

void Game::saveSettings() {
    std::ofstream file("settings.cfg");
    if (file.is_open()) {
        file << winWidth << " " << winHeight << " " << highScore;
        file.close();
    }
}

void Game::changeResolution(int w, int h) {
    winWidth = w; winHeight = h;
    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    updateScaleFactor();
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
    SDL_Surface* surf = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA8888);
    SDL_FillSurfaceRect(surf, NULL, SDL_MapRGB(SDL_GetPixelFormatDetails(surf->format), NULL, 255, 255, 255));
    texWhitePixel = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    srand((unsigned int)time(0));
    loadTextures();
    updateScaleFactor();
    paddle = new Paddle((float)winWidth / 2.0f - (50.0f * scaleFactor), (float)winHeight - (50.0f * scaleFactor), scaleFactor);
    lives = 3; score = 0; currentLevelIndex = 1;
    gameState = STATE_MENU;
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
    texBrickWood = loadTexture("brick_wood.bmp");
    texBrickStone = loadTexture("brick_stone.bmp");
    texBrickGold = loadTexture("brick_gold.bmp");
    texBrickGreen = loadTexture("brick_green.bmp");
    texItemPower = loadTexture("item_power.bmp");
    texItemPoint = loadTexture("item_point.bmp");
}

void Game::resetBall() {
    balls.clear();
    paddle->setWidth(100.0f);
    SDL_FRect pRect = paddle->getRect();
    float ballSize = 16.0f * scaleFactor;
    Ball b(pRect.x + pRect.w / 2.0f - ballSize / 2.0f, pRect.y - ballSize - 2.0f, 0.0f, 0.0f, scaleFactor);
    balls.push_back(b);
    ballStuckToPaddle = true;
}

void Game::createBrick(float x, float y, int type, BrickShape shape) {
    int health = (type == 1) ? 1 : (type == 2 ? 2 : 3);
    float r, g, b;
    if (type == 1) { r = 0.8f; g = 0.5f; b = 0.2f; }
    else if (type == 2) { r = 0.5f; g = 0.5f; b = 0.6f; }
    else if (type == 3) { r = 0.9f; g = 0.8f; b = 0.1f; }
    else { r = 0.2f; g = 0.8f; b = 0.2f; }
    bricks.push_back(Brick(x, y, health, type, shape, { r, g, b, 1.0f }, scaleFactor));
}

void Game::loadLevel(int levelIndex) {
    bricks.clear(); powerups.clear(); pointItems.clear(); particles.clear();

    float levelDesignWidth = 800.0f * scaleFactor;
    float offsetX = (winWidth - levelDesignWidth) / 2.0f;
    float brickW = 60.0f * scaleFactor;
    float brickH = 30.0f * scaleFactor;

    // Bereich für Bricks definieren (Innerhalb der Arena)
    float startY = arenaTopBoundary + 40.0f * scaleFactor;
    float endY = (float)winHeight * 0.6f; // Nur bis 60% des Bildschirms
    float minX = offsetX + 50.0f * scaleFactor;
    float maxX = offsetX + levelDesignWidth - 50.0f * scaleFactor - brickW;

    if (levelIndex == 1) {
        // RANDOM CHAOS LEVEL
        int numBricks = 40; // Anzahl der Steine
        int tries = 0;
        int placed = 0;

        while (placed < numBricks && tries < 2000) {
            tries++;

            // Zufällige Position
            float rx = minX + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (maxX - minX)));
            float ry = startY + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (endY - startY)));

            SDL_FRect newRect = { rx, ry, brickW, brickH };

            // Check Collision mit existierenden Bricks
            bool overlaps = false;
            for (const auto& b : bricks) {
                SDL_FRect existing = b.getRect();
                // Kleiner Puffer (5px), damit sie nicht kleben
                SDL_FRect padded = { existing.x - 5, existing.y - 5, existing.w + 10, existing.h + 10 };
                if (SDL_HasRectIntersectionFloat(&newRect, &padded)) {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps) {
                // Zufälliger Typ (1=Holz, 2=Stein, 3=Gold)
                int typeRoll = rand() % 100;
                int type = 1;
                if (typeRoll > 50) type = 2;
                if (typeRoll > 85) type = 3;

                // Zufällige Form (Rechteck, Dreieck, Penta)
                BrickShape shape = SHAPE_RECT;
                if (type == 2 && (rand() % 3 == 0)) shape = SHAPE_TRIANGLE; // Steine manchmal Dreiecke
                if (type == 3 && (rand() % 2 == 0)) shape = SHAPE_PENTA;    // Gold oft Fünfecke

                createBrick(rx, ry, type, shape);
                placed++;
            }
        }
    }
    // Andere Level bleiben vorerst klassisch (oder können wir später auch ändern)
    else if (levelIndex == 2) {
        float bX = offsetX + 200.0f * scaleFactor;
        float spacingX = 70.0f * scaleFactor;
        float spacingY = 40.0f * scaleFactor;
        for (int x = 0; x < 5; x++) for (int y = 2; y < 6; y++)
            if (x == 0 || x == 4 || y == 2 || y == 5) createBrick(bX + (float)x * spacingX, startY + (float)y * spacingY, 2, SHAPE_RECT);
        createBrick(offsetX + 340.0f * scaleFactor, startY, 3, SHAPE_PENTA);
        // ... (Rest von Level 2 vereinfacht für Übersicht, kannst du auffüllen wie vorher)
    }
    else {
        // Fallback für Level 3 & 4 (einfach ein paar randoms zum Testen)
        for (int i = 0; i < 30; i++) createBrick(offsetX + (rand() % 600) * scaleFactor, startY + (rand() % 300) * scaleFactor, 1, SHAPE_RECT);
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
    if ((powerups.size() + pointItems.size()) >= 4) return;
    int roll = rand() % 100;
    float size = 20.0f * scaleFactor;
    if (roll < 20) {
        PowerUp pu; pu.rect = { x, y, size, size }; pu.active = true; pu.type = (PowerUpType)(rand() % 3); powerups.push_back(pu);
    }
    else if (roll < 60) {
        PointItem pi; pi.x = x; pi.y = y; pi.active = true;
        int pRoll = rand() % 100;
        if (pRoll < 10) { pi.value = 50; pi.radius = 6.0f * scaleFactor; pi.velY = 250.0f * scaleFactor; pi.color = { 1,0,0,1 }; }
        else if (pRoll < 35) { pi.value = 25; pi.radius = 9.0f * scaleFactor; pi.velY = 180.0f * scaleFactor; pi.color = { 1,0.5f,0,1 }; }
        else if (pRoll < 65) { pi.value = 20; pi.radius = 10.0f * scaleFactor; pi.velY = 150.0f * scaleFactor; pi.color = { 1,1,0,1 }; }
        else { pi.value = 10; pi.radius = 14.0f * scaleFactor; pi.velY = 100.0f * scaleFactor; pi.color = { 0.2f,1,0.2f,1 }; }
        pointItems.push_back(pi);
    }
}

void Game::spawnParticles(float x, float y, Color c) {
    for (int i = 0; i < 6; i++) {
        Particle p; p.x = x; p.y = y;
        p.velX = (float)(rand() % 200 - 100) * scaleFactor;
        p.velY = (float)(rand() % 200 - 100) * scaleFactor;
        p.life = 1.0f; p.color = c;
        particles.push_back(p);
    }
}

void Game::processEvents() {
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
                if (event.key.key == SDLK_SPACE) { lives = 3; score = 0; currentLevelIndex = 1; loadLevel(1); resetBall(); gameState = STATE_PLAYING; }
            }
        }
    }
}

// ---------------------------------------------------------
// 3D Rendering Helpers
// ---------------------------------------------------------

SDL_FPoint Game::transform3D(float x, float y) {
    float centerX = (float)winWidth / 2.0f;
    float normalizedY = y / (float)winHeight;
    float perspectiveScale = 0.5f + (0.5f * normalizedY);
    float relX = x - centerX;
    float screenX = centerX + (relX * perspectiveScale);
    float screenY = arenaTopBoundary + (normalizedY * ((float)winHeight - arenaTopBoundary));
    return { screenX, screenY };
}

SDL_FPoint Game::transform3DWithHeight(float x, float y, float zHeight) {
    float centerX = (float)winWidth / 2.0f;
    float normalizedY = y / (float)winHeight;
    float perspectiveScale = 0.5f + (0.5f * normalizedY);
    float relX = x - centerX;
    float screenX = centerX + (relX * perspectiveScale);
    float screenY = arenaTopBoundary + (normalizedY * ((float)winHeight - arenaTopBoundary));
    screenY -= zHeight * perspectiveScale;
    return { screenX, screenY };
}

void Game::drawQuad(SDL_FPoint p1, SDL_FPoint p2, SDL_FPoint p3, SDL_FPoint p4, Color c) {
    SDL_Vertex vertices[4];
    SDL_FColor col = { c.r, c.g, c.b, c.a };
    vertices[0].position = p1; vertices[0].color = col; vertices[0].tex_coord = { 0,0 };
    vertices[1].position = p2; vertices[1].color = col; vertices[1].tex_coord = { 0,0 };
    vertices[2].position = p3; vertices[2].color = col; vertices[2].tex_coord = { 0,0 };
    vertices[3].position = p4; vertices[3].color = col; vertices[3].tex_coord = { 0,0 };
    int indices[] = { 0, 1, 2, 0, 2, 3 };
    SDL_RenderGeometry(renderer, texWhitePixel, vertices, 4, indices, 6);
}

// ----------------------------------------------------------------------------------
// SPEZIALISIERTE RENDERER FÜR FORMEN
// ----------------------------------------------------------------------------------

void Game::renderBrickRect3D(SDL_Texture* tex, SDL_FRect rect, Color c) {
    if (!tex) tex = texWhitePixel;
    SDL_FPoint p1 = transform3D(rect.x, rect.y);
    SDL_FPoint p2 = transform3D(rect.x + rect.w, rect.y);
    SDL_FPoint p3 = transform3D(rect.x + rect.w, rect.y + rect.h);
    SDL_FPoint p4 = transform3D(rect.x, rect.y + rect.h);

    SDL_Vertex vertices[4];
    SDL_FColor col = { c.r, c.g, c.b, c.a };
    vertices[0].position = p1; vertices[0].color = col; vertices[0].tex_coord = { 0.0f, 0.0f };
    vertices[1].position = p2; vertices[1].color = col; vertices[1].tex_coord = { 1.0f, 0.0f };
    vertices[2].position = p3; vertices[2].color = col; vertices[2].tex_coord = { 1.0f, 1.0f };
    vertices[3].position = p4; vertices[3].color = col; vertices[3].tex_coord = { 0.0f, 1.0f };
    int indices[] = { 0, 1, 2, 0, 2, 3 };
    SDL_RenderGeometry(renderer, tex, vertices, 4, indices, 6);
}

void Game::renderBrickTriangle3D(SDL_Texture* tex, SDL_FRect rect, Color c) {
    if (!tex) tex = texWhitePixel;
    // Dreieck: Ein Punkt oben mittig, zwei unten
    SDL_FPoint pTop = transform3D(rect.x + rect.w / 2.0f, rect.y);
    SDL_FPoint pBotR = transform3D(rect.x + rect.w, rect.y + rect.h);
    SDL_FPoint pBotL = transform3D(rect.x, rect.y + rect.h);

    SDL_Vertex vertices[3];
    SDL_FColor col = { c.r, c.g, c.b, c.a };

    // Wir mappen die Textur so, dass sie ins Dreieck passt
    vertices[0].position = pTop;  vertices[0].color = col; vertices[0].tex_coord = { 0.5f, 0.0f }; // Oben Mitte
    vertices[1].position = pBotR; vertices[1].color = col; vertices[1].tex_coord = { 1.0f, 1.0f }; // Unten Rechts
    vertices[2].position = pBotL; vertices[2].color = col; vertices[2].tex_coord = { 0.0f, 1.0f }; // Unten Links

    int indices[] = { 0, 1, 2 };
    SDL_RenderGeometry(renderer, tex, vertices, 3, indices, 3);
}

void Game::renderBrickPenta3D(SDL_Texture* tex, SDL_FRect rect, Color c) {
    if (!tex) tex = texWhitePixel;
    float midY = rect.y + rect.h * 0.4f;

    SDL_FPoint pTop = transform3D(rect.x + rect.w / 2.0f, rect.y);
    SDL_FPoint pMidR = transform3D(rect.x + rect.w, midY);
    SDL_FPoint pBotR = transform3D(rect.x + rect.w, rect.y + rect.h);
    SDL_FPoint pBotL = transform3D(rect.x, rect.y + rect.h);
    SDL_FPoint pMidL = transform3D(rect.x, midY);

    SDL_Vertex vertices[5];
    SDL_FColor col = { c.r, c.g, c.b, c.a };

    vertices[0].position = pTop;  vertices[0].color = col; vertices[0].tex_coord = { 0.5f, 0.0f };
    vertices[1].position = pMidR; vertices[1].color = col; vertices[1].tex_coord = { 1.0f, 0.4f };
    vertices[2].position = pBotR; vertices[2].color = col; vertices[2].tex_coord = { 1.0f, 1.0f };
    vertices[3].position = pBotL; vertices[3].color = col; vertices[3].tex_coord = { 0.0f, 1.0f };
    vertices[4].position = pMidL; vertices[4].color = col; vertices[4].tex_coord = { 0.0f, 0.4f };

    int indices[] = { 0, 1, 4, 1, 2, 3, 1, 3, 4 };
    SDL_RenderGeometry(renderer, tex, vertices, 5, indices, 9);
}

void Game::renderBillboard(SDL_Texture* tex, float x, float y, float w, float h, Color c) {
    float cx = x + w / 2.0f;
    float cy = y + h / 2.0f;
    SDL_FPoint screenPos = transform3D(cx, cy);
    float normalizedY = cy / (float)winHeight;
    float scale = 0.5f + (0.5f * normalizedY);
    float drawW = w * scale;
    float drawH = h * scale;
    SDL_FRect dst = { screenPos.x - drawW / 2.0f, screenPos.y - drawH / 2.0f, drawW, drawH };
    SDL_SetTextureColorModFloat(tex, c.r, c.g, c.b);
    SDL_SetTextureAlphaModFloat(tex, c.a);
    SDL_RenderTexture(renderer, tex, NULL, &dst);
    SDL_SetTextureColorModFloat(tex, 1, 1, 1);
    SDL_SetTextureAlphaModFloat(tex, 1);
}

void Game::renderArena3D() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    float wallH = 60.0f * scaleFactor;
    float wallThick = 50.0f * scaleFactor;

    SDL_FPoint fl_TL = transform3D(0, arenaTopBoundary);
    SDL_FPoint fl_TR = transform3D((float)winWidth, arenaTopBoundary);
    SDL_FPoint fl_BR = transform3D((float)winWidth, (float)winHeight);
    SDL_FPoint fl_BL = transform3D(0, (float)winHeight);
    drawQuad(fl_TL, fl_TR, fl_BR, fl_BL, { 0.9f, 0.9f, 1.0f, 0.2f });

    Color wallInnerCol = { 0.6f, 0.7f, 0.8f, 0.6f };
    Color wallTopCol = { 0.8f, 0.9f, 1.0f, 0.8f };

    SDL_FPoint wl_In_BL = transform3D(0, (float)winHeight);
    SDL_FPoint wl_In_TL = transform3D(0, arenaTopBoundary);
    SDL_FPoint wl_In_TR = transform3DWithHeight(0, arenaTopBoundary, wallH);
    SDL_FPoint wl_In_BR = transform3DWithHeight(0, (float)winHeight, wallH);
    drawQuad(wl_In_TL, wl_In_TR, wl_In_BR, wl_In_BL, wallInnerCol);

    SDL_FPoint wl_Out_TR = transform3DWithHeight(-wallThick, arenaTopBoundary, wallH);
    SDL_FPoint wl_Out_BR = transform3DWithHeight(-wallThick, (float)winHeight, wallH);
    drawQuad(wl_Out_TR, wl_In_TR, wl_In_BR, wl_Out_BR, wallTopCol);

    SDL_FPoint wr_In_BR = transform3D((float)winWidth, (float)winHeight);
    SDL_FPoint wr_In_TR = transform3D((float)winWidth, arenaTopBoundary);
    SDL_FPoint wr_In_TL = transform3DWithHeight((float)winWidth, arenaTopBoundary, wallH);
    SDL_FPoint wr_In_BL = transform3DWithHeight((float)winWidth, (float)winHeight, wallH);
    drawQuad(wr_In_TL, wr_In_TR, wr_In_BR, wr_In_BL, wallInnerCol);

    SDL_FPoint wr_Out_TL = transform3DWithHeight((float)winWidth + wallThick, arenaTopBoundary, wallH);
    SDL_FPoint wr_Out_BL = transform3DWithHeight((float)winWidth + wallThick, (float)winHeight, wallH);
    drawQuad(wr_Out_TL, wr_In_TL, wr_In_BL, wr_Out_BL, wallTopCol);

    SDL_FPoint wb_In_BL = transform3D(0, arenaTopBoundary);
    SDL_FPoint wb_In_BR = transform3D((float)winWidth, arenaTopBoundary);
    SDL_FPoint wb_In_TR = transform3DWithHeight((float)winWidth, arenaTopBoundary, wallH);
    SDL_FPoint wb_In_TL = transform3DWithHeight(0, arenaTopBoundary, wallH);
    drawQuad(wb_In_TL, wb_In_TR, wb_In_BR, wb_In_BL, wallInnerCol);

    drawQuad(wl_Out_TR, wr_Out_TL, wb_In_TR, wb_In_TL, wallTopCol);

    SDL_SetRenderDrawColorFloat(renderer, 0.9f, 1.0f, 1.0f, 0.9f);
    SDL_RenderLine(renderer, wl_In_BR.x, wl_In_BR.y, wl_In_TR.x, wl_In_TR.y);
    SDL_RenderLine(renderer, wr_In_BL.x, wr_In_BL.y, wr_In_TL.x, wr_In_TL.y);
    SDL_RenderLine(renderer, wb_In_TL.x, wb_In_TL.y, wb_In_TR.x, wb_In_TR.y);
    SDL_RenderLine(renderer, fl_BL.x, fl_BL.y, fl_TL.x, fl_TL.y);
    SDL_RenderLine(renderer, fl_BR.x, fl_BR.y, fl_TR.x, fl_TR.y);
    SDL_RenderLine(renderer, fl_TL.x, fl_TL.y, fl_TR.x, fl_TR.y);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Game::update(float dt) {
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
                score += pi.value;
                if (score > highScore) { highScore = score; saveSettings(); }
            }
            if (pi.y > (float)winHeight) pi.active = false;
        }

        for (auto& p : powerups) {
            if (!p.active) continue;
            p.rect.y += 150.0f * scaleFactor * dt;
            SDL_FRect pRect = paddle->getRect();
            if (SDL_HasRectIntersectionFloat(&p.rect, &pRect)) {
                p.active = false;
                if (p.type == PU_WIDE) paddle->setWidth(150.0f);
                if (p.type == PU_FIRE) for (auto& b : balls) b.setFireball(true);
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
            if (!ballStuckToPaddle) ball.update(dt, winWidth, winHeight, arenaTopBoundary);

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
                    if (brick.getShape() == SHAPE_TRIANGLE || brick.getShape() == SHAPE_PENTA) {
                        float rndMod = ((rand() % 100) / 100.0f) * 100.0f - 50.0f;
                        ball.setVelX(ball.getVelX() + rndMod);
                    }
                    brick.hit();
                    if (!brick.isActive()) {
                        shakeTime = 0.1f;
                        score += 10;
                        if (score > highScore) { highScore = score; saveSettings(); }
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
            if (lives > 0) resetBall(); else {
                gameState = STATE_GAME_OVER;
            }
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
    float cx = (float)winWidth / 2.0f; float cy = (float)winHeight / 2.0f;
    float btnW = 200.0f * scaleFactor; float btnH = 50.0f * scaleFactor;
    drawText("NEOBALL", cx - 120.0f * scaleFactor, 100.0f * scaleFactor, 8.0f * scaleFactor, { 0,1,1,1 });
    drawText("HIGHSCORE:", cx - 100.0f * scaleFactor, 180.0f * scaleFactor, 4.0f * scaleFactor, { 1,1,0,1 });
    drawNumber(highScore, cx + 60.0f * scaleFactor, 180.0f * scaleFactor, 4.0f / 10.0f * 1.5f * scaleFactor);
    const char* playText = (score > 0 || currentLevelIndex > 1) ? "RESUME" : "PLAY";
    if (drawButton(cx - btnW / 2.0f, cy - 60.0f * scaleFactor, btnW, btnH, playText)) gameState = STATE_PLAYING;
    if (drawButton(cx - btnW / 2.0f, cy + 10.0f * scaleFactor, btnW, btnH, "SETTINGS")) gameState = STATE_SETTINGS;
    if (drawButton(cx - btnW / 2.0f, cy + 80.0f * scaleFactor, btnW, btnH, "EXIT")) isRunning = false;
}

void Game::renderSettings() {
    float cx = (float)winWidth / 2.0f;
    float btnW = 300.0f * scaleFactor; float btnH = 40.0f * scaleFactor;
    float startY = 120.0f * scaleFactor; float gap = 55.0f * scaleFactor;
    drawText("RESOLUTION", cx - 120.0f * scaleFactor, 50.0f * scaleFactor, 6.0f * scaleFactor, { 1,1,1,1 });
    if (drawButton(cx - btnW / 2.0f, startY, btnW, btnH, "800 X 600")) changeResolution(800, 600);
    if (drawButton(cx - btnW / 2.0f, startY + gap, btnW, btnH, "1280 X 720")) changeResolution(1280, 720);
    if (drawButton(cx - btnW / 2.0f, startY + gap * 2, btnW, btnH, "1680 X 1050")) changeResolution(1680, 1050);
    if (drawButton(cx - btnW / 2.0f, startY + gap * 3, btnW, btnH, "1920 X 1080")) changeResolution(1920, 1080);
    if (drawButton(cx - btnW / 2.0f, startY + gap * 4, btnW, btnH, "2560 X 1440")) changeResolution(2560, 1440);
    float listEnd = startY + (gap * 5);
    if (drawButton(cx - (200.0f * scaleFactor) / 2.0f, listEnd + 20.0f * scaleFactor, 200.0f * scaleFactor, 50.0f * scaleFactor, "BACK")) gameState = STATE_MENU;
}

void Game::renderLevelComplete() {
    float cx = (float)winWidth / 2.0f; float cy = (float)winHeight / 2.0f;
    float btnW = 200.0f * scaleFactor; float btnH = 50.0f * scaleFactor;
    drawText("LEVEL COMPLETE", cx - 200.0f * scaleFactor, cy - 100.0f * scaleFactor, 6.0f * scaleFactor, { 0,1,0,1 });
    if (drawButton(cx - btnW / 2.0f, cy, btnW, btnH, "NEXT LEVEL")) gameState = STATE_PLAYING;
    if (drawButton(cx - btnW / 2.0f, cy + 70.0f * scaleFactor, btnW, btnH, "QUIT")) isRunning = false;
}

void Game::renderGameOver() {
    SDL_SetRenderDrawColorFloat(renderer, 0.2f, 0, 0, 1);
    SDL_RenderClear(renderer);
    float cx = (float)winWidth / 2.0f; float cy = (float)winHeight / 2.0f;
    drawText("GAME OVER", cx - 140.0f * scaleFactor, cy - 100.0f * scaleFactor, 8.0f * scaleFactor, { 1,0,0,1 });
    if (score >= highScore && score > 0) {
        drawText("NEW HIGHSCORE!", cx - 200.0f * scaleFactor, cy, 5.0f * scaleFactor, { 1,1,0,1 });
    }
    else {
        drawText("SCORE:", cx - 80.0f * scaleFactor, cy, 5.0f * scaleFactor, { 1,1,1,1 });
        drawNumber(score, cx + 80.0f * scaleFactor, cy, 5.0f / 10.0f * 1.5f * scaleFactor);
    }
    drawText("PRESS SPACE TO RESTART", cx - 220.0f * scaleFactor, cy + 80.0f * scaleFactor, 4.0f * scaleFactor, { 1,1,1,1 });
}

void Game::render() {
    SDL_SetRenderDrawColorFloat(renderer, 0.1f, 0.1f, 0.15f, 1.0f);
    SDL_RenderClear(renderer);

    if (gameState == STATE_MENU) {
        renderMenu();
    }
    else if (gameState == STATE_SETTINGS) {
        renderSettings();
    }
    else if (gameState == STATE_LEVEL_COMPLETE) {
        renderLevelComplete();
    }
    else if (gameState == STATE_GAME_OVER) {
        renderGameOver();
    }
    else {
        if (texBg) { SDL_RenderTexture(renderer, texBg, NULL, NULL); }

        renderArena3D();

        float sX = (shakeTime > 0) ? (float)(rand() % 6 - 3) * scaleFactor : 0.0f;
        float sY = (shakeTime > 0) ? (float)(rand() % 6 - 3) * scaleFactor : 0.0f;

        auto ApplyShake = [&](SDL_FRect r) { r.x += sX; r.y += sY; return r; };

        // Bricks Rendering mit Shapes!
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

                if (b.getShape() == SHAPE_RECT) {
                    renderBrickRect3D(t, br, b.getColor());
                }
                else if (b.getShape() == SHAPE_TRIANGLE) {
                    renderBrickTriangle3D(t, br, b.getColor());
                }
                else if (b.getShape() == SHAPE_PENTA) {
                    renderBrickPenta3D(t, br, b.getColor());
                }
            }
        }

        SDL_FRect pr = ApplyShake(paddle->getRect());
        renderBrickRect3D(texPaddle, pr, { 0.6f, 0.6f, 0.8f, 1.0f });

        for (const auto& pi : pointItems) {
            if (pi.active) {
                renderBillboard(texItemPoint, pi.x - pi.radius, pi.y - pi.radius, pi.radius * 2.0f, pi.radius * 2.0f, pi.color);
            }
        }
        for (const auto& p : powerups) {
            if (p.active) {
                Color c = { 0,1,0,1 }; if (p.type == PU_FIRE) c = { 1,0,0,1 }; if (p.type == PU_WIDE) c = { 0,0,1,1 };
                renderBillboard(texItemPower, p.rect.x, p.rect.y, p.rect.w, p.rect.h, c);
            }
        }
        for (const auto& b : balls) {
            if (!b.isActive()) continue;
            SDL_FRect br = b.getRect();
            Color c = { 1,1,1,1 };
            if (b.isFire()) c = { 1, 0.2f, 0, 1 };
            renderBillboard(texBall, br.x, br.y, br.w, br.h, c);
        }
        for (const auto& p : particles) {
            if (p.life > 0) {
                renderBillboard(texWhitePixel, p.x, p.y, 4.0f * scaleFactor, 4.0f * scaleFactor, p.color);
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
        drawNumber(score, (float)winWidth - 150.0f * scaleFactor, 20.0f * scaleFactor, 1.5f * scaleFactor);
        drawText("LVL:", 20.0f * scaleFactor, 20.0f * scaleFactor, 1.5f * scaleFactor, { 1,1,1,1 });
        drawNumber(currentLevelIndex, 80.0f * scaleFactor, 20.0f * scaleFactor, 1.5f * scaleFactor);
        drawText("HI:", 20.0f * scaleFactor, 50.0f * scaleFactor, 1.0f * scaleFactor, { 1,1,0,1 });
        drawNumber(highScore, 60.0f * scaleFactor, 50.0f * scaleFactor, 1.0f * scaleFactor);
    }
    SDL_RenderPresent(renderer);
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
        if (ch >= 'A' && ch <= 'Z') drawChar(ch, cursorX, y, scale, c);
        else if (ch >= '0' && ch <= '9') drawNumber(ch - '0', cursorX, y, scale / 10.0f * 1.5f);
        else if (ch == ':') {
            SDL_SetRenderDrawColorFloat(renderer, c.r, c.g, c.b, c.a);
            SDL_FRect r1 = { cursorX + scale, y + scale, scale, scale };
            SDL_FRect r2 = { cursorX + scale, y + 3 * scale, scale, scale };
            SDL_RenderFillRect(renderer, &r1); SDL_RenderFillRect(renderer, &r2);
        }
        cursorX += (3.0f * scale) + (1.0f * scale);
        text++;
    }
}

bool Game::drawButton(float x, float y, float w, float h, const char* text) {
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
