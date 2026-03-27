#include "Ball.h"

Ball::Ball(float x, float y, float vx, float vy, float scale)
    : velX(vx), velY(vy), active(true), isFireball(false) {

    // Basisgröße 16x16 skalieren
    float size = 16.0f * scale;
    rect = { x, y, size, size };
}

void Ball::update(float dt, int winW, int winH, float topLimit) {
    rect.x += velX * dt;
    rect.y += velY * dt;

    // Links abprallen
    if (rect.x < 0) {
        rect.x = 0;
        velX *= -1;
    }
    // Rechts abprallen
    if (rect.x > (float)winW - rect.w) {
        rect.x = (float)winW - rect.w;
        velX *= -1;
    }

    // Oben abprallen (am neuen topLimit der Arena!)
    if (rect.y < topLimit) {
        rect.y = topLimit;
        velY *= -1;
    }

    // Unten rausfliegen (Game Over Bedingung für diesen Ball)
    if (rect.y > (float)winH) {
        active = false;
    }
}
