/*
 * Copyright 2017 Google LLC
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

namespace hello_ar_vulkan {
namespace util {

void ThrowJavaException(JNIEnv* env, const char* msg) {
  LOGE("Throw Java exception: %s", msg);
  jclass c = env->FindClass("java/lang/RuntimeException");
  env->ThrowNew(c, msg);
}

float CalculateDistanceToPlane(const ArSession& ar_session,
                               const ArPose& plane_pose,
                               const ArPose& camera_pose) {
  float plane_pose_raw[7] = {0.f};
  ArPose_getPoseRaw(&ar_session, &plane_pose, plane_pose_raw);
  glm::vec3 plane_position(plane_pose_raw[4], plane_pose_raw[5],
                           plane_pose_raw[6]);
  glm::vec3 normal = GetPlaneNormal(ar_session, plane_pose);

  float camera_pose_raw[7] = {0.f};
  ArPose_getPoseRaw(&ar_session, &camera_pose, camera_pose_raw);
  glm::vec3 camera_P_plane(camera_pose_raw[4] - plane_position.x,
                           camera_pose_raw[5] - plane_position.y,
                           camera_pose_raw[6] - plane_position.z);
  return glm::dot(normal, camera_P_plane);
}

bool LoadTextFileFromAssetManager(const char* file_name,
                                  AAssetManager* asset_manager,
                                  std::string* out_file_text_string) {
  // If the file hasn't been uncompressed, load it to the internal storage.
  // Note that AAsset_openFileDescriptor doesn't support compressed
  // files (.obj).
  AAsset* asset =
      AAssetManager_open(asset_manager, file_name, AASSET_MODE_STREAMING);
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

bool LoadPngFromAssetManager(const char* path, uint32_t& width,
                             uint32_t& height, VkDeviceSize& image_size,
                             void*& pixel_data) {
  // TODO: b/434017796 - Optimize this
  JNIEnv* env = GetJniEnv();

  // Find the Java class and the loadImageData method
  static jclass helper_class = nullptr;
  static jmethodID load_image_method = nullptr;

  if (helper_class == nullptr) {
    jclass local_helper_class =
        FindClass("com/google/ar/core/examples/c/helloarvulkan/JniInterface");
    if (local_helper_class == nullptr) {
      LOGE("Failed to find JniInterface class");
      return false;
    }
    // Cache for reuse
    helper_class = static_cast<jclass>(env->NewGlobalRef(local_helper_class));
    env->DeleteLocalRef(local_helper_class);

    load_image_method = env->GetStaticMethodID(
        helper_class, "loadImageData",
        "(Ljava/lang/String;)Lcom/google/ar/core/examples/c/helloarvulkan/"
        "JniInterface$ImageData;");
    if (load_image_method == nullptr) {
      LOGE("Failed to find loadImageData method");
      env->DeleteGlobalRef(helper_class);
      helper_class = nullptr;
      return false;
    }
  }
  // Call the Java method
  jstring j_path = env->NewStringUTF(path);
  jobject image_data_obj =
      env->CallStaticObjectMethod(helper_class, load_image_method, j_path);
  env->DeleteLocalRef(j_path);

  // If the image failed to load, return false.
  if (image_data_obj == nullptr) {
    LOGE("Failed to load image: %s", path);
    return false;
  }

  // Get the fields from the returned ImageData Java object
  jclass image_data_class = env->GetObjectClass(image_data_obj);
  jfieldID width_field = env->GetFieldID(image_data_class, "width", "I");
  jfieldID height_field = env->GetFieldID(image_data_class, "height", "I");
  jfieldID buffer_field =
      env->GetFieldID(image_data_class, "buffer", "Ljava/nio/ByteBuffer;");

  // Assign the values to the output reference parameters
  width = env->GetIntField(image_data_obj, width_field);
  height = env->GetIntField(image_data_obj, height_field);

  jobject byte_buffer_obj = env->GetObjectField(image_data_obj, buffer_field);

  void* java_pixel_data = env->GetDirectBufferAddress(byte_buffer_obj);
  image_size = env->GetDirectBufferCapacity(byte_buffer_obj);
  pixel_data = malloc(image_size);
  memcpy(pixel_data, java_pixel_data, image_size);

  return true;  // Indicate success
}

void Log4x4Matrix(const float raw_matrix[16]) {
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

void GetTransformMatrixFromAnchor(const ArAnchor& ar_anchor,
                                  ArSession* ar_session,
                                  glm::mat4* out_model_mat) {
  if (out_model_mat == nullptr) {
    LOGE("util::GetTransformMatrixFromAnchor model_mat is null.");
    return;
  }
  util::ScopedArPose pose(ar_session);
  ArAnchor_getPose(ar_session, &ar_anchor, pose.GetArPose());
  ArPose_getMatrix(ar_session, pose.GetArPose(),
                   glm::value_ptr(*out_model_mat));
}

glm::vec3 GetPlaneNormal(const ArSession& ar_session,
                         const ArPose& plane_pose) {
  float plane_pose_raw[7] = {0.f};
  ArPose_getPoseRaw(&ar_session, &plane_pose, plane_pose_raw);
  glm::quat plane_quaternion(plane_pose_raw[3], plane_pose_raw[0],
                             plane_pose_raw[1], plane_pose_raw[2]);
  // Get normal vector, normal is defined to be positive Y-position in local
  // frame.
  const glm::vec3 positive_y_direction(0., 1.f, 0.);
  return glm::rotate(plane_quaternion, positive_y_direction);
}

// Helper function to parse float data from a line
bool ParseVertexAttribute(const char* line_header, const char* format,
                          std::vector<float>& out_data, int expected_matches) {
  float values[3];  // Max 3 components for v/vn
  int matches = 0;
  if (expected_matches == 2) {
    matches = sscanf(line_header, format, &values[0], &values[1]);
  } else if (expected_matches == 3) {
    matches = sscanf(line_header, format, &values[0], &values[1], &values[2]);
  }

  if (matches != expected_matches) {
    LOGE("Failed to parse line with format '%s': %s", format, line_header);
    return false;
  }

  for (int i = 0; i < expected_matches; ++i) {
    out_data.push_back(values[i]);
  }
  return true;
}

bool LoadObjFile(const std::string& file_name, AAssetManager* asset_manager,
                 std::vector<float>& out_vertices,
                 std::vector<float>& out_normals, std::vector<float>& out_uv,
                 std::vector<uint16_t>& out_indices) {
  std::vector<float> temp_positions;
  std::vector<float> temp_normals;
  std::vector<float> temp_uvs;
  std::vector<uint16_t> vertex_indices;
  std::vector<uint16_t> normal_indices;
  std::vector<uint16_t> uv_indices;

  std::string file_buffer;
  const bool read_success = LoadTextFileFromAssetManager(
      file_name.c_str(), asset_manager, &file_buffer);
  if (!read_success) {
    return false;
  }
  std::stringstream file_string_stream(file_buffer);

  while (!file_string_stream.eof()) {
    char line_header[128];
    file_string_stream.getline(line_header, 128);

    if (line_header[0] == 'v' && line_header[1] == 'n') {
      if (!ParseVertexAttribute(line_header, "vn %f %f %f", temp_normals, 3))
        return false;
    } else if (line_header[0] == 'v' && line_header[1] == 't') {
      if (!ParseVertexAttribute(line_header, "vt %f %f", temp_uvs, 2))
        return false;
    } else if (line_header[0] == 'v') {
      if (!ParseVertexAttribute(line_header, "v %f %f %f", temp_positions, 3))
        return false;
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
          switch (per_vert_infor_count) {
            case 0:
              // Write to vertex indices.
              vertex_index[i] = atoi(per_vert_info);  // NOLINT;
              break;
            case 1:
              // Write to texture indices.
              if (is_vertex_normal_only_face) {
                normal_index[i] = atoi(per_vert_info);  // NOLINT
                is_normal_available = true;
              } else {
                texture_index[i] = atoi(per_vert_info);  // NOLINT;
                is_uv_available = true;
              }
              break;
            case 2:
              // Write to normal indices.
              if (!is_vertex_normal_only_face) {
                normal_index[i] = atoi(per_vert_info);  // NOLINT;
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
    out_vertices.push_back(temp_positions[vertex_index * 3]);
    out_vertices.push_back(temp_positions[vertex_index * 3 + 1]);
    out_vertices.push_back(temp_positions[vertex_index * 3 + 2]);
    out_indices.push_back(i);

    if (is_normal_available) {
      unsigned int normal_index = normal_indices[i];
      out_normals.push_back(temp_normals[normal_index * 3]);
      out_normals.push_back(temp_normals[normal_index * 3 + 1]);
      out_normals.push_back(temp_normals[normal_index * 3 + 2]);
    }

    if (is_uv_available) {
      unsigned int uv_index = uv_indices[i];
      out_uv.push_back(temp_uvs[uv_index * 2]);
      out_uv.push_back(temp_uvs[uv_index * 2 + 1]);
    }
  }

  return true;
}

}  // namespace util
}  // namespace hello_ar_vulkan
