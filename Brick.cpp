#include "Brick.h"

Brick::Brick(float x, float y, int hp, int t, Color c, float scale)
    : health(hp), type(t), color(c), active(true), zOffset(0.0f) {

    // Basisgröße 60x30 skalieren
    rect = { x, y, 60.0f * scale, 30.0f * scale };
}

void Brick::hit() {
    health--;
    zOffset = 5.0f;
    if (health <= 0) active = false;
}

void Brick::updateAnimation(float dt) {
    if (zOffset > 0) {
        zOffset -= 10.0f * dt;
        if (zOffset < 0) zOffset = 0;
    }
}