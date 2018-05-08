/*
 * Copyright 2018 Google Inc. All Rights Reserved.
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

#include <android/bitmap.h>
#include <unistd.h>
#include <sstream>
#include <string>

#include "jni_interface.h"

namespace augmented_image {
namespace util {

// Anonymous namespace for local static variables
namespace {
constexpr char kJniInterfaceClassName[] =
    "com/google/ar/core/examples/c/augmentedimage/JniInterface";
constexpr char kLoadImageMethodName[] = "loadImage";
constexpr char kLoadImageMethodSignature[] =
    "(Ljava/lang/String;)Landroid/graphics/Bitmap;";
constexpr char kLoadTextureMethodName[] = "loadTexture";
constexpr char kLoadTextureMethodSignature[] = "(ILandroid/graphics/Bitmap;)V";
constexpr char kHideFitToScanMethodName[] = "hideFitToScanImage";
constexpr char kHideFitToScanMethodSignature[] =
    "(Lcom/google/ar/core/examples/c/augmentedimage/AugmentedImageActivity;)V";

static jclass jni_class_id = nullptr;
static jmethodID jni_load_image_method_id = nullptr;
static jmethodID jni_load_texture_method_id = nullptr;
static jmethodID jni_hide_fit_to_scan_method_id = nullptr;
}  // namespace

void CheckGlError(const char* operation) {
  GLint error = glGetError();
  if (error) {
    LOGE("after %s() glError (0x%x)\n", operation, error);
    abort();
  }
}

void InitializeJavaMethodIDs() {
  JNIEnv* env = GetJniEnv();
  jclass local_class_id = FindClass(kJniInterfaceClassName);
  jni_class_id = static_cast<jclass>(env->NewGlobalRef(local_class_id));
  jni_load_image_method_id =
      env->GetStaticMethodID(jni_class_id,
                             kLoadImageMethodName,
                             kLoadImageMethodSignature);
  jni_load_texture_method_id =
      env->GetStaticMethodID(jni_class_id,
                             kLoadTextureMethodName,
                             kLoadTextureMethodSignature);
  jni_hide_fit_to_scan_method_id = env->GetStaticMethodID(
      jni_class_id, kHideFitToScanMethodName, kHideFitToScanMethodSignature);
}

void ReleaseJavaMethodIDs() {
  JNIEnv* env = GetJniEnv();
  env->DeleteGlobalRef(jni_class_id);
  jni_class_id = nullptr;
  jni_load_image_method_id = nullptr;
  jni_load_texture_method_id = nullptr;
  jni_hide_fit_to_scan_method_id = nullptr;
}

static jobject CallJavaLoadImage(jstring image_path) {
  JNIEnv* env = GetJniEnv();
  return env->CallStaticObjectMethod(jni_class_id, jni_load_image_method_id,
                                     image_path);
}

static void CallJavaLoadTexture(int target, jobject image_obj) {
  JNIEnv* env = GetJniEnv();
  env->CallStaticVoidMethod(jni_class_id, jni_load_texture_method_id,
                            target, image_obj);
}

static void CallJavaHideFitToScanImage(jobject activity) {
  JNIEnv* env = GetJniEnv();
  env->CallStaticVoidMethod(jni_class_id, jni_hide_fit_to_scan_method_id,
                            activity);
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
    LOGE("augmented_image::util::Could not compile shader %d:\n%s\n",
         shader_type, buf);
    free(buf);
    glDeleteShader(shader);
    shader = 0;
  }

  return shader;
}

GLuint CreateProgram(const char* vertex_source, const char* fragment_source) {
  GLuint vertexShader = LoadShader(GL_VERTEX_SHADER, vertex_source);
  if (!vertexShader) {
    return 0;
  }

  GLuint fragment_shader = LoadShader(GL_FRAGMENT_SHADER, fragment_source);
  if (!fragment_shader) {
    return 0;
  }

  GLuint program = glCreateProgram();
  if (program) {
    glAttachShader(program, vertexShader);
    CheckGlError("augmented_image::util::glAttachShader");
    glAttachShader(program, fragment_shader);
    CheckGlError("augmented_image::util::glAttachShader");
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
          LOGE("augmented_image::util::Could not link program:\n%s\n", buf);
          free(buf);
        }
      }
      glDeleteProgram(program);
      program = 0;
    }
  }
  return program;
}

bool HideFitToScanImage(void* activity) {
  jobject activity_obj = static_cast<jobject>(activity);
  CallJavaHideFitToScanImage(activity_obj);
  return true;
}

bool LoadPngFromAssetManager(int target, const std::string& path) {
  JNIEnv* env = GetJniEnv();
  jstring j_path = env->NewStringUTF(path.c_str());
  jobject image_obj = CallJavaLoadImage(j_path);
  env->DeleteLocalRef(j_path);

  CallJavaLoadTexture(target, image_obj);
  return true;
}

bool LoadImageFromAssetManager(const std::string& path, int* out_width,
                               int* out_height, int* out_stride,
                               uint8_t** out_pixel_buffer) {
  JNIEnv* env = GetJniEnv();
  jstring j_path = env->NewStringUTF(path.c_str());
  jobject image_obj = CallJavaLoadImage(j_path);
  env->DeleteLocalRef(j_path);

  // image_obj contains a Bitmap Java object.
  AndroidBitmapInfo bitmap_info;
  AndroidBitmap_getInfo(env, image_obj, &bitmap_info);

  // Attention: We are only going to support RGBA_8888 format in this sample.
  CHECK(bitmap_info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);

  *out_width = bitmap_info.width;
  *out_height = bitmap_info.height;
  *out_stride = bitmap_info.stride;
  void* jvm_buffer = nullptr;
  CHECK(AndroidBitmap_lockPixels(env, image_obj, &jvm_buffer) ==
        ANDROID_BITMAP_RESULT_SUCCESS);

  // Copy jvm_buffer_address to pixel_buffer_address
  int32_t total_size_in_byte = bitmap_info.stride * bitmap_info.width;
  *out_pixel_buffer = new uint8_t[total_size_in_byte];
  memcpy(*out_pixel_buffer, jvm_buffer, total_size_in_byte);

  // release jvm_buffer back to JVM
  CHECK(AndroidBitmap_unlockPixels(env, image_obj) ==
        ANDROID_BITMAP_RESULT_SUCCESS);
  return true;
}

bool LoadEntireAssetFile(AAssetManager* mgr, const std::string& file_name,
                         std::string* out_file_buffer) {
  AAsset* asset =
      AAssetManager_open(mgr, file_name.c_str(), AASSET_MODE_BUFFER);
  if (asset == nullptr) {
    LOGE("Error opening asset %s", file_name.c_str());
    return false;
  }

  off_t file_size = AAsset_getLength(asset);
  out_file_buffer->resize(file_size);
  int ret = AAsset_read(asset, &out_file_buffer->front(), file_size);

  if (ret < 0 || ret == EOF) {
    LOGE("Failed to open file: %s", file_name.c_str());
    AAsset_close(asset);
    return false;
  }

  AAsset_close(asset);
  return true;
}

bool LoadObjFile(AAssetManager* mgr, const std::string& file_name,
                 std::vector<GLfloat>* out_vertices,
                 std::vector<GLfloat>* out_normals,
                 std::vector<GLfloat>* out_uv,
                 std::vector<GLushort>* out_indices) {
  std::vector<GLfloat> temp_positions;
  std::vector<GLfloat> temp_normals;
  std::vector<GLfloat> temp_uvs;
  std::vector<GLushort> vertex_indices;
  std::vector<GLushort> normal_indices;
  std::vector<GLushort> uv_indices;

  // If the file hasn't been uncompressed, load it to the internal storage.
  // Note that AAsset_openFileDescriptor doesn't support compressed
  // files (.obj).
  std::string file_buffer;
  if (!LoadEntireAssetFile(mgr, file_name, &file_buffer)) {
    return false;
  }
  std::stringstream file_string_stream(file_buffer);

  while (!file_string_stream.eof()) {
    char line_header[128];
    file_string_stream.getline(line_header, 128);

    if (line_header[0] == 'v' && line_header[1] == 'n') {
      // Parse vertex normal.
      GLfloat normal[3];
      int matches = sscanf(line_header, "vn %f %f %f\n", &normal[0], &normal[1],
                           &normal[2]);
      if (matches != 3) {
        LOGE("Format of 'vn float float float' required for each normal line");
        return false;
      }

      temp_normals.push_back(normal[0]);
      temp_normals.push_back(normal[1]);
      temp_normals.push_back(normal[2]);
    } else if (line_header[0] == 'v' && line_header[1] == 't') {
      // Parse texture uv.
      GLfloat uv[2];
      int matches = sscanf(line_header, "vt %f %f\n", &uv[0], &uv[1]);
      if (matches != 2) {
        LOGE("Format of 'vt float float' required for each texture uv line");
        return false;
      }

      temp_uvs.push_back(uv[0]);
      temp_uvs.push_back(uv[1]);
    } else if (line_header[0] == 'v') {
      // Parse vertex.
      GLfloat vertex[3];
      int matches = sscanf(line_header, "v %f %f %f\n", &vertex[0], &vertex[1],
                           &vertex[2]);
      if (matches != 3) {
        LOGE("Format of 'v float float float' required for each vertice line");
        return false;
      }

      temp_positions.push_back(vertex[0]);
      temp_positions.push_back(vertex[1]);
      temp_positions.push_back(vertex[2]);
    } else if (line_header[0] == 'f') {
      // Actual faces information starts from the second character.
      char* face_line = &line_header[1];

      unsigned int vertex_index[4];
      unsigned int normal_index[4];
      unsigned int texture_index[4];

      std::vector<char*> per_vert_info_list;
      char* per_vert_info_list_c_str;
      char* face_line_iter = face_line;
      while ((per_vert_info_list_c_str =
                  strtok_r(face_line_iter, " ", &face_line_iter))) {
        // Divide each faces information into individual positions.
        per_vert_info_list.push_back(per_vert_info_list_c_str);
      }

      bool is_normal_available = false;
      bool is_uv_available = false;
      for (int i = 0; i < per_vert_info_list.size(); ++i) {
        char* per_vert_info;
        int per_vert_infor_count = 0;

        bool is_vertex_normal_only_face =
            (strstr(per_vert_info_list[i], "//") != nullptr);

        char* per_vert_info_iter = per_vert_info_list[i];
        while ((per_vert_info =
                    strtok_r(per_vert_info_iter, "/", &per_vert_info_iter))) {
          // write only normal and vert values.
          switch (per_vert_infor_count) {
            case 0:
              // Write to vertex indices.
              vertex_index[i] = atoi(per_vert_info);  // NOLINT
              break;
            case 1:
              // Write to texture indices.
              if (is_vertex_normal_only_face) {
                normal_index[i] = atoi(per_vert_info);  // NOLINT
                is_normal_available = true;
              } else {
                texture_index[i] = atoi(per_vert_info);  // NOLINT
                is_uv_available = true;
              }
              break;
            case 2:
              // Write to normal indices.
              if (!is_vertex_normal_only_face) {
                normal_index[i] = atoi(per_vert_info);  // NOLINT
                is_normal_available = true;
                break;
              }
              [[clang::fallthrough]];
            // Intentionally falling to default error case because vertex
            // normal face only has two values.
            default:
              // Error formatting.
              LOGE(
                  "Format of 'f int/int/int int/int/int int/int/int "
                  "(int/int/int)' "
                  "or 'f int//int int//int int//int (int//int)' required for "
                  "each face");
              return false;
          }
          per_vert_infor_count++;
        }
      }

      int vertices_count = per_vert_info_list.size();
      for (int i = 2; i < vertices_count; ++i) {
        vertex_indices.push_back(vertex_index[0] - 1);
        vertex_indices.push_back(vertex_index[i - 1] - 1);
        vertex_indices.push_back(vertex_index[i] - 1);

        if (is_normal_available) {
          normal_indices.push_back(normal_index[0] - 1);
          normal_indices.push_back(normal_index[i - 1] - 1);
          normal_indices.push_back(normal_index[i] - 1);
        }

        if (is_uv_available) {
          uv_indices.push_back(texture_index[0] - 1);
          uv_indices.push_back(texture_index[i - 1] - 1);
          uv_indices.push_back(texture_index[i] - 1);
        }
      }
    }
  }

  bool is_normal_available = (!normal_indices.empty());
  bool is_uv_available = (!uv_indices.empty());

  if (is_normal_available && normal_indices.size() != vertex_indices.size()) {
    LOGE("Obj normal indices does not equal to vertex indices.");
    return false;
  }

  if (is_uv_available && uv_indices.size() != vertex_indices.size()) {
    LOGE("Obj UV indices does not equal to vertex indices.");
    return false;
  }

  for (unsigned int i = 0; i < vertex_indices.size(); i++) {
    unsigned int vertex_index = vertex_indices[i];
    out_vertices->push_back(temp_positions[vertex_index * 3]);
    out_vertices->push_back(temp_positions[vertex_index * 3 + 1]);
    out_vertices->push_back(temp_positions[vertex_index * 3 + 2]);
    out_indices->push_back(i);

    if (is_normal_available) {
      unsigned int normal_index = normal_indices[i];
      out_normals->push_back(temp_normals[normal_index * 3]);
      out_normals->push_back(temp_normals[normal_index * 3 + 1]);
      out_normals->push_back(temp_normals[normal_index * 3 + 2]);
    }

    if (is_uv_available) {
      unsigned int uv_index = uv_indices[i];
      out_uv->push_back(temp_uvs[uv_index * 2]);
      out_uv->push_back(temp_uvs[uv_index * 2 + 1]);
    }
  }

  return true;
}

void Log4x4Matrix(float raw_matrix[16]) {
  LOGI(
      "%f, %f, %f, %f\n"
      "%f, %f, %f, %f\n"
      "%f, %f, %f, %f\n"
      "%f, %f, %f, %f\n",
      raw_matrix[0], raw_matrix[1], raw_matrix[2], raw_matrix[3], raw_matrix[4],
      raw_matrix[5], raw_matrix[6], raw_matrix[7], raw_matrix[8], raw_matrix[9],
      raw_matrix[10], raw_matrix[11], raw_matrix[12], raw_matrix[13],
      raw_matrix[14], raw_matrix[15]);
}

void GetTransformMatrixFromAnchor(const ArSession* ar_session,
                                  const ArAnchor* ar_anchor,
                                  glm::mat4* out_model_mat) {
  if (out_model_mat == nullptr) {
    LOGE("util::GetTransformMatrixFromAnchor model_mat is null.");
    return;
  }
  util::ScopedArPose pose(ar_session);
  ArAnchor_getPose(ar_session, ar_anchor, pose.GetArPose());
  ArPose_getMatrix(ar_session, pose.GetArPose(),
                   glm::value_ptr(*out_model_mat));
}

void ConvertRgbaToGrayscale(const uint8_t* image_pixel_buffer, int32_t width,
                            int32_t height, int32_t stride,
                            uint8_t** out_grayscale_buffer) {
  int32_t grayscale_stride = stride / 4;  // Only support RGBA_8888 format
  uint8_t* grayscale_buffer = new uint8_t[grayscale_stride * height];
  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      const uint8_t* pixel = &image_pixel_buffer[w * 4 + h * stride];
      uint8_t r = *pixel;
      uint8_t g = *(pixel + 1);
      uint8_t b = *(pixel + 2);
      grayscale_buffer[w + h * grayscale_stride] =
          static_cast<uint8_t>(0.213f * r + 0.715 * g + 0.072 * b);
    }
  }
  *out_grayscale_buffer = grayscale_buffer;
}

}  // namespace util
}  // namespace augmented_image
