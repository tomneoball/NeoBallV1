#pragma once

struct Color {
    float r, g, b, a;
    Color darken(float factor) const {
        return { r * factor, g * factor, b * factor, a };
    }
};

enum PowerUpType { PU_WIDE, PU_FIRE, PU_MULTI };

enum GameState {
    STATE_MENU,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_LEVEL_COMPLETE,
    STATE_SHOP,         // <--- NEU: Shop Zustand
    STATE_GAME_OVER
};

struct PointItem {
    float x, y;
    float velY;
    float radius;
    int value;
    Color color;
    bool active;
};

// Hilfsstruktur für Buttons
struct Button {
    float x, y, w, h;
    const char* label;
    bool hovered;
};