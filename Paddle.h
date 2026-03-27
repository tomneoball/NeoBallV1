#pragma once
#include <SDL3/SDL.h>

class Paddle {
public:
    // scale parameter im Konstruktor
    Paddle(float x, float y, float scale);

    void update(float dt, int windowWidth);
    void setWidth(float wBase); // wBase ist die Breite bei 800x600 (z.B. 100 oder 150)
    SDL_FRect getRect() const { return rect; }

private:
    SDL_FRect rect;
    float targetWidth;
    float speed;
    float scale; // Speichern des Skalierungsfaktors
};