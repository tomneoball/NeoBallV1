#include "Paddle.h"

Paddle::Paddle(float x, float y, float s) : scale(s) {
    // Basiswerte (800x600) * Scale
    float w = 100.0f * scale;
    float h = 20.0f * scale;
    rect = { x, y, w, h };

    targetWidth = w;
    speed = 500.0f * scale; // Auch Geschwindigkeit skalieren!
}

void Paddle::update(float dt, int windowWidth) {
    const bool* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT]) {
        rect.x -= speed * dt;
    }
    if (keys[SDL_SCANCODE_RIGHT]) {
        rect.x += speed * dt;
    }

    // Begrenzung
    if (rect.x < 0) rect.x = 0;
    if (rect.x + rect.w > (float)windowWidth) rect.x = (float)windowWidth - rect.w;

    // Animation der Breite
    if (rect.w != targetWidth) {
        rect.w = rect.w + (targetWidth - rect.w) * 5.0f * dt;
    }
}

void Paddle::setWidth(float wBase) {
    // Wenn wir z.B. 150 übergeben, wird es auf 150 * scale gesetzt
    targetWidth = wBase * scale;
}