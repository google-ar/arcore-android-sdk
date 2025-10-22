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

#ifndef THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_VULKAN_HANDLER_H_
#define THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_VULKAN_HANDLER_H_

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
      __android_log_print(ANDROID_LOG_ERROR, "Hello Ar Vulkan ",              \
                          "==== Vulkan error %d. File[%s], line[%d]", status, \
                          __FILE__, __LINE__);                                \
      assert(false);                                                          \
    }                                                                         \
  }

namespace hello_ar_vulkan {

/**
 * Class with all the elements required to setup a Vulkan environment, and
 * create the necessary elements to draw on a screen.
 */
class VulkanHandler {
 public:
  enum class ShaderType { kVertexShader, kFragmentShader };


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
                                     std::vector<util::VertexInfo> vertices,
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

  /**
   * Clean the texture image info.
   *
   * @param texture_image_info the texture image info to clean.
   */

  void CleanTextureImageInfo(util::TextureImageInfo& texture_image_info);

  /**
   * Clean the buffer and its memory.
   *
   * @param buffer the buffer to clean.
   * @param buffer_memory the memory of the buffer to clean.
   * @param mapped_data the mapped data of the buffer to clean.
   */
  void CleanBuffer(VkBuffer& buffer, VkDeviceMemory& buffer_memory,
                   void* mapped_data = nullptr);

  /**
   * Find the memory type that matches the given type filter and properties.
   *
   * @param physical_device the physical device to find the memory type for.
   * @param typeFilter the type filter to match.
   * @param properties the properties to match.
   *
   * @return the memory type that matches the given type filter and properties.
   */
  uint32_t FindMemoryType(VkPhysicalDevice physical_device, uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);

  /**
   * Load a shader from the asset manager.
   *
   * @param content the shader content.
   * @param size the shader size.
   *
   * @return the shader module.
   */
  VkShaderModule LoadShader(VkDevice logical_device,
                            const uint32_t* const content, size_t size) const;

  /**
   * Create a buffer with the given size, usage, and properties.
   *
   * @param physical_device the physical device to create the buffer for.
   * @param size the size of the buffer.
   * @param usage the usage of the buffer.
   * @param properties the properties of the buffer.
   * @param buffer the buffer to create.
   * @param buffer_memory the memory of the buffer.
   */
  void CreateBuffer(VkPhysicalDevice physical_device, VkDeviceSize size,
                    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                    VkBuffer& buffer, VkDeviceMemory& buffer_memory);

  /** Transitions the layout of the given Vulkan image.
   *
   * The behavior regarding CPU blocking depends on the `current_frame`
   * argument:
   * - Synchronous (Blocking): If `current_frame` is -1 (the default), this
   * function will block the calling thread until the layout transition is
   * complete.
   * - Asynchronous (Non-Blocking): If `current_frame` is a non-negative value,
   *   the transition is associated with the specified frame and the function
   *   returns without waiting for completion.
   *
   * @param image: The Vulkan image to transition.
   * @param old_layout: The current layout of the image.
   * @param new_layout: The desired new layout of the image.
   * @param synchronous: Whether to block the calling thread until the layout
   * transition is complete. True by default.
   * @param current_frame: The index of the current frame in flight. Defaults to
   * -1 for synchronous blocking execution. Provide a valid frame index for
   * asynchronous execution.
   */
  void TransitionImageLayout(VkImage image, VkImageLayout old_layout,
                             VkImageLayout new_layout, bool synchronous = true,
                             int current_frame = -1);

  /** Copies data from a Vulkan buffer to a Vulkan image.
   *
   * The behavior regarding CPU blocking depends on the `current_frame`
   * argument:
   * - Synchronous (Blocking): If `current_frame` is -1 (the default), this
   *  function will block the calling thread until the copy operation is
   *  complete.
   * - Asynchronous (Non-Blocking): If `current_frame` is a non-negative value,
   *   the copy is associated with the specified frame, and the function
   *   returns without waiting for completion. Synchronization should be handled
   *   externally, typically using fences or semaphores associated with the
   *   frame.
   *
   * @param buffer the source Vulkan buffer containing the data to copy.
   * @param image the destination Vulkan image.
   * @param width the width of the region to copy in pixels.
   * @param height the height of the region to copy in pixels.
   * @param aspect_mask the aspect mask of the image to copy to.
   * @param synchronous whether to block the calling thread until the copy
   * operation is complete. True by default.
   * @param current_frame the index of current frame in the flight.
   */
  void CopyBufferToImage(
      VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
      VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
      bool synchronous = true, int current_frame = -1);

  /**
   * Copies data from a source buffer to a destination buffer.
   *
   * @param src_buffer the source Vulkan buffer containing the data to copy.
   * @param dst_buffer the destination Vulkan buffer.
   * @param size the size of the data to copy.
   */
  void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

  /**
   * Update the depth texture.
   *
   * @param ar_session the AR session to use for the depth map.
   * @param ar_frame the AR frame to use for the depth map.
   * @param current_frame the index of the current frame in flight.
   */
  void UpdateDepthTexture(ArSession* ar_session, ArFrame* ar_frame,
                          int current_frame);

  VkPhysicalDevice GetPhysicalDevice() { return physical_device_; }
  VkDevice GetLogicalDevice() { return logical_device_; }
  VkQueue GetQueue() { return queue_; }
  VkCommandPool GetCommandPool() { return command_pool_; }
  VkCommandBuffer GetCommandBuffer(int current_frame) {
    return command_buffers_[current_frame];
  }
  VkRenderPass GetRenderPass() { return render_pass_; }
  int GetMaxFramesInFlight() { return max_frames_in_flight_; }
  VkSurfaceCapabilitiesKHR GetSurfaceCapabilities() {
    return surface_capabilities_;
  }

  VkBuffer GetIndexBuffer(int current_frame) {
    return index_buffers_[current_frame];
  }
  VkBuffer* GetVertexBuffer(int current_frame) {
    return &vertex_buffers_[current_frame];
  }

  uint32_t GetIndexCount(int current_frame) {
    return index_count_[current_frame];
  }
  VkImageView GetDepthTextureView(int current_frame) const {
    return depth_texture_infos_[current_frame].texture_image_view;
  }
  VkSampler GetDepthSampler() const { return depth_sampler_; }

  float GetDepthAspectRatio(int current_frame) const {
    return static_cast<float>(depth_texture_widths_[current_frame]) /
           static_cast<float>(depth_texture_heights_[current_frame]);
  }

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

  void InitSwapchainImageRelatives(
      VkDevice logical_device, VkSwapchainKHR swapchain,
      VkRenderPass render_pass, VkSurfaceCapabilitiesKHR surface_capabilities,
      VkSurfaceFormatKHR surface_format, uint32_t swapchain_length);
  void InitCommandBuffers(VkDevice logical_device, VkCommandPool command_pool,
                          int max_frames_in_flight);
  void InitSyncObjects(VkDevice logical_device, int max_frames_in_flight);
  void CreateDepthResources();
  VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  VkFormat FindDepthFormat();
  void CreateImage(uint32_t width, uint32_t height, VkFormat format,
                   VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkImage& image,
                   VkDeviceMemory& image_memory);
  VkImageView CreateImageView(VkImage image, VkFormat format,
                              VkImageAspectFlags aspect_flags);

  // Depth Texture related functions.
  void CreateDepthSampler();
  void RecreateDepthResources(int32_t width, int32_t height,
                              int32_t plane_data_size, int current_frame);
  void DestroyDepthResources(int current_frame);

  // Cleanup functions
  void CleanVerticesAndIndices(int index);

  // Helper functions to begin and end single time commands.
  VkCommandBuffer BeginSingleTimeCommands();
  void EndSingleTimeCommands(VkCommandBuffer& command_buffer);

  VkInstance instance_;
  VkSurfaceKHR surface_;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice logical_device_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  VkSurfaceCapabilitiesKHR surface_capabilities_;
  VkSurfaceFormatKHR surface_format_;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkRenderPass render_pass_ = VK_NULL_HANDLE;
  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  util::TextureImageInfo depth_buffer_info_;

  int max_frames_in_flight_ = 0;

  uint32_t queue_family_index_;
  uint32_t swapchain_length_;

  // array of frame buffers and views
  std::vector<VkCommandBuffer> command_buffers_;
  std::vector<util::SwapchainImageRelative> swapchain_image_relatives_;
  std::vector<VkBuffer> index_buffers_;
  std::vector<VkDeviceMemory> index_buffers_memory_;
  std::vector<uint32_t> index_count_;
  std::vector<VkBuffer> vertex_buffers_;
  std::vector<VkDeviceMemory> vertex_buffers_memory_;
  std::vector<VkSemaphore> image_available_semaphores;
  std::vector<VkSemaphore> render_finished_semaphores;
  std::vector<VkFence> fences_;
  // Depth texture resources.
  std::vector<util::TextureImageInfo> depth_texture_infos_;
  VkSampler depth_sampler_ = VK_NULL_HANDLE;
  std::vector<VkImageLayout> depth_texture_layouts_;
  std::vector<VkBuffer> depth_staging_buffers_;
  std::vector<VkDeviceMemory> depth_staging_buffer_memories_;
  std::vector<void*> mapped_depth_staging_buffers_;
  std::vector<int32_t> depth_texture_widths_;
  std::vector<int32_t> depth_texture_heights_;
};

}  // namespace hello_ar_vulkan

#endif  // THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOARVULKAN_CPP_VULKAN_HANDLER_H_
