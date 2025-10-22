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

#ifndef THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_POINT_CLOUD_RENDERER_H_
#define THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_POINT_CLOUD_RENDERER_H_

#include <vulkan/vulkan.h>

#include <vector>

#include "arcore_c_api.h"
#include "vulkan_handler.h"

namespace hello_ar_vulkan {
class PointCloudRenderer {
 public:
  static constexpr int kMaxPoints = 4096;

  /**
   * Constructor for PointCloudRenderer.
   *
   * @param vulkan_handler the vulkan handler to be used by the renderer.
   * @param ar_session the ar session to be used by the renderer.
   */

  PointCloudRenderer(VulkanHandler* vulkan_handler, ArSession* ar_session);

  /**
   * Destructor for PointCloudRenderer.
   */

  ~PointCloudRenderer();

  /**
   * Draw the point cloud to the screen.
   *
   * @param current_frame the index of current frame in the flight.
   * @param point_cloud, passed the point cloud to be rendered.
   * @param view_matrix the view matrix provided by ARCore.
   * @param projection_matrix the projection matrix provided by ARCore.
   *
   */

  void Draw(int current_frame, ArPointCloud* point_cloud,
            const glm::mat4& view_matrix, const glm::mat4& projection_matrix);

 private:
  void CreateVertexBuffer();
  void CreatePipeline();

  VulkanHandler* vulkan_handler_;
  ArSession* ar_session_;

  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkShaderModule vertex_shader_module_ = VK_NULL_HANDLE;
  VkShaderModule fragment_shader_module_ = VK_NULL_HANDLE;
  std::vector<VkBuffer> vertex_buffers_;
  std::vector<VkDeviceMemory> vertex_buffer_memories_;
  std::vector<void*> vertex_buffers_mapped_;
};
}  // namespace hello_ar_vulkan

#endif  // THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_POINT_CLOUD_RENDERER_H_
