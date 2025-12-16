#pragma once
#include <SDL3/SDL.h>

class Ball {
public:
    Ball(float x, float y, float vx, float vy, float scale);

    // Update mit topLimit für die Arena
    void update(float dt, int winW, int winH, float topLimit);

    // --- GETTER & SETTER (Diese haben gefehlt und verursachten C2039) ---
    float getVelX() const { return velX; }
    float getVelY() const { return velY; }

    void setVelX(float vx) { velX = vx; }
    void setVelY(float vy) { velY = vy; }

    // Setzt beides gleichzeitig
    void setVelocity(float vx, float vy) { velX = vx; velY = vy; }

    // Position setzen (für Reset oder Sticky)
    void setPosition(float x, float y) { rect.x = x; rect.y = y; }

    // Richtung umkehren
    void invertY() { velY *= -1; }
    void invertX() { velX *= -1; }

    // Status Abfragen
    SDL_FRect getRect() const { return rect; }
    bool isActive() const { return active; }

    bool isFire() const { return isFireball; }
    void setFireball(bool f) { isFireball = f; }

private:
    SDL_FRect rect;
    float velX, velY;
    bool active;
    bool isFireball;
};
