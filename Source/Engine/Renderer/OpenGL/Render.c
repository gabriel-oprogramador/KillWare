#include "Renderer/Render.h"
#include "Platform/ApiGL.h"

/*static struct {*/
/*} SRS;  // Static Render State*/

void RenderInitialize();
void RenderTerminate();
void RenderUpdate(float DeltaTime);

void RenderInitialize() {
  FColor color;
  color.r = 255;
  color.g = 255;
  color.b = 0;
  color.a = 255;
  RenderSetClearColor(color);
}

void RenderTerminate() {
}

void RenderUpdate(float DeltaTime) {
  RenderClear();
}

void RenderClear() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RenderSetClearColor(FColor Color) {
  FVector4 nColor;
  nColor.x = Color.r / 255.f;
  nColor.y = Color.g / 255.f;
  nColor.z = Color.b / 255.f;
  nColor.w = Color.a / 255.f;
  glClearColor(nColor.x, nColor.y, nColor.z, nColor.w);
}

void RenderSetViewport(FViewport Viewport) {
  glViewport(Viewport.posX, Viewport.posY, Viewport.width, Viewport.height);
}
