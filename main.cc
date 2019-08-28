#include "SDL.h"
#include <cstdlib>
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

struct color {
  unsigned char alpha;
  unsigned char blue;
  unsigned char green;
  unsigned char red;
};
color ColorRGB(uchar red, uchar green, uchar blue) {
  color c;
  c.red = red;
  c.green = green;
  c.blue = blue;
  c.alpha = 0x00;
  return c;
}

struct screenData {
  color* framebuffer;
  float* depthbuffer;
  int width;
  int height;
};

void Initialize(SDL_Window** window, SDL_Renderer** renderer,
                SDL_Texture** texture, screenData* screenData);

glm::vec4 BoundingBox(glm::vec4 v1, glm::vec4 v2, 
                      glm::vec4 v3) {
  float minx = std::min(v1.x, std::min(v2.x, v3.x));
  float miny = std::min(v1.y, std::min(v2.y, v3.y));
  float maxx = std::max(v1.x, std::max(v2.x, v3.x));
  float maxy = std::max(v1.y, std::max(v2.y, v3.y));

  return glm::vec4(minx, miny, maxx, maxy);
}

glm::vec3 Baricenter(glm::vec3 p, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) { 
  glm::vec3 v21 = v2-v1;
  glm::vec3 v31 = v3-v1;
  glm::vec3 vp1 =  v1 - p;

  glm::vec3 x = glm::vec3(v21.x, v31.x, vp1.x);
  glm::vec3 y = glm::vec3(v21.y, v31.y, vp1.y);
  glm::vec3 cross  = glm::cross(x,y);

  float u = cross.x / cross.z;
  float v = cross.y / cross.z;

  return glm::vec3(u, v, 1-u-v);
}

bool PointInTriangle(glm::vec3 p, glm::vec3 v1, glm::vec3 v2, 
                     glm::vec3 v3) {
  glm::vec3 bari =  Baricenter(p, v1, v2, v3);
  if (bari.x < 0 || bari.y < 0 || (bari.x + bari.y) > 1.f) {
    return false;
  }
  return true;
}

void DrawTriangle(screenData* screenData, glm::vec4 v1, glm::vec4 v2, glm::vec4 v3) { 
  glm::vec4 bbox = BoundingBox(v1, v2, v3);
  float minx = bbox[0];
  float miny = bbox[1];
  float maxx = bbox[2];
  float maxy = bbox[3];
  glm::vec3 lightDir(0, 0, 1);
  glm::vec3 normal = glm::normalize(glm::cross(glm::vec3(v1-v2), glm::vec3(v1-v3)));
  float magnitude = glm::dot(lightDir, normal);
  for (int y = miny; y <= maxy; y++) {
    for (int x = minx; x <= maxx; x++) {
      if (PointInTriangle(glm::vec3(x, y, 1), glm::vec3(v1), 
                          glm::vec3(v2), glm::vec3(v3))) {
        if (magnitude >= 0.f) {
          screenData->framebuffer[x + y * screenData->width] = ColorRGB(
            (uchar) (magnitude * 0xff),
            (uchar) (magnitude * 0xff),
            (uchar) (magnitude * 0xff)
          );
        }
      }
    }
  }
}

void Destroy(screenData* screenData, SDL_Window* window,
             SDL_Renderer* renderer, SDL_Texture* texture);


glm::vec4 assimpToGlm(const aiVector3D& aivert) {
  return glm::vec4(aivert.x, aivert.y, aivert.z, 1.f);
}

void Draw(screenData* screenData, const aiScene* scene) {
  const aiMesh* mesh = scene->mMeshes[0];
  float depth = 255;
  float w = screenData->width;
  float h = screenData->height;
  glm::mat4 rotateZ(-1, 0, 0, 0,
                    0, -1 ,0, 0,
                    0, 0, 1, 0,
                    0,0,0,1);
  glm::mat4 scale(w/2.0f,0,0,0,
                  0,h/2.0f,0,0,
                  0,0,depth,0,
                  0,0,0,1);
  glm::mat4 translate(1,0,0,0,
                      0,1,0,0,
                      0,0,1,0,
                      w/2.0f, h/2.0f, 0 , 1);
  //glm::mat4 scale = glm::scale(glm::mat4(1),glm::vec3(w/3.f, h/3.f, 1));
  //glm::mat4 translate  = glm::translate(glm::mat4(1), glm::vec3(10 + w/3.f, 10 + h/3.f, 0));
  glm::mat4 viewport = translate * scale * rotateZ;

  for (int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    glm::vec4 v1 = viewport *
      assimpToGlm(mesh->mVertices[face.mIndices[0]]);
    glm::vec4 v2 = viewport *
      assimpToGlm(mesh->mVertices[face.mIndices[1]]);
    glm::vec4 v3 = viewport *
      assimpToGlm(mesh->mVertices[face.mIndices[2]]);
    DrawTriangle(screenData, v1, v2, v3);
  }

}

void EventLoop(screenData* screenData, const aiScene* scene, 
               SDL_Renderer* renderer, SDL_Texture* texture) {
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
    memset(screenData->framebuffer, 0xff,
      screenData->height * screenData->width * sizeof(unsigned int));

    Draw(screenData, scene);

    memcpy(texturePixels, screenData->framebuffer,
      screenData->height * screenData->width * sizeof(unsigned int));
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

  screenData screenData;
  screenData.width = 640;
  screenData.height = 480;

  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  Initialize(&window, &renderer, &texture, &screenData);

  screenData.framebuffer = (color*)calloc(
    screenData.height * screenData.width,  sizeof(color));

  EventLoop(&screenData, scene, renderer, texture);
 
  Destroy(&screenData, window, renderer, texture);
  aiReleaseImport(scene);
}

void Initialize(SDL_Window** window, SDL_Renderer** renderer,
                SDL_Texture** texture, screenData* screenData) {
  SDL_Init(SDL_INIT_EVERYTHING);
  *window = SDL_CreateWindow(
    "title",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    screenData->width, screenData->height, 0);

  *renderer = SDL_CreateRenderer(*window, -1, 0);

  *texture = SDL_CreateTexture(
    *renderer,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_STREAMING,
    screenData->width,screenData->height);
}

void Destroy(screenData* screenData, SDL_Window* window,
             SDL_Renderer* renderer, SDL_Texture* texture) {
  free(screenData->framebuffer);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
