/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "util.h"

#include <unistd.h>
#include <sstream>
#include <string>

#include "jni_interface.h"

namespace computer_vision {
namespace util {

void CheckGlError(const char* operation) {
  for (GLint error = glGetError(); error; error = glGetError()) {
    LOGE("after %s() glError (0x%x)\n", operation, error);
  }
}

void ThrowJavaException(JNIEnv* env, const char* msg) {
  LOGE("Throw Java exception: %s", msg);
  jclass c = env->FindClass("java/lang/RuntimeException");
  env->ThrowNew(c, msg);
}

// Convenience function used in CreateProgram below.
static GLuint LoadShader(GLenum shader_type, const char* shader_source) {
  GLuint shader = glCreateShader(shader_type);
  if (!shader) {
    return shader;
  }

  glShaderSource(shader, 1, &shader_source, nullptr);
  glCompileShader(shader);
  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

  if (!compiled) {
    GLint info_len = 0;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
    if (!info_len) {
      return shader;
    }

    char* buf = reinterpret_cast<char*>(malloc(info_len));
    if (!buf) {
      return shader;
    }

    glGetShaderInfoLog(shader, info_len, nullptr, buf);
    LOGE("computer_vision::util::Could not compile shader %d:\n%s\n",
         shader_type, buf);
    free(buf);
    glDeleteShader(shader);
    shader = 0;
  }

  return shader;
}

GLuint CreateProgram(AAssetManager* mgr, const char* vertex_shader_file_name,
                     const char* fragment_shader_file_name) {
  std::string VertexShaderContent;
  if (!LoadTextFileFromAssetManager(mgr, vertex_shader_file_name,
                                    &VertexShaderContent)) {
    LOGE("Failed to load file: %s", vertex_shader_file_name);
    return 0;
  }

  std::string FragmentShaderContent;
  if (!LoadTextFileFromAssetManager(mgr, fragment_shader_file_name,
                                    &FragmentShaderContent)) {
    LOGE("Failed to load file: %s", fragment_shader_file_name);
    return 0;
  }

  GLuint vertexShader =
      LoadShader(GL_VERTEX_SHADER, VertexShaderContent.c_str());
  if (!vertexShader) {
    return 0;
  }

  GLuint fragment_shader =
      LoadShader(GL_FRAGMENT_SHADER, FragmentShaderContent.c_str());
  if (!fragment_shader) {
    return 0;
  }

  GLuint program = glCreateProgram();
  if (program) {
    glAttachShader(program, vertexShader);
    CheckGlError("computer_vision::util::glAttachShader");
    glAttachShader(program, fragment_shader);
    CheckGlError("computer_vision::util::glAttachShader");
    glLinkProgram(program);
    GLint link_status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
      GLint buf_length = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &buf_length);
      if (buf_length) {
        char* buf = reinterpret_cast<char*>(malloc(buf_length));
        if (buf) {
          glGetProgramInfoLog(program, buf_length, nullptr, buf);
          LOGE("computer_vision::util::Could not link program:\n%s\n", buf);
          free(buf);
        }
      }
      glDeleteProgram(program);
      program = 0;
    }
  }
  return program;
}

bool LoadTextFileFromAssetManager(AAssetManager* mgr, const char* file_name,
                                  std::string* out_file_text_string) {
  // If the file hasn't been uncompressed, load it to the internal storage.
  // Note that AAsset_openFileDescriptor doesn't support compressed
  // files (.obj).
  AAsset* asset = AAssetManager_open(mgr, file_name, AASSET_MODE_STREAMING);
  if (asset == nullptr) {
    LOGE("Error opening asset %s", file_name);
    return false;
  }

  off_t file_size = AAsset_getLength(asset);
  out_file_text_string->resize(file_size);
  int ret = AAsset_read(asset, &out_file_text_string->front(), file_size);

  if (ret <= 0) {
    LOGE("Failed to open file: %s", file_name);
    AAsset_close(asset);
    return false;
  }

  AAsset_close(asset);
  return true;
}
}  // namespace util
}  // namespace computer_vision
