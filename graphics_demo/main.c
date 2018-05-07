#include "SDL.h"

typedef struct {
  int x;
  int y;
  double theta;
} Sprite;

int main(int argc, char* argv[]) {
  SDL_Window* win;
  SDL_Renderer* rend;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(640, 480, 0, &win, &rend);
  SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

  SDL_Surface* surf;
  SDL_Texture* tex;

  surf = SDL_LoadBMP("./graphics_demo/worm.bmp");
  tex = SDL_CreateTextureFromSurface(rend, surf);
  SDL_FreeSurface(surf);

  Sprite sprite = {0, 0, 0.0};

  for(int i=0; i < 100; i++) {
    SDL_SetRenderDrawColor(rend, 128, 128, 128, 255);
    SDL_RenderClear(rend);

    SDL_Rect sprite_rect;

    sprite_rect.x = sprite.x;
    sprite_rect.y = sprite.y;
    sprite_rect.w = 40;
    sprite_rect.h = 40;

    SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);

    SDL_RenderCopyEx(rend, tex, NULL, &sprite_rect, sprite.theta, NULL, SDL_FLIP_NONE);
    SDL_RenderDrawRect(rend, &sprite_rect);

    SDL_RenderPresent(rend);

    sprite.theta += 10.0;
    sprite.y += 5;
    sprite.x += 10;

    SDL_Delay(100);
  }
}


