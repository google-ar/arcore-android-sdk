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

#ifndef THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_BACKGROUND_RENDERER_H_
#define THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_BACKGROUND_RENDERER_H_

#include <android/asset_manager.h>
#include <android/native_window.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "android_vulkan_loader.h"
#include "util.h"
#include "vulkan_handler.h"
namespace hello_ar_vulkan {

class BackgroundRenderer {
 public:
  explicit BackgroundRenderer(VulkanHandler* vulkan_handler);
  ~BackgroundRenderer();

  /**
   * Create image based on the provided hardware buffer and bind the rendering
   * command to the command buffer.
   *
   * @param current_frame the index of current frame in the flight.
   * @param hardware_buffer, passed from hello_ar_vulkan_activity.cc,
   * the chunk of memory containing the ARCore camera
   *
   * image info.
   */
  void DrawBackground(int current_frame, AHardwareBuffer* hardware_buffer);

 private:
  void Init(VkAndroidHardwareBufferFormatPropertiesANDROID format_info);
  void CreateBackgroundPipelineLayout();
  void CreateBackgroundPipeline();
  VkShaderModule LoadShader(const uint32_t* content, size_t size) const;
  VkSampler CreateSampler(VkSamplerYcbcrConversionInfo sampler_conversion_info);
  VkDescriptorPool CreateDescriptorPool();
  VkDescriptorSetLayout CreateDescriptorSetLayout();
  void InitDescriptorSets();

  bool is_initialized_ = false;
  VulkanHandler* vulkan_handler_;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkSamplerYcbcrConversion conversion_ = VK_NULL_HANDLE;
  VkSampler sampler_ = VK_NULL_HANDLE;

  std::vector<util::TextureImageInfo> texture_image_infos_;
  std::vector<VkDescriptorSet> descriptor_sets_;
};
}  // namespace hello_ar_vulkan

#endif  // THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_BACKGROUND_RENDERER_H_
