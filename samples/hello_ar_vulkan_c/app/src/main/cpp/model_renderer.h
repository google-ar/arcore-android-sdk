/*
 * Copyright 2025 Google LLC
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

#ifndef THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_MODEL_RENDERER_H_
#define THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_MODEL_RENDERER_H_
#include <android/asset_manager.h>
#include <android/native_window.h>

#include <vector>

#include "arcore_c_api.h"
#include "android_vulkan_loader.h"
#include "glm.h"
#include "util.h"
#include "vulkan_handler.h"
namespace hello_ar_vulkan {

class ModelRenderer {
 public:
  // Structure with position and texture coordinates
  struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
  };
  struct InstanceData {
    glm::mat4 model_view;
    glm::vec4 object_color;
  };
  // Structure with uniform buffers for the vertex shader.
  struct VertexUniformBuffer {
    glm::mat4 projection_matrix;
  };
  // Structure with uniform buffers for the fragment shader.
  struct FragmentUniformBuffer {
    alignas(16) glm::mat4 depth_uv_transform;
    alignas(16) glm::vec4 color_correction_parameters;
    alignas(16) glm::vec4 material_parameters;
    float depth_aspect_ratio;
    float use_depth_for_occlusion;
    // 0: disable, 1: enable.
  };

  ModelRenderer(VulkanHandler* vulkan_handler, AAssetManager* asset_manager);
  ~ModelRenderer();
  /**
   * Creates image based on model, view, and projection matrices, binds the
   * rendering command to the command buffer, and updates the model uniform
   * buffer.
   *
   * @param current_frame the index of current frame in the flight.
   * @param model_matrix the matrix that transforms the model.
   * @param view_matrix the matrix that transforms the view.
   * @param projection_matrix the matrix that transforms the projection.
   * @param texture_transform_matrix the matrix that transforms the texture.
   */
  void DrawModel(int current_frame,
                 std::vector<util::InstanceData>& instance_data,
                 const glm::mat4& projection_matrix,
                 const glm::mat3& texture_transform_matrix,
                 float use_depth_for_occlusion, const float* color_correction);
  /**
   * Updates the projection matrix in the vertex uniform buffer.
   *
   * @param current_frame the index of current frame in the flight.
   * @param projection_matrix the matrix that transforms the projection.
   */
  void UpdateModelProjectionMatrix(int current_frame,
                                   const glm::mat4& projection_matrix);

 private:
  void CreateModelPipeline();
  void CreateTexture();
  void CreateDescriptorSetLayout();
  void CreateDescriptorPool();
  void CreateDescriptorSet();
  void CreateUniformBuffers();
  void CreateModelVertexBuffer();
  void CreateModelIndexBuffer();
  void CreateInstanceBuffers();
  void UpdateFragmentDescriptorSets(int current_frame,
                                    const glm::mat3& texture_transform_matrix,
                                    float use_depth_for_occlusion,
                                    const float* color_correction4);

  VulkanHandler* vulkan_handler_;
  VkShaderModule vertex_shader_module_ = VK_NULL_HANDLE;
  VkShaderModule fragment_shader_module_ = VK_NULL_HANDLE;

  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;

  std::vector<float> vertices_;
  std::vector<float> normals_;
  std::vector<float> uvs_;
  std::vector<uint16_t> indices_;
  std::vector<Vertex> model_vertices_;

  VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
  VkBuffer index_buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
  VkDeviceMemory index_buffer_memory_ = VK_NULL_HANDLE;
  std::vector<VkBuffer> instance_buffers_;
  std::vector<VkDeviceMemory> instance_buffer_memories_;
  std::vector<void*> instance_buffers_mapped_;

  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptor_sets_;
  util::TextureImageInfo texture_image_info_;
  VkSampler sampler_ = VK_NULL_HANDLE;

  std::vector<VkBuffer> fragment_uniform_buffers_;
  std::vector<VkDeviceMemory> fragment_uniform_buffers_memory_;
  std::vector<void*> fragment_uniform_buffers_mapped_;
};
}  // namespace hello_ar_vulkan
#endif  // THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_MODEL_RENDERER_H_
