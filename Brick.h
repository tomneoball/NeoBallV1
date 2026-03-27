#include <SDL3/SDL.h>
#include "Structs.h" // Für Color

// NEU: Die Formen
enum BrickShape { SHAPE_RECT, SHAPE_TRIANGLE, SHAPE_PENTA };

class Brick {
public:
    // Konstruktor angepasst: Nimmt jetzt auch die Shape
    Brick(float x, float y, int hp, int t, BrickShape s, Color c, float scale);

    void hit();
    void updateAnimation(float dt);

    bool isActive() const { return active; }
    SDL_FRect getRect() const { return rect; }
    Color getColor() const { return color; }
    int getType() const { return type; }
    float getZOffset() const { return zOffset; }

    // NEU: Getter für die Form
    BrickShape getShape() const { return shape; }

private:
    SDL_FRect rect;
    int health;
    int type;
    BrickShape shape; // NEU
    Color color;
    bool active;
    float zOffset;
};
