#pragma once
#include <SDL3/SDL.h>

class Ball {
public:
    // Scale Parameter im Konstruktor
    Ball(float x, float y, float velX, float velY, float scale);

    void update(float dt, int winW, int winH);

    SDL_FRect getRect() const { return rect; }
    float getVelY() const { return velY; }

    void setPosition(float x, float y) { rect.x = x; rect.y = y; }
    void setVelocity(float vx, float vy) { velX = vx; velY = vy; }

    void invertY() { velY *= -1; }
    void invertX() { velX *= -1; }
    void setVelX(float v) { velX = v; }

    bool isActive() const { return active; }
    void setFireball(bool f) { isFireball = f; }
    bool isFire() const { return isFireball; }

private:
    SDL_FRect rect;
    float velX, velY;
    bool active;
    bool isFireball;
};