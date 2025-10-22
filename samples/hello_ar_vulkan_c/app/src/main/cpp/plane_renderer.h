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

#ifndef THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_PLANE_RENDERER_H_
#define THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_PLANE_RENDERER_H_

#include <vulkan/vulkan.h>

#include <vector>

#include "glm.h"
#include "util.h"
#include "vulkan_handler.h"

namespace hello_ar_vulkan {

class PlaneRenderer {
 public:
  struct PlaneUniformBuffer {
    glm::mat4 projection_view_matrix;
    glm::mat4 view_matrix;
  };

  struct PlaneFragUniformBuffer {
    alignas(16) glm::mat4 depth_uv_transform;
    float depth_aspect_ratio;
    float use_depth_for_occlusion;
  };

  struct PlanePushConstants {
    glm::mat4 model_matrix;
    glm::vec3 plane_normal;
  };

  // Struct to hold parameters for draw calls
  struct PlaneDrawParams {
    glm::mat4 model_matrix;
    glm::vec3 plane_normal;
    uint32_t index_count = 0;
    uint32_t first_index = 0;
    int32_t vertex_offset = 0;
  };

  static constexpr int kMaxPlaneVertices = 1024;
  static constexpr int kMaxPlaneIndices = kMaxPlaneVertices * 3;

  /**
   * Constructor for the plane renderer.
   *
   * @param vulkan_handler The VulkanHandler instance to use for the plane
   * renderer.
   */
  explicit PlaneRenderer(VulkanHandler* vulkan_handler);

  /**
   * Destructor for the plane renderer.
   */
  ~PlaneRenderer();
  PlaneRenderer(const PlaneRenderer&) = delete;
  PlaneRenderer& operator=(const PlaneRenderer&) = delete;

  /**
   * Generates a mesh for a given AR plane and records the commands to draw it.
   *
   * @param current_frame The index of the current frame in flight.
   * @param ar_session The ARCore session, used to get plane details.
   * @param ar_plane The specific AR plane to render.
   * @param view_matrix The camera's view matrix for the current frame.
   * @param projection_matrix The camera's projection matrix for the current
   * frame.
   * @param texture_transform_matrix The matrix to transform texture
   * coordinates.
   * @param use_depth_for_occlusion Whether to use depth for occlusion.
   *
   * @return The number of planes rendered.
   */
  int Draw(int current_frame, const ArSession& ar_session,
           const ArTrackableList* plane_list, const glm::mat4& view_matrix,
           const glm::mat4& projection_matrix,
           const glm::mat3& texture_transform_matrix,
           float use_depth_for_occlusion);

 private:
  // Initializes the uniform buffers for the model.
  void CreateUniformBuffers();
  // Initializes the texture for the plane.
  void CreateTexture();
  // Initializes the descriptor set layout for the plane.
  void CreateDescriptorSetLayout();
  // Initializes the descriptor pool for the plane.
  void CreateDescriptorPool();
  // Initializes the descriptor sets for the plane.
  void CreateDescriptorSets();
  // Initializes the vertex and index buffers for the plane.
  void CreateVertexAndIndexBuffers();
  // Creates the graphics pipeline layout and pipeline for the plane
  void CreatePlanePipeline();
  // Updates the uniform buffers for the plane.
  void UpdateUniformBuffers(int current_frame,
                            const glm::mat4& projection_matrix,
                            const glm::mat4& view_matrix,
                            const glm::mat3& texture_transform_matrix,
                            float use_depth_for_occlusion);

  VulkanHandler* vulkan_handler_;
  VkShaderModule vertex_shader_module_ = VK_NULL_HANDLE;
  VkShaderModule fragment_shader_module_ = VK_NULL_HANDLE;

  // Descriptor resources
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptor_sets_;

  // Uniform buffers for the plane.
  std::vector<VkBuffer> uniform_buffers_;
  // Memory for the uniform buffers.
  std::vector<VkDeviceMemory> uniform_buffers_memory_;
  std::vector<void*> uniform_buffers_mapped_;
  std::vector<VkBuffer> frag_uniform_buffers_;
  std::vector<VkDeviceMemory> frag_uniform_buffers_memory_;
  std::vector<void*> frag_uniform_buffers_mapped_;

  // Vertex and index buffers for the plane.
  std::vector<VkBuffer> vertex_buffers_;
  std::vector<VkDeviceMemory> vertex_buffer_memories_;
  std::vector<void*> vertex_buffers_mapped_;
  std::vector<VkBuffer> index_buffers_;
  std::vector<VkDeviceMemory> index_buffer_memories_;
  std::vector<void*> index_buffers_mapped_;

  // Pipeline resources for the plane
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;

  // Texture resources
  util::TextureImageInfo texture_image_info_;
  VkSampler sampler_ = VK_NULL_HANDLE;
};
}  // namespace hello_ar_vulkan
#endif  // THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_PLANE_RENDERER_H_
