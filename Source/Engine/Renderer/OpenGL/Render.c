#include "Renderer/Render.h"
#include "Platform/Platform.h"
#include "Platform/ApiGL.h"
#include "Platform/Log.h"
#include "ThirdParty/STB/stb_image.h"
#include "Runtime/Engine.h"

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
}

void RenderClear() {
  float time = PGetTime();
  float speed = 0.5f;
  float angle = time * speed;
  float r = (sinf(angle) + 1.0f) / 2.0f;
  float g = (sinf(angle + 2.094f) + 1.0f) / 2.0f;
  float b = (sinf(angle + 4.188f) + 1.0f) / 2.0f;
  glClearColor(r, g, b, 1.0f);
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

void RenderDrawPreview() {
  static GLuint VAO = 0, VBO = 0, EBO = 0, TXD = 0;
  static GLuint uColor = 0, uScale = 0;
  static FShader Shader = 0;

  static cstring vertSrc = "#version 330 core\n"
                           "layout (location = 0) in vec3 aVertPos;"
                           "layout (location = 1) in vec2 aTexCoord;"
                           "uniform vec2 uScale;"
                           "out vec2 vTexCoord;"
                           "void main(){"
                           "  vec3 pos = vec3(aVertPos.xy * uScale, aVertPos.z);"
                           "  gl_Position = vec4(pos, 1.0);"
                           "  vTexCoord = aTexCoord;"
                           "}";
  static cstring fragSrc = "#version 330 core\n"
                           "in vec2 vTexCoord;"
                           "uniform sampler2D uMainTex;"
                           "uniform vec4 uColor;"
                           "out vec4 fragColor;"
                           "void main(){"
                           "  vec4 finalColor = (uColor.a > 0.1) ? uColor : vec4(1.0, 0.0, 1.0, 1.0);"
                           "  fragColor = finalColor * texture(uMainTex, vTexCoord);"
                           "}";

  if(!VAO) {
    // clang-format off
    float vertices[] = {
    // [   Position   ]   [ Coord ]
       0.5f,  0.5f, 0.f,  1.f, 1.f,
       0.5f, -0.5f, 0.f,  1.f, 0.f,
      -0.5f, -0.5f, 0.f,  0.f, 0.f,
      -0.5f,  0.5f, 0.f,  0.f, 1.f
    };
    int32 indices[] = {0, 1, 2, 2, 3, 0};
    // clang-format on

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), NULL);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    Shader = ShaderCreate(vertSrc, fragSrc);
    uColor = glGetUniformLocation(Shader, "uColor");
    uScale = glGetUniformLocation(Shader, "uScale");

    PFile logoFile;
    if(PFileOpen("Content/Logo.png", PLATFORM_FILE_READ, &logoFile)) {
      uint8* buffer = PMemAlloc(logoFile.size);
      GT_ASSERT(buffer);
      PFileRead(&logoFile, 0, logoFile.size, buffer, NULL);
      int32 w, h, f;
      stbi_set_flip_vertically_on_load(true);
      stbi_uc* data = stbi_load_from_memory(buffer, logoFile.size, &w, &h, &f, 4);
      GT_ASSERT(data);
      PMemFree(buffer);
      glGenTextures(1, &TXD);
      glBindTexture(GL_TEXTURE_2D, TXD);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      glBindTexture(GL_TEXTURE_2D, 0);
      stbi_image_free(data);
    }
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_DEPTH_TEST);

  const float color[] = {1, 1, 1, 1};
  const float scale[] = {1.0 / GEngine.windowInfo.aspect, 1.0};

  glBindVertexArray(VAO);
  glUseProgram(Shader);

  glUniform4fv(uColor, 1, color);
  glUniform2fv(uScale, 1, scale);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, TXD);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  glBindVertexArray(0);
}
