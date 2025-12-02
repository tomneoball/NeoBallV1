#define SDL_MAIN_HANDLED 
#include "Game.h"
#include <SDL3/SDL_main.h>

int main(int argc, char* argv[]) {
    SDL_SetMainReady();

    Game game;
    // Wir übergeben nur den Titel, Größe kommt aus Settings-File
    if (game.init("NeoBall Replica")) {
        game.run();
    }
    return 0;
}