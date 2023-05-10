/*
 * Copyright 2023 Google LLC
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

#ifndef THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_SIMPLEVULKAN_CPP_VULKAN_HANDLER_H_
#define THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_SIMPLEVULKAN_CPP_VULKAN_HANDLER_H_

#include <android/asset_manager.h>
#include <android/native_window.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "android_vulkan_loader.h"
#include "util.h"

// Vulkan call wrapper
#define CALL_VK(func)                                                         \
  {                                                                           \
    VkResult status = (func);                                                 \
    if (VK_SUCCESS != status) {                                               \
      __android_log_print(ANDROID_LOG_ERROR, "Simple Vulkan ",                \
                          "==== Vulkan error %d. File[%s], line[%d]", status, \
                          __FILE__, __LINE__);                                \
      assert(false);                                                          \
    }                                                                         \
  }

namespace simple_vulkan {

/**
 * Class with all the elements required to setup a Vulkan environment, and
 * create the necessary elements to draw on a screen.
 */
class VulkanHandler {
 public:
  enum class ShaderType { kVertexShader, kFragmentShader };

  /**
   * @struct Vertex Information to be processed in the vertex shader.
   */
  struct VertexInfo {
    // pos_x: x-axis position in the screen where the vertex of the image will
    // be rendered. Normalized between -1 (left) and 1 (right).
    float pos_x;
    // pos_y: y-axis position in the screen where the vertex of the image will
    // be rendered. Normalized between -1 (top) and 1 (bottom).
    float pos_y;
    // tex_u: x-axis position of the texture to be renderer. Normalized between
    // 0 (left) and 1 (right).
    float tex_u;
    // tex_v: y-axis position of the texture to be renderer. Normalized between
    // 0 (top) and 1 (bottom).
    float tex_v;
  };

  /**
   * Structure with texture image.
   */
  struct TextureImageInfo {
    VkImageView texture_image_view;
    VkDeviceMemory texture_memory;
    VkImage texture_image;
  };

  /**
   * Structure with swapchain image's relative.
   */
  struct SwapchinImageRelative {
    VkImageView swapchain_view;
    VkFramebuffer frame_buffer;
  };

  /**
   * Vulkan Handler Constructor
   *
   * @param window C counterpart of the android.view.Surface object in Java.
   * @param max_frames_in_flight maximum frames allowed to be computed. Details:
   * https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/max_frames_in_flight
   */
  VulkanHandler(ANativeWindow* window, int max_frames_in_flight);

  /**
   * Vulkan Handler Deconstructor
   */
  ~VulkanHandler();

  /**
   * Get the framebuffer index we should draw in.
   *
   * @param current_frame the index of current frame in the flight.
   *
   * @return The index of the next frame buffer we are going to
   * draw on. -1 if failed to accquire next image index.
   */
  uint32_t AcquireNextImage(int current_frame);

  /**
   * Wait until previous frame with the same index completed.
   *
   * @param current_frame the index of current frame in the flight.
   */
  void WaitForFrame(int current_frame);

  /**
   * Create image based on the provided hardware buffer and bind the rendering
   * command to the command buffer.
   *
   * @param current_frame the index of current frame in the flight.
   * @param hardware_buffer the chunk of memory containing the ARCore camera
   * image info.
   */
  void RenderFromHardwareBuffer(int current_frame,
                                AHardwareBuffer* hardware_buffer);

  /**
   * Check whether the vertices are set for the frame.
   *
   * @param current_frame the index of current frame in the flight.
   *
   * @return whether the vertices are set for the frame.
   */
  bool IsVerticesSetForFrame(int current_frame);

  /**
   * Set the vertices and indices for the frame if it is not set.
   *
   * @param current_frame the index of current frame in the flight.
   * @param vertices the vertices for the frame, usually 4 corners.
   * @param indices the indices for the frame, usually 2 triangles covering
   * 4 corners' vertices.
   */
  void SetVerticesAndIndicesForFrame(int current_frame,
                                     std::vector<VertexInfo> vertices,
                                     std::vector<uint16_t> indices);

  /**
   * Begin the render pass.
   *
   * In Vulkan, all of the rendering happens inside a VkRenderPass. It is not
   * possible to do rendering commands outside of a renderpass, but it is
   * possible to do Compute commands without them.
   *
   * More details:
   * https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Starting-a-render-pass
   *
   * @param current_frame the index of current frame in the flight.
   * @param swapchain_image_index the index of next swapchian image.
   */
  void BeginRenderPass(int current_frame, uint32_t swapchain_image_index);

  /**
   * End the render pass.
   *
   * More details:
   * https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Finishing-up
   *
   * @param current_frame the index of current frame in the flight.
   */
  void EndRenderPass(int current_frame);

  /**
   * Begin the command buffer.
   *
   * More details:
   * https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Command-buffer-recording
   *
   * @param current_frame the index of current frame in the flight.
   */
  void BeginRecordingCommandBuffer(int current_frame);

  /**
   * End the command buffer.
   *
   * More details:
   * https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Finishing-up
   *
   * @param current_frame the index of current frame in the flight.
   */
  void FinishRecordingCommandBuffer(int current_frame);

  /**
   * Submit the command buffer.
   *
   * More details:
   * https://vkguide.dev/docs/chapter-1/vulkan_command_flow/
   *
   * @param current_frame the index of current frame in the flight.
   */
  void SubmitRecordingCommandBuffer(int current_frame);

  /**
   * End the command buffer.
   *
   * More details:
   * https://vkguide.dev/docs/chapter-1/vulkan_command_flow/
   *
   * @param current_frame the index of current frame in the flight.
   */
  void PresentRecordingCommandBuffer(int current_frame,
                                     uint32_t swapchain_image_index);

 private:
  // Creation function of vulkan class. Dependent classes are put into
  // the parametes.
  VkInstance CreateInstance();
  VkSurfaceKHR CreateSurface(VkInstance instance, ANativeWindow* window);
  VkPhysicalDevice CreatePhysicalDevice(VkInstance instance);
  uint32_t GetQueueFamilyIndex(VkPhysicalDevice physical_device);
  VkDevice CreateLogicalDevice(VkPhysicalDevice physical_device,
                               uint32_t queue_family_index);
  VkSurfaceFormatKHR GetSurfaceFormat(VkSurfaceKHR surface,
                                      VkPhysicalDevice physical_device);
  VkSwapchainKHR CreateSwapchain(VkDevice logical_device, VkSurfaceKHR surface,
                                 VkSurfaceCapabilitiesKHR surface_capabilities,
                                 VkSurfaceFormatKHR surface_format,
                                 uint32_t queue_family_index);
  VkRenderPass CreateRenderPass(VkDevice logical_device);
  VkCommandPool CreateCommandPool(VkDevice logical_device,
                                  uint32_t queue_family_index);
  VkDescriptorPool CreateDescriptorPool(VkDevice logical_device,
                                        int max_frames_in_flight);

  VkSampler CreateSampler(VkDevice logical_device,
                          VkSamplerYcbcrConversionInfo sampler_conversion_info);
  VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice logical_device,
                                                  VkSampler sampler);
  VkPipelineLayout CreatePipelineLayout(
      VkDevice logical_device, VkDescriptorSetLayout descriptor_set_layout);
  VkPipeline CreateGraphicsPipeline(VkDevice logical_device,
                                    VkRenderPass render_pass,
                                    VkPipelineLayout pipeline_layout);

  void InitDescriptorSets(VkDevice logical_device,
                          VkDescriptorPool descriptor_pool,
                          VkDescriptorSetLayout descriptor_set_layout,
                          int max_frames_in_flight);
  void InitSwapchainImageRelatives(
      VkDevice logical_device, VkSwapchainKHR swapchain,
      VkRenderPass render_pass, VkSurfaceCapabilitiesKHR surface_capabilities,
      VkSurfaceFormatKHR surface_format, uint32_t swapchain_length);
  void InitCommandBuffers(VkDevice logical_device, VkCommandPool command_pool,
                          int max_frames_in_flight);
  void InitSyncObjects(VkDevice logical_device, int max_frames_in_flight);

  // Cleanup functions
  void CleanTextureImageInfo(int index);
  void CleanVertiesAndIndies(int index);

  // Other reference functions.
  uint32_t FindMemoryType(VkPhysicalDevice physical_device, uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
  VkShaderModule LoadShader(VkDevice logical_device,
                            const uint32_t* const content, size_t size) const;
  void CreateBuffer(VkPhysicalDevice physical_device, VkDeviceSize size,
                    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                    VkBuffer& buffer, VkDeviceMemory& buffer_memory);
  void TransitionImageLayout(VkImage image, VkImageLayout old_layout,
                             VkImageLayout new_layout);

  VkInstance instance_;
  VkSurfaceKHR surface_;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice logical_device_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  VkSurfaceCapabilitiesKHR surface_capabilities_;
  VkSurfaceFormatKHR surface_format_;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkSamplerYcbcrConversion conversion_ = VK_NULL_HANDLE;
  VkSampler sampler_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;
  VkRenderPass render_pass_ = VK_NULL_HANDLE;
  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_;

  int max_frames_in_flight_ = 0;

  uint32_t queue_family_index_;
  uint32_t swapchain_length_;

  // array of frame buffers and views
  std::vector<TextureImageInfo> texture_image_infos_;
  std::vector<VkCommandBuffer> command_buffers_;
  std::vector<SwapchinImageRelative> swapchain_image_relatives_;
  std::vector<VkDescriptorSet> descriptor_sets_;
  std::vector<VkBuffer> index_buffers_;
  std::vector<VkDeviceMemory> index_buffers_memory_;
  std::vector<uint32_t> index_count_;
  std::vector<VkBuffer> vertex_buffers_;
  std::vector<VkDeviceMemory> vertex_buffers_memory_;
  std::vector<VkSemaphore> image_available_semaphores;
  std::vector<VkSemaphore> render_finished_semaphores;
  std::vector<VkFence> fences_;
};

}  // namespace simple_vulkan

#endif  // THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_SIMPLEVULKAN_CPP_VULKAN_HANDLER_H_
