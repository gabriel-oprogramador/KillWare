#include "Renderer/Render.h"
#include "Platform/ApiGL.h"
#include "Platform/Log.h"

FShader ShaderCreate(cstring VertexSource, cstring FragmentSource) {
  char logBuffer[GT_LOG_BUFFER] = {""};
  uint32 vertexShader, fragmentShader, shaderProgram;
  int32 status = 0;

  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, (const char**)&VertexSource, 0);
  glCompileShader(vertexShader);
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if(!status) {
    glGetShaderInfoLog(vertexShader, GT_LOG_BUFFER, NULL, logBuffer);
    GT_ALERT("Vertex: %s", logBuffer);
    glDeleteShader(vertexShader);
    return 0;
  }

  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, (const char**)&FragmentSource, 0);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
  if(!status) {
    glGetShaderInfoLog(fragmentShader, GT_LOG_BUFFER, NULL, logBuffer);
    GT_ALERT("Fragment: %s", logBuffer);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return 0;
  }

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
  if(!status) {
    glGetProgramInfoLog(shaderProgram, GT_LOG_BUFFER, NULL, logBuffer);
    GT_ALERT("Shader Link: %s", logBuffer);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(shaderProgram);
    return 0;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  //TODO:Create arenas for Shader
  return shaderProgram;
}

void ShaderBind(FShader Self) {
  if(Self == 0) {
    glUseProgram(0);
  }
  glUseProgram(Self);
}
