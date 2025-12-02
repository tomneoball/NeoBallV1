#pragma once
#include <SDL3/SDL.h>
#include "Structs.h"

class Brick {
public:
    // Scale im Konstruktor
    Brick(float x, float y, int hp, int type, Color c, float scale);

    void hit();
    bool isActive() const { return active; }
    SDL_FRect getRect() const { return rect; }
    Color getColor() const { return color; }
    int getType() const { return type; }
    float getZOffset() const { return zOffset; }
    void updateAnimation(float dt);

private:
    SDL_FRect rect;
    bool active;
    int health;
    int type;
    Color color;
    float zOffset;
};