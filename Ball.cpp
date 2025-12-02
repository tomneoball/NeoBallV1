#include "Ball.h"

Ball::Ball(float x, float y, float vx, float vy, float scale)
    : velX(vx), velY(vy), active(true), isFireball(false) {

    // Basisgröße 16x16 skalieren
    float size = 16.0f * scale;
    rect = { x, y, size, size };
}

void Ball::update(float dt, int winW, int winH) {
    rect.x += velX * dt;
    rect.y += velY * dt;

    if (rect.x < 0) { rect.x = 0; velX *= -1; }
    if (rect.x > (float)winW - rect.w) { rect.x = (float)winW - rect.w; velX *= -1; }
    if (rect.y < 0) { rect.y = 0; velY *= -1; }

    if (rect.y > (float)winH) active = false;
}