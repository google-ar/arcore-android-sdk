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

#ifndef C_ARCORE_HELLOE_AR_OBJ_RENDERER_
#define C_ARCORE_HELLOE_AR_OBJ_RENDERER_
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/asset_manager.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "arcore_c_api.h"
#include "glm.h"

namespace hello_ar {

// PlaneRenderer renders ARCore plane type.
class ObjRenderer {
 public:
  ObjRenderer() = default;
  ~ObjRenderer() = default;

  // Loads the OBJ file and texture and sets up OpenGL resources used to draw
  // the model.  Must be called on the OpenGL thread prior to any other calls.
  void InitializeGlContent(AAssetManager* asset_manager,
                           const std::string& obj_file_name,
                           const std::string& png_file_name);

  // Sets the surface's lighting reflectace properties.  Diffuse is modulated by
  // the texture's color.
  void SetMaterialProperty(float ambient, float diffuse, float specular,
                           float specular_power);

  // Draws the model.
  void Draw(const glm::mat4& projection_mat, const glm::mat4& view_mat,
            const glm::mat4& model_mat, const float* color_correction4,
            const float* object_color4) const;

  void SetUvTransformMatrix(const glm::mat3& uv_transform) {
    uv_transform_ = uv_transform;
  }

  void SetDepthTexture(int texture_id, int width, int height) {
    depth_texture_id_ = texture_id;
    depth_aspect_ratio_ = (float)width / (float)height;
  }

  // Specifies whether to use the depth texture to perform depth-based occlusion
  // of virtual objects from real-world geometry.
  //
  // This function is a no-op if the value provided is the same as what is
  // already set. If the value changes, this function will recompile and reload
  // the shader program to either enable/disable depth-based occlusion. NOTE:
  // recompilation of the shader is inefficient. This code could be optimized to
  // precompile both versions of the shader.
  //
  // @param context Context for loading the shader.
  // @param useDepthForOcclusion Specifies whether to use the depth texture to
  // perform occlusion during rendering of virtual objects.
  void setUseDepthForOcclusion(AAssetManager* asset_manager,
                               bool use_depth_for_occlusion);

 private:
  void compileAndLoadShaderProgram(AAssetManager* asset_manager);

  // Shader material lighting pateremrs
  float ambient_ = 0.0f;
  float diffuse_ = 2.0f;
  float specular_ = 0.5f;
  float specular_power_ = 6.0f;

  // Model attribute arrays
  std::vector<GLfloat> vertices_;
  std::vector<GLfloat> uvs_;
  std::vector<GLfloat> normals_;

  // Model triangle indices
  std::vector<GLushort> indices_;

  // Loaded TEXTURE_2D object name
  GLuint texture_id_;
  GLuint depth_texture_id_;

  // Shader program details
  GLuint shader_program_;
  GLint position_attrib_;
  GLint tex_coord_attrib_;
  GLint normal_attrib_;
  GLint mvp_mat_uniform_;
  GLint mv_mat_uniform_;
  GLint texture_uniform_;
  GLint lighting_param_uniform_;
  GLint material_param_uniform_;
  GLint color_correction_param_uniform_;
  GLint color_uniform_;
  GLint depth_texture_uniform_;
  GLint depth_uv_transform_uniform_;
  GLint depth_aspect_ratio_uniform_;

  bool use_depth_for_occlusion_ = false;
  float depth_aspect_ratio_ = 0.0f;
  glm::mat3 uv_transform_ = glm::mat3(1.0f);
};
}  // namespace hello_ar

#endif  // C_ARCORE_HELLOE_AR_OBJ_RENDERER_
