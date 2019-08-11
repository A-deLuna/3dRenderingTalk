#include "SDL.h"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <string>
#include <iostream>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

using uint = unsigned int;
using uchar = unsigned char;
struct Color {
  Color(uchar red, uchar green, uchar blue, uchar alpha) {
    this->red = red;
    this->green = green;
    this->blue = blue;
    this->alpha = alpha;
  }

  unsigned char alpha;
  unsigned char blue;
  unsigned char green;
  unsigned char red;
};

void Initialize(SDL_Window** window, SDL_Renderer** renderer,
                SDL_Texture** texture, int w, int h);

void Destroy(Color* framebuffer, SDL_Window* window,
             SDL_Renderer* renderer, SDL_Texture* texture);


void Draw(Color* framebuffer, const aiScene* scene, int h, int w) {
  const aiMesh* mesh = scene->mMeshes[0];
  glm::mat4 scale(w*3.f/4.0f,0,0,0,
                  0,h*3.f/4.0f,0,0,
                  0,0,1,0,
                  0,0,0,1);
  glm::mat4 translate(1,0,0,0,
                      0,1,0,0,
                      0,0,1,0,
                      w/8.f + w*3.f/4.0f,h/8.f+h*3.f/4.0f,0,1);
  for (int i = 0; i < mesh->mNumVertices; i++) {
    const aiVector3D aivert = mesh->mVertices[i];
    glm::vec4 vertice(aivert.x, aivert.y, aivert.z, 1);
    glm::vec4 screenVert = translate * scale * vertice;
    
    int pixel = (int)(screenVert.x + screenVert.y * w);
    if (pixel>= 0 && pixel < w * h) {
      framebuffer[pixel] = Color(0,0,0,255);
    }
  }
}

void EventLoop(Color* framebuffer, const aiScene* scene, 
               SDL_Renderer* renderer, SDL_Texture* texture,
               int screenWidth, int screenHeight) {
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
    memset(framebuffer, 0xff,
      screenHeight * screenWidth * sizeof(unsigned int));

    Draw(framebuffer, scene, screenHeight, screenWidth);

    memcpy(texturePixels, framebuffer,
      screenHeight * screenWidth * sizeof(unsigned int));
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, 0, 0);
    SDL_RenderPresent(renderer);
  }
}

int main() {
  const aiScene* scene = aiImportFile("african_head/african_head.obj",
                                      aiProcessPreset_TargetRealtime_Fast);
  if(!scene) {
    std::cout << aiGetErrorString();
    exit(0);
  }

  int screenWidth = 640;
  int screenHeight = 480;
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  Initialize(&window, &renderer, &texture, screenWidth, screenHeight);

  Color* framebuffer = (Color*)calloc(
    screenHeight * screenWidth,  sizeof(Color));

  EventLoop(framebuffer, scene, renderer, texture, screenWidth,
            screenHeight);
 
  Destroy(framebuffer, window, renderer, texture);
  aiReleaseImport(scene);
}

void Initialize(SDL_Window** window, SDL_Renderer** renderer,
                SDL_Texture** texture, int w, int h) {
  SDL_Init(SDL_INIT_EVERYTHING);
  *window = SDL_CreateWindow(
    "title",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    w, h, 0);

  *renderer = SDL_CreateRenderer(*window, -1, 0);

  *texture = SDL_CreateTexture(
    *renderer,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_STREAMING,
    w,h);
}

void Destroy(Color* framebuffer, SDL_Window* window,
             SDL_Renderer* renderer, SDL_Texture* texture) {
  free(framebuffer);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
