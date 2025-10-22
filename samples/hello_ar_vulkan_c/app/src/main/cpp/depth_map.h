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

#ifndef THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_DEPTH_MAP_H_
#define THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_DEPTH_MAP_H_

#include <cstdint>

#include "arcore_c_api.h"
#include "util.h"
#include "vulkan_handler.h"

namespace hello_ar_vulkan {

class DepthMapRenderer {
 public:
  /**
   * Constructs a DepthMapRenderer.
   *
   * @param vulkan_handler The Vulkan handler to use for the depth map
   *     renderer.
   * @param ar_session The ARCore session to use for the depth map renderer.
   */
  DepthMapRenderer(VulkanHandler* vulkan_handler);

  /**
   * Destructor for the depth map renderer.
   */
  ~DepthMapRenderer();

  /**
   * Draws the depth map for the current frame.
   *
   * @param current_frame The index of the current frame in flight.
   * @param ar_frame The ARCore frame to use for the depth map.
   */
  void DrawDepthMap(int current_frame);

 private:
  // Creates the palette texture from the depth color palette PNG file.
  void CreatePaletteTexture();
  // Creates the descriptor set resources.
  void CreateDescriptorSetLayout();
  // Creates the descriptor pool resources.
  void CreateDescriptorPool();
  // Creates the descriptor sets.
  void CreateDescriptorSets();
  // Creates the depth map pipeline.
  void CreatePipeline();
  // Update the descriptor sets with the depth texture from vulkan_handler.
  void UpdateDescriptorSets(int current_frame);

  VulkanHandler* vulkan_handler_ = nullptr;

  // Depth map shader resources.
  VkShaderModule depth_map_vertex_shader_ = VK_NULL_HANDLE;
  VkShaderModule depth_map_fragment_shader_ = VK_NULL_HANDLE;

  // Palette texture resources.
  util::TextureImageInfo palette_texture_info_;
  VkSampler palette_sampler_ = VK_NULL_HANDLE;

  // Descriptor resources
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptor_sets_;

  // Depth map pipeline resources.
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkViewport viewport_;
  VkRect2D scissor_;
};

}  // namespace hello_ar_vulkan

#endif  // THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_DEPTH_MAP_H_
