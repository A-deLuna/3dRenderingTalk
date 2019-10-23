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
  uint8_t* depthbuffer;
  int32_t width;
  int32_t height;
  uint8_t depth;
};

struct resources {
  aiScene const* scene;
};

void Initialize(SDL_Window** window,
                SDL_Renderer** renderer,
                SDL_Texture** texture,
                screen* screen);

vec4 BoundingBox(vec3 v1, vec3 v2, vec3 v3) {
  float minx = std::min(v1.x, std::min(v2.x, v3.x));
  float miny = std::min(v1.y, std::min(v2.y, v3.y));
  float maxx = std::max(v1.x, std::max(v2.x, v3.x));
  float maxy = std::max(v1.y, std::max(v2.y, v3.y));

  return vec4(minx, miny, maxx, maxy);
}

vec3 Baricenter(vec3 p, vec3 v1, vec3 v2, vec3 v3) {
  vec3 v31 = v1 - v3;
  vec3 v32 = v2 - v3;
  vec3 pv3 = v3 - p;

  vec3 x = vec3(v31.x, v32.x, pv3.x);
  vec3 y = vec3(v31.y, v32.y, pv3.y);
  vec3 cross = glm::cross(x, y);

  float u = cross.x / cross.z;
  float v = cross.y / cross.z;

  return vec3(u, v, 1.f - u - v);
}

bool PointInTriangle(vec3 baricenter) {
  return baricenter.x >= 0 && baricenter.y >= 0 &&
         baricenter.x + baricenter.y <= 1.f;
}

void DrawTriangle(screen* screen, vec3 v1, vec3 v2, vec3 v3, vec3 normal) {
  vec4 bbox = BoundingBox(v1, v2, v3);
  float minx = bbox[0];
  float miny = bbox[1];
  float maxx = bbox[2];
  float maxy = bbox[3];
  vec3 lightDir(0, 0, 1);

  float magnitude = -glm::dot(lightDir, normal);
  if (magnitude < 0.f)
    return;

  for (int32_t y = miny; y <= maxy; y++) {
    for (int32_t x = minx; x <= maxx; x++) {
      vec3 baricenter = Baricenter(vec3(x, y, 1), v1, v2, v3);
      if (PointInTriangle(baricenter)) {
        float pointz =
            v1.z * baricenter.x + v2.z * baricenter.y + v3.z * baricenter.z;
        uint8_t* pixelDepth = screen->depthbuffer + (x + y * screen->width);
        if (pointz > *pixelDepth) {
          *pixelDepth = pointz;
          screen->framebuffer[x + y * screen->width] =
              ColorRGB((uint8_t)(magnitude * 0xff), (uint8_t)(magnitude * 0xff),
                       (uint8_t)(magnitude * 0xff));
        }
      }
    }
  }
}

void Destroy(screen* screen,
             SDL_Window* window,
             SDL_Renderer* renderer,
             SDL_Texture* texture);

vec3 convertGlm(aiVector3D vec) {
  return vec3(vec.x, vec.y, vec.z);
}

void Draw(screen* screen, resources* resources) {
  aiMesh const* mesh = resources->scene->mMeshes[0];

  for (size_t i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];

    vec3 v1_model = convertGlm(mesh->mVertices[face.mIndices[0]]);
    vec3 v1_screen = vec3((v1_model.x + 1.f) * screen->width / 2.f,
                          (v1_model.y + 1.f) * screen->height / 2.f,
                          (v1_model.z + 1.f) * screen->depth / 2.f);

    vec3 v2_model = convertGlm(mesh->mVertices[face.mIndices[1]]);
    vec3 v2_screen = vec3((v2_model.x + 1.f) * screen->width / 2.f,
                          (v2_model.y + 1.f) * screen->height / 2.f,
                          (v2_model.z + 1.f) * screen->depth / 2.f);

    vec3 v3_model = convertGlm(mesh->mVertices[face.mIndices[2]]);
    vec3 v3_screen = vec3((v3_model.x + 1.f) * screen->width / 2.f,
                          (v3_model.y + 1.f) * screen->height / 2.f,
                          (v3_model.z + 1.f) * screen->depth / 2.f);

    vec3 normal =
        glm::normalize(glm::cross(v3_model - v1_model, v2_model - v1_model));

    DrawTriangle(screen, v1_screen, v2_screen, v3_screen, normal);
  }
}

void EventLoop(screen* screen,
               resources* resources,
               SDL_Renderer* renderer,
               SDL_Texture* texture) {
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

    memset(screen->depthbuffer, 0x00,
           screen->height * screen->width * sizeof(uint8_t));

    Draw(screen, resources);

    memcpy(texturePixels, screen->framebuffer,
           screen->height * screen->width * sizeof(uint32_t));
    SDL_UnlockTexture(texture);
    SDL_RenderCopyEx(renderer, texture, 0, 0, 0, 0, SDL_FLIP_VERTICAL);
    SDL_RenderPresent(renderer);
  }
}

int main() {
  // Load model
  resources resources = {};
  resources.scene = aiImportFile("african_head/african_head.obj",
                                 aiProcessPreset_TargetRealtime_Fast);
  if (!resources.scene) {
    std::cout << aiGetErrorString();
    exit(0);
  }

  screen screen = {};
  screen.width = 1024;
  screen.height = 768;
  screen.depth = 255;

  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  Initialize(&window, &renderer, &texture, &screen);

  screen.framebuffer =
      (color*)malloc(screen.height * screen.width * sizeof(color));

  screen.depthbuffer =
      (uint8_t*)malloc(screen.height * screen.width * sizeof(uint8_t));

  EventLoop(&screen, &resources, renderer, texture);

  Destroy(&screen, window, renderer, texture);
  aiReleaseImport(resources.scene);
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
