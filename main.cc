#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>
#include "SDL.h"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

using glm::vec2;
using glm::vec3;
using glm::vec4;

struct color {
  uint8_t alpha;
  uint8_t blue;
  uint8_t green;
  uint8_t red;
};

color ColorRGB(uint8_t red, uint8_t green, uint8_t blue) {
  color c;
  c.red = red;
  c.green = green;
  c.blue = blue;
  c.alpha = 0x00;
  return c;
}

struct screen {
  color* framebuffer;
  int32_t width;
  int32_t height;
};

void Initialize(SDL_Window** window,
                SDL_Renderer** renderer,
                SDL_Texture** texture,
                screen* screen);

void Destroy(screen* screen,
             SDL_Window* window,
             SDL_Renderer* renderer,
             SDL_Texture* texture);

vec3 convertGlm(aiVector3D vec) {
  return vec3(vec.x, vec.y, vec.z);
}

void Draw(screen* screen) {
  for (int32_t i = 0; i < screen->height; i++) {
    screen->framebuffer[i + screen->width * i] = ColorRGB(0xff, 0xff, 0xff);
    screen->framebuffer[(screen->width - i - 1) + screen->width * i] =
        ColorRGB(0xff, 0xff, 0xff);
  }
}

void EventLoop(screen* screen, SDL_Renderer* renderer, SDL_Texture* texture) {
  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
      }
    }

    void* texturePixels;
    int pitch;
    SDL_LockTexture(texture, 0, &texturePixels, &pitch);
    memset(screen->framebuffer, 0x00,
           screen->height * screen->width * sizeof(uint32_t));

    Draw(screen);

    memcpy(texturePixels, screen->framebuffer,
           screen->height * screen->width * sizeof(uint32_t));
    SDL_UnlockTexture(texture);
    SDL_RenderCopyEx(renderer, texture, 0, 0, 0, 0, SDL_FLIP_VERTICAL);
    SDL_RenderPresent(renderer);
  }
}

int main() {
  screen screen = {};
  screen.width = 1024;
  screen.height = 768;

  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  Initialize(&window, &renderer, &texture, &screen);

  screen.framebuffer =
      (color*)malloc(screen.height * screen.width * sizeof(color));

  EventLoop(&screen, renderer, texture);

  Destroy(&screen, window, renderer, texture);
}

void Initialize(SDL_Window** window,
                SDL_Renderer** renderer,
                SDL_Texture** texture,
                screen* screen) {
  SDL_Init(SDL_INIT_EVERYTHING);
  *window = SDL_CreateWindow("title", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, screen->width,
                             screen->height, 0);

  *renderer = SDL_CreateRenderer(*window, -1, 0);

  *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888,
                               SDL_TEXTUREACCESS_STREAMING, screen->width,
                               screen->height);
}

void Destroy(screen* screen,
             SDL_Window* window,
             SDL_Renderer* renderer,
             SDL_Texture* texture) {
  free(screen->framebuffer);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
