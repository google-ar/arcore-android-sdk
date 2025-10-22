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

#include "vulkan_handler.h"

#include <android/log.h>
#include <vulkan/vulkan.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "android_vulkan_loader.h"
#include "util.h"

namespace hello_ar_vulkan {
const uint64_t kFenceTimeoutNs = 100L * 1000L * 1000L;

VulkanHandler::VulkanHandler(ANativeWindow* window, int max_frames_in_flight) {
  CHECK(LoadVulkan());

  max_frames_in_flight_ = max_frames_in_flight;

  instance_ = CreateInstance();
  surface_ = CreateSurface(instance_, window);
  physical_device_ = CreatePhysicalDevice(instance_);
  queue_family_index_ = GetQueueFamilyIndex(physical_device_);
  logical_device_ = CreateLogicalDevice(physical_device_, queue_family_index_);
  vkGetDeviceQueue(logical_device_, queue_family_index_, /* queueIndex=*/0,
                   &queue_);
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_,
                                            &surface_capabilities_);

  CreateDepthResources();
  render_pass_ = CreateRenderPass(logical_device_);
  command_pool_ = CreateCommandPool(logical_device_, queue_family_index_);
  surface_format_ = GetSurfaceFormat(surface_, physical_device_);
  swapchain_ = CreateSwapchain(logical_device_, surface_, surface_capabilities_,
                               surface_format_, queue_family_index_);
  CALL_VK(vkGetSwapchainImagesKHR(logical_device_, swapchain_,
                                  &swapchain_length_,
                                  /* pSwapchainImages=*/nullptr));

  InitSwapchainImageRelatives(logical_device_, swapchain_, render_pass_,
                              surface_capabilities_, surface_format_,
                              swapchain_length_);
  CreateDepthSampler();

  vertex_buffers_.resize(max_frames_in_flight_);
  vertex_buffers_memory_.resize(max_frames_in_flight_);
  index_buffers_.resize(max_frames_in_flight_);
  index_buffers_memory_.resize(max_frames_in_flight_);
  index_count_.resize(max_frames_in_flight_);
  depth_texture_infos_.resize(max_frames_in_flight_);
  depth_texture_layouts_.assign(max_frames_in_flight_,
                                VK_IMAGE_LAYOUT_UNDEFINED);
  depth_staging_buffers_.assign(max_frames_in_flight_, VK_NULL_HANDLE);
  depth_staging_buffer_memories_.assign(max_frames_in_flight_, VK_NULL_HANDLE);
  mapped_depth_staging_buffers_.assign(max_frames_in_flight_, nullptr);
  depth_texture_widths_.assign(max_frames_in_flight_, 0);
  depth_texture_heights_.assign(max_frames_in_flight_, 0);

  InitCommandBuffers(logical_device_, command_pool_, max_frames_in_flight_);
  // Init semaphores and fences
  InitSyncObjects(logical_device_, max_frames_in_flight_);
}

VulkanHandler::~VulkanHandler() {
  CALL_VK(vkWaitForFences(logical_device_, fences_.size(), fences_.data(),
                          VK_TRUE, kFenceTimeoutNs));
  for (uint32_t i = 0; i < max_frames_in_flight_; i++) {
    CleanVerticesAndIndices(i);
    DestroyDepthResources(i);
  }

  vkDestroySampler(logical_device_, depth_sampler_, nullptr);
  vkFreeCommandBuffers(logical_device_, command_pool_, max_frames_in_flight_,
                       command_buffers_.data());
  for (size_t i = 0; i < max_frames_in_flight_; i++) {
    vkDestroySemaphore(logical_device_, render_finished_semaphores[i],
                       /* pAllocator=*/nullptr);
    vkDestroySemaphore(logical_device_, image_available_semaphores[i],
                       /* pAllocator=*/nullptr);
    vkDestroyFence(logical_device_, fences_[i], /* pAllocator=*/nullptr);
  }

  vkDestroyCommandPool(logical_device_, command_pool_, /* pAllocator=*/nullptr);
  vkDestroyRenderPass(logical_device_, render_pass_, /* pAllocator=*/nullptr);
  for (uint32_t i = 0; i < swapchain_length_; i++) {
    vkDestroyFramebuffer(logical_device_,
                         swapchain_image_relatives_[i].frame_buffer,
                         /*pAllocator=*/nullptr);
    vkDestroyImageView(logical_device_,
                       swapchain_image_relatives_[i].swapchain_view,
                       /* pAllocator=*/nullptr);
  }
  CleanTextureImageInfo(depth_buffer_info_);
  vkDestroySwapchainKHR(logical_device_, swapchain_,
                        /* pAllocator=*/nullptr);

  vkDestroyDevice(logical_device_, /* pAllocator=*/nullptr);
  vkDestroySurfaceKHR(instance_, surface_, /* pAllocator=*/nullptr);
  vkDestroyInstance(instance_, /* pAllocator=*/nullptr);
}

uint32_t VulkanHandler::AcquireNextImage(int current_frame) {
  uint32_t out_next_index = -1;
  CALL_VK(vkAcquireNextImageKHR(logical_device_, swapchain_,
                                /* timeout=*/UINT64_MAX,
                                image_available_semaphores[current_frame],
                                /* fence=*/VK_NULL_HANDLE, &out_next_index));

  return out_next_index;
}

void VulkanHandler::WaitForFrame(int current_frame) {
  CALL_VK(vkWaitForFences(logical_device_, /* fenceCount=*/1,
                          &fences_[current_frame], VK_TRUE, kFenceTimeoutNs));
}

bool VulkanHandler::IsVerticesSetForFrame(int current_frame) {
  return vertex_buffers_[current_frame] != VK_NULL_HANDLE;
}

void VulkanHandler::SetVerticesAndIndicesForFrame(
    int current_frame, std::vector<util::VertexInfo> vertices,
    std::vector<uint16_t> indices) {
  CleanVerticesAndIndices(current_frame);

  VkDeviceSize vertex_buffer_size = sizeof(vertices[0]) * vertices.size();
  CreateBuffer(
      physical_device_, vertex_buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vertex_buffers_[current_frame], vertex_buffers_memory_[current_frame]);

  void* vertex_data;
  CALL_VK(vkMapMemory(logical_device_, vertex_buffers_memory_[current_frame], 0,
                      vertex_buffer_size, 0, &vertex_data));
  memcpy(vertex_data, vertices.data(), vertex_buffer_size);
  vkUnmapMemory(logical_device_, vertex_buffers_memory_[current_frame]);

  // Create an index buffer to draw square textures.
  VkDeviceSize index_buffer_size = sizeof(indices[0]) * indices.size();
  CreateBuffer(
      physical_device_, index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      index_buffers_[current_frame], index_buffers_memory_[current_frame]);

  void* index_data;
  vkMapMemory(logical_device_, index_buffers_memory_[current_frame], 0,
              index_buffer_size, 0, &index_data);
  memcpy(index_data, indices.data(), index_buffer_size);
  vkUnmapMemory(logical_device_, index_buffers_memory_[current_frame]);

  index_count_[current_frame] = indices.size();
}

void VulkanHandler::BeginRenderPass(int current_frame,
                                    uint32_t swapchain_image_index) {
  std::array<VkClearValue, 2> clear_vals;
  clear_vals[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clear_vals[1].depthStencil = {1.0f, 0};
  const VkRenderPassBeginInfo render_pass_begin_info{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = render_pass_,
      .framebuffer =
          swapchain_image_relatives_[swapchain_image_index].frame_buffer,
      .renderArea =
          {
              .offset =
                  {
                      .x = 0,
                      .y = 0,
                  },
              .extent = surface_capabilities_.currentExtent,
          },
      .clearValueCount = static_cast<uint32_t>(clear_vals.size()),
      .pClearValues = clear_vals.data()};
  vkCmdBeginRenderPass(command_buffers_[current_frame], &render_pass_begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanHandler::EndRenderPass(int current_frame) {
  vkCmdEndRenderPass(command_buffers_[current_frame]);
}

void VulkanHandler::BeginRecordingCommandBuffer(int current_frame) {
  // We start by creating and declare the "beginning" our command buffer
  VkCommandBufferBeginInfo cmd_buffer_begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = 0,
      .pInheritanceInfo = nullptr,
  };
  CALL_VK(vkBeginCommandBuffer(command_buffers_[current_frame],
                               &cmd_buffer_begin_info));
}

void VulkanHandler::FinishRecordingCommandBuffer(int current_frame) {
  CALL_VK(vkEndCommandBuffer(command_buffers_[current_frame]));
}

void VulkanHandler::SubmitRecordingCommandBuffer(int current_frame) {
  CALL_VK(vkResetFences(logical_device_, 1, &fences_[current_frame]));

  VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame]};
  VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  const VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = wait_semaphores,
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &command_buffers_[current_frame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signal_semaphores};

  VkResult result =
      vkQueueSubmit(queue_, 1, &submit_info, fences_[current_frame]);
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    LOGE("Failed to submit command buffer due to error code %d", result);
  }
}

void VulkanHandler::PresentRecordingCommandBuffer(
    int current_frame, uint32_t swapchain_image_index) {
  VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};
  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signal_semaphores,
      .swapchainCount = 1,
      .pSwapchains = &swapchain_,
      .pImageIndices = &swapchain_image_index,
  };

  VkResult result = vkQueuePresentKHR(queue_, &present_info);
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    LOGE("Failed to present due to error code %d", result);
  }
}

uint32_t VulkanHandler::FindMemoryType(VkPhysicalDevice physical_device,
                                       uint32_t typeFilter,
                                       VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  LOGE("Failed to find suitable memory type!");
  CHECK(false);
}

VkShaderModule VulkanHandler::LoadShader(VkDevice logical_device,
                                         const uint32_t* const content,
                                         size_t size) const {
  VkShaderModule shader;
  VkShaderModuleCreateInfo shader_module_create_info{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .flags = 0,
      .codeSize = size,
      .pCode = content,
  };
  CALL_VK(vkCreateShaderModule(logical_device, &shader_module_create_info,
                               nullptr, &shader));

  return shader;
}

void VulkanHandler::CreateBuffer(VkPhysicalDevice physical_device,
                                 VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer& buffer,
                                 VkDeviceMemory& buffer_memory) {
  VkBufferCreateInfo buffer_info{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                 .size = size,
                                 .usage = usage,
                                 .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

  CALL_VK(vkCreateBuffer(logical_device_, &buffer_info, nullptr, &buffer));

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(logical_device_, buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex = FindMemoryType(
          physical_device, mem_requirements.memoryTypeBits, properties)};

  CALL_VK(
      vkAllocateMemory(logical_device_, &alloc_info, nullptr, &buffer_memory));

  vkBindBufferMemory(logical_device_, buffer, buffer_memory, 0);
}

void VulkanHandler::TransitionImageLayout(VkImage image,
                                          VkImageLayout old_layout,
                                          VkImageLayout new_layout,
                                          bool synchronous, int current_frame) {
  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  if (synchronous) {
    command_buffer = BeginSingleTimeCommands();
  } else {
    CHECK(current_frame >= 0 && current_frame < command_buffers_.size());
    command_buffer = command_buffers_[current_frame];
  }

  VkImageMemoryBarrier barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

  // Set the aspect mask based on the new layout
  if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  VkPipelineStageFlags source_stage = 0;
  VkPipelineStageFlags destination_stage = 0;

  // --- Set Source Stage & Access Mask (based on old_layout) ---
  // Determines when the previous operations must be finished
  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
    barrier.srcAccessMask = 0;
    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    LOGE("Unsupported old layout");
  }

  // --- Set Destination Stage & Access Mask (based on new_layout) ---
  // Determines when the new operations can start
  if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else if (new_layout == VK_IMAGE_LAYOUT_GENERAL) {
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else {
    LOGE("Unsupported new layout");
  }

  vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  if (synchronous) {
    EndSingleTimeCommands(command_buffer);
  }
}

void VulkanHandler::CopyBufferToImage(VkBuffer buffer, VkImage image,
                                      uint32_t width, uint32_t height,
                                      VkImageAspectFlags aspect_mask,
                                      bool synchronous, int current_frame) {
  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  if (synchronous) {
    command_buffer = BeginSingleTimeCommands();
  } else {
    CHECK(current_frame >= 0 && current_frame < command_buffers_.size());
    command_buffer = command_buffers_[current_frame];
  }

  VkBufferImageCopy copy_region = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource =
          {
              .aspectMask = aspect_mask,
              .mipLevel = 0,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
      .imageOffset = {0, 0, 0},
      .imageExtent = {static_cast<uint32_t>(width),
                      static_cast<uint32_t>(height), 1},
  };
  vkCmdCopyBufferToImage(command_buffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
  if (synchronous) {
    EndSingleTimeCommands(command_buffer);
  }
}

void VulkanHandler::CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer,
                               VkDeviceSize size) {
  VkCommandBuffer command_buffer = BeginSingleTimeCommands();

  VkBufferCopy copy_region{};
  copy_region.size = size;
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  EndSingleTimeCommands(command_buffer);
}

// ============================= Private =============================

VkInstance VulkanHandler::CreateInstance() {
  // Create Vulkan Instance
  VkInstance instance;

  std::vector<const char*> instance_extensions;
  instance_extensions.push_back("VK_KHR_surface");
  instance_extensions.push_back("VK_KHR_android_surface");
  instance_extensions.push_back("VK_KHR_get_physical_device_properties2");
  instance_extensions.push_back("VK_KHR_external_memory_capabilities");

  const VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "hello_ar_vulkan",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "vulkan",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_MAKE_VERSION(1, 2, 0),
  };
  const VkInstanceCreateInfo instance_create_info{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount =
          static_cast<uint32_t>(instance_extensions.size()),
      .ppEnabledExtensionNames = instance_extensions.data(),
  };
  CALL_VK(vkCreateInstance(&instance_create_info,
                           /* pAllocator=*/nullptr, &instance));

  return instance;
}

VkSurfaceKHR VulkanHandler::CreateSurface(VkInstance instance,
                                          ANativeWindow* window) {
  // Create Surface
  VkSurfaceKHR surface;
  const VkAndroidSurfaceCreateInfoKHR create_info{
      .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
      .flags = 0,
      .window = window};

  CALL_VK(vkCreateAndroidSurfaceKHR(instance, &create_info,
                                    /* pAllocator=*/nullptr, &surface));
  return surface;
}

VkPhysicalDevice VulkanHandler::CreatePhysicalDevice(VkInstance instance) {
  VkPhysicalDevice physical_device;
  // Find one GPU to use:
  // On Android, every GPU device is equal -- supporting
  // graphics/compute/present
  // for this sample, we use the very first GPU device found on the system
  uint32_t gpu_count = 0;
  CALL_VK(vkEnumeratePhysicalDevices(instance, &gpu_count,
                                     /* pPhysicalDevices=*/nullptr));
  VkPhysicalDevice tmp_gpus[1];
  CALL_VK(vkEnumeratePhysicalDevices(instance, &gpu_count, tmp_gpus));
  physical_device = tmp_gpus[0];  // Pick up the first GPU Device

  return physical_device;
}

uint32_t VulkanHandler::GetQueueFamilyIndex(VkPhysicalDevice physical_device) {
  uint32_t queue_family_index;

  uint32_t queue_family_count;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           nullptr);
  CHECK(queue_family_count);
  std::vector<VkQueueFamilyProperties> queue_family_properties(
      queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           queue_family_properties.data());

  for (queue_family_index = 0; queue_family_index < queue_family_count;
       queue_family_index++) {
    if (queue_family_properties[queue_family_index].queueFlags &
        VK_QUEUE_GRAPHICS_BIT) {
      break;
    }
  }
  CHECK(queue_family_index < queue_family_count);

  return queue_family_index;
}

VkDevice VulkanHandler::CreateLogicalDevice(VkPhysicalDevice physical_device,
                                            uint32_t queue_family_index) {
  VkDevice logical_device;
  std::vector<const char*> device_extensions;
  device_extensions.push_back("VK_KHR_swapchain");
  device_extensions.push_back(
      "VK_ANDROID_external_memory_android_hardware_buffer");
  device_extensions.push_back("VK_KHR_sampler_ycbcr_conversion");
  device_extensions.push_back("VK_KHR_maintenance1");
  device_extensions.push_back("VK_KHR_bind_memory2");
  device_extensions.push_back("VK_KHR_get_memory_requirements2");
  device_extensions.push_back("VK_KHR_external_memory");
  device_extensions.push_back("VK_EXT_queue_family_foreign");
  device_extensions.push_back("VK_KHR_dedicated_allocation");

  float priorities[] = {
      1.0f,
  };
  const VkDeviceQueueCreateInfo queue_create_info{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .flags = 0,
      .queueFamilyIndex = queue_family_index,
      .queueCount = 1,
      .pQueuePriorities = priorities,
  };

  const VkPhysicalDeviceVulkan11Features physicalDeviceFeatures{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
      .samplerYcbcrConversion = VK_TRUE,
  };

  const VkDeviceCreateInfo device_create_info{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &physicalDeviceFeatures,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queue_create_info,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
      .ppEnabledExtensionNames = device_extensions.data(),
      .pEnabledFeatures = nullptr,
  };

  CALL_VK(vkCreateDevice(physical_device, &device_create_info,
                         /* pAllocator=*/nullptr, &logical_device));
  return logical_device;
}

VkSurfaceFormatKHR VulkanHandler::GetSurfaceFormat(
    VkSurfaceKHR surface, VkPhysicalDevice physical_device) {
  // Query the list of supported surface format and choose one we like.
  // More details:
  // https://developer.android.com/training/wide-color-gamut#vulkan
  VkSurfaceFormatKHR surface_format;

  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
                                       nullptr);
  VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[format_count];
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
                                       formats);

  uint32_t chosen_format;
  for (chosen_format = 0; chosen_format < format_count; chosen_format++) {
    if (formats[chosen_format].format == VK_FORMAT_R8G8B8A8_UNORM) break;
  }
  CHECK(chosen_format < format_count);
  surface_format = formats[chosen_format];
  delete[] formats;

  return surface_format;
}

VkSwapchainKHR VulkanHandler::CreateSwapchain(
    VkDevice logical_device, VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR surface_capabilities,
    VkSurfaceFormatKHR surface_format, uint32_t queue_family_index) {
  VkSwapchainKHR swapchain;
  memset(&swapchain, 0, sizeof(swapchain));

  // Create a swap chain (here we choose the minimum available number of surface
  // in the chain)
  const VkSwapchainCreateInfoKHR swapchain_create_info{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = surface_capabilities.minImageCount,
      .imageFormat = surface_format.format,
      .imageColorSpace = surface_format.colorSpace,
      .imageExtent = surface_capabilities.currentExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queue_family_index,
      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_FALSE,
      .oldSwapchain = VK_NULL_HANDLE,
  };

  CALL_VK(vkCreateSwapchainKHR(logical_device, &swapchain_create_info,
                               /* pAllocator=*/nullptr, &swapchain));

  return swapchain;
}

VkRenderPass VulkanHandler::CreateRenderPass(VkDevice logical_device) {
  VkRenderPass render_pass;
  VkAttachmentDescription color_attachment{
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  const VkAttachmentDescription depth_attachment{
      .format = FindDepthFormat(),
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkAttachmentReference depth_attachment_ref{
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  const VkAttachmentReference color_ref = {
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  const VkSubpassDependency dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
  };

  const VkSubpassDescription subpass_description{
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = nullptr,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_ref,
      .pResolveAttachments = nullptr,
      .pDepthStencilAttachment = &depth_attachment_ref,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr,
  };

  std::array<VkAttachmentDescription, 2> attachments = {color_attachment,
                                                        depth_attachment};

  const VkRenderPassCreateInfo render_pass_create_info{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass_description,
      .dependencyCount = 1,
      .pDependencies = &dependency,
  };
  CALL_VK(vkCreateRenderPass(logical_device, &render_pass_create_info,
                             /* pAllocator=*/nullptr, &render_pass));
  return render_pass;
}

VkCommandPool VulkanHandler::CreateCommandPool(VkDevice logical_device,
                                               uint32_t queue_family_index) {
  VkCommandPool command_pool;
  const VkCommandPoolCreateInfo cmd_pool_create_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queue_family_index,
  };
  CALL_VK(vkCreateCommandPool(logical_device, &cmd_pool_create_info,
                              /* pAllocator=*/nullptr, &command_pool));
  return command_pool;
}

void VulkanHandler::InitSwapchainImageRelatives(
    VkDevice logical_device, VkSwapchainKHR swapchain, VkRenderPass render_pass,
    VkSurfaceCapabilitiesKHR surface_capabilities,
    VkSurfaceFormatKHR surface_format, uint32_t swapchain_length) {
  std::vector<VkImage> swapchain_images;
  swapchain_images.resize(swapchain_length);
  CALL_VK(vkGetSwapchainImagesKHR(logical_device, swapchain, &swapchain_length,
                                  swapchain_images.data()));

  // create image views and frame buffers for each swapchain image
  swapchain_image_relatives_.resize(swapchain_length);
  for (uint32_t i = 0; i < swapchain_length; i++) {
    const VkImageViewCreateInfo view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = 0,
        .image = swapchain_images[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = surface_format.format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    CALL_VK(vkCreateImageView(logical_device, &view_create_info,
                              /* pAllocator=*/nullptr,
                              &swapchain_image_relatives_[i].swapchain_view));

    std::array<VkImageView, 2> attachments = {
        swapchain_image_relatives_[i].swapchain_view,
        depth_buffer_info_.texture_image_view};

    VkFramebufferCreateInfo fb_create_info{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width =
            static_cast<uint32_t>(surface_capabilities.currentExtent.width),
        .height =
            static_cast<uint32_t>(surface_capabilities.currentExtent.height),
        .layers = 1,
    };

    CALL_VK(vkCreateFramebuffer(logical_device, &fb_create_info,
                                /* pAllocator=*/nullptr,
                                &swapchain_image_relatives_[i].frame_buffer));
  }
}

void VulkanHandler::InitCommandBuffers(VkDevice logical_device,
                                       VkCommandPool command_pool,
                                       int max_frames_in_flight) {
  command_buffers_.resize(max_frames_in_flight);
  const VkCommandBufferAllocateInfo cmd_buffer_create_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = (uint32_t)max_frames_in_flight,
  };
  CALL_VK(vkAllocateCommandBuffers(logical_device, &cmd_buffer_create_info,
                                   command_buffers_.data()));
}

void VulkanHandler::InitSyncObjects(VkDevice logical_device,
                                    int max_frames_in_flight) {
  image_available_semaphores.resize(max_frames_in_flight);
  render_finished_semaphores.resize(max_frames_in_flight);
  fences_.resize(max_frames_in_flight);

  const VkFenceCreateInfo fence_create_info{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  const VkSemaphoreCreateInfo semaphore_create_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .flags = 0,
  };

  for (size_t i = 0; i < max_frames_in_flight; i++) {
    CALL_VK(vkCreateSemaphore(logical_device, &semaphore_create_info, nullptr,
                              &image_available_semaphores[i]));
    CALL_VK(vkCreateSemaphore(logical_device, &semaphore_create_info, nullptr,
                              &render_finished_semaphores[i]));
    CALL_VK(vkCreateFence(logical_device, &fence_create_info, nullptr,
                          &fences_[i]));
  }
}

void VulkanHandler::CreateDepthResources() {
  VkFormat depth_format_ = FindDepthFormat();
  CreateImage(
      surface_capabilities_.currentExtent.width,
      surface_capabilities_.currentExtent.height, depth_format_,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_buffer_info_.texture_image,
      depth_buffer_info_.texture_memory);
  depth_buffer_info_.texture_image_view =
      CreateImageView(depth_buffer_info_.texture_image, depth_format_,
                      VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkFormat VulkanHandler::FindSupportedFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physical_device_, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  return VK_FORMAT_UNDEFINED;
}

VkFormat VulkanHandler::FindDepthFormat() {
  return FindSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VulkanHandler::CreateImage(uint32_t width, uint32_t height,
                                VkFormat format, VkImageTiling tiling,
                                VkImageUsageFlags usage,
                                VkMemoryPropertyFlags properties,
                                VkImage& image, VkDeviceMemory& image_memory) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(logical_device_, &imageInfo, nullptr, &image) !=
      VK_SUCCESS) {
    LOGE("failed to create image!");
  }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(logical_device_, image, &mem_requirements);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex = FindMemoryType(
          physical_device_, mem_requirements.memoryTypeBits, properties)};

  if (vkAllocateMemory(logical_device_, &allocInfo, nullptr, &image_memory) !=
      VK_SUCCESS) {
    LOGE("failed to allocate image memory!");
  }

  vkBindImageMemory(logical_device_, image, image_memory, 0);
}

VkImageView VulkanHandler::CreateImageView(VkImage image, VkFormat format,
                                           VkImageAspectFlags aspect_flags) {
  VkImageViewCreateInfo viewInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange =
          {
              .aspectMask = aspect_flags,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

  VkImageView image_view;
  if (vkCreateImageView(logical_device_, &viewInfo, nullptr, &image_view) !=
      VK_SUCCESS) {
    LOGE("failed to create texture image view!");
  }

  return image_view;
}

VkCommandBuffer VulkanHandler::BeginSingleTimeCommands() {
  VkCommandBuffer command_buffer;
  VkCommandBufferAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  CALL_VK(
      vkAllocateCommandBuffers(logical_device_, &alloc_info, &command_buffer));
  VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  CALL_VK(vkBeginCommandBuffer(command_buffer, &begin_info));
  return command_buffer;
}

void VulkanHandler::EndSingleTimeCommands(VkCommandBuffer& command_buffer) {
  CALL_VK(vkEndCommandBuffer(command_buffer));
  VkSubmitInfo submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &command_buffer,
  };
  CALL_VK(vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE));
  CALL_VK(vkQueueWaitIdle(queue_));
  vkFreeCommandBuffers(logical_device_, command_pool_, 1, &command_buffer);
}

void VulkanHandler::CleanTextureImageInfo(
    util::TextureImageInfo& texture_image_info) {
  if (texture_image_info.texture_image != VK_NULL_HANDLE) {
    vkDestroyImageView(logical_device_, texture_image_info.texture_image_view,
                       /* vkDestroyImageView=*/nullptr);
    texture_image_info.texture_image_view = VK_NULL_HANDLE;
    vkFreeMemory(logical_device_, texture_image_info.texture_memory,
                 /* pAllocator=*/nullptr);
    texture_image_info.texture_memory = VK_NULL_HANDLE;
    vkDestroyImage(logical_device_, texture_image_info.texture_image,
                   /* pAllocator=*/nullptr);
    texture_image_info.texture_image = VK_NULL_HANDLE;
  }
}

void VulkanHandler::CleanBuffer(VkBuffer& buffer, VkDeviceMemory& buffer_memory,
                                void* mapped_data) {
  if (mapped_data != nullptr) {
    vkUnmapMemory(logical_device_, buffer_memory);
  }
  if (buffer_memory != VK_NULL_HANDLE) {
    vkFreeMemory(logical_device_, buffer_memory, /* pAllocator=*/nullptr);
    buffer_memory = VK_NULL_HANDLE;
  }
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(logical_device_, buffer, /* pAllocator=*/nullptr);
    buffer = VK_NULL_HANDLE;
  }
}

void VulkanHandler::CleanVerticesAndIndices(int frame_index) {
  if (vertex_buffers_[frame_index] != VK_NULL_HANDLE) {
    index_count_[frame_index] = 0;

    vkDestroyBuffer(logical_device_, index_buffers_[frame_index],
                    /* pAllocator=*/nullptr);
    index_buffers_[frame_index] = VK_NULL_HANDLE;
    vkFreeMemory(logical_device_, index_buffers_memory_[frame_index],
                 /* pAllocator=*/nullptr);
    index_buffers_memory_[frame_index] = VK_NULL_HANDLE;

    vkDestroyBuffer(logical_device_, vertex_buffers_[frame_index],
                    /* pAllocator=*/nullptr);
    vertex_buffers_[frame_index] = VK_NULL_HANDLE;
    vkFreeMemory(logical_device_, vertex_buffers_memory_[frame_index],
                 /* pAllocator=*/nullptr);
    vertex_buffers_memory_[frame_index] = VK_NULL_HANDLE;
  }
}

void VulkanHandler::CreateDepthSampler() {
  VkSamplerCreateInfo sampler_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
  };
  CALL_VK(vkCreateSampler(logical_device_, &sampler_info, nullptr,
                          &depth_sampler_));
}

void VulkanHandler::UpdateDepthTexture(ArSession* ar_session, ArFrame* ar_frame,
                                       int current_frame) {
  ArImage* ar_depth_image = nullptr;
  if (ArFrame_acquireDepthImage16Bits(ar_session, ar_frame, &ar_depth_image) !=
      AR_SUCCESS) {
    return;
  }

  int32_t width, height;
  ArImage_getWidth(ar_session, ar_depth_image, &width);
  ArImage_getHeight(ar_session, ar_depth_image, &height);
  const uint8_t* pixel_data = nullptr;
  int plane_data_size = 0;
  ArImage_getPlaneData(ar_session, ar_depth_image, 0, &pixel_data,
                       &plane_data_size);

  if (pixel_data == nullptr || plane_data_size <= 0) {
    ArImage_release(ar_depth_image);
    return;
  }

  // If dimensions change or the current frame's staging buffer is invalid,
  // recreate resources for the current frame.
  if (width != depth_texture_widths_[current_frame] ||
      height != depth_texture_heights_[current_frame]) {
    RecreateDepthResources(width, height, plane_data_size, current_frame);
  }

  // Copy the pixel data to the staging buffer.
  memcpy(mapped_depth_staging_buffers_[current_frame], pixel_data,
         static_cast<size_t>(plane_data_size));

  // Transition the image to the optimal layout for transfer.
  if (depth_texture_layouts_[current_frame] !=
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    TransitionImageLayout(depth_texture_infos_[current_frame].texture_image,
                          depth_texture_layouts_[current_frame],
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          /*synchronous=*/true, current_frame);
    depth_texture_layouts_[current_frame] =
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  }

  // Copy the staging buffer to the image.
  CopyBufferToImage(depth_staging_buffers_[current_frame],
                    depth_texture_infos_[current_frame].texture_image, width,
                    height, VK_IMAGE_ASPECT_COLOR_BIT,
                    /*synchronous=*/true, current_frame);

  // Transition the image to the optimal layout for shader access.
  TransitionImageLayout(depth_texture_infos_[current_frame].texture_image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        /*synchronous=*/true, current_frame);
  depth_texture_layouts_[current_frame] =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ArImage_release(ar_depth_image);
}

void VulkanHandler::RecreateDepthResources(int32_t width, int32_t height,
                                           int32_t plane_data_size,
                                           int current_frame) {
  // Clean up resources for the current frame before creating new ones.
  DestroyDepthResources(current_frame);

  // Create the new VkImage for the depth texture.
  VkImageCreateInfo image_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R16_UNORM,
      .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  CALL_VK(vkCreateImage(logical_device_, &image_info, nullptr,
                        &depth_texture_infos_[current_frame].texture_image));

  // Allocate and bind new memory for the image.
  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(
      logical_device_, depth_texture_infos_[current_frame].texture_image,
      &mem_requirements);
  VkMemoryAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex =
          FindMemoryType(physical_device_, mem_requirements.memoryTypeBits,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

  CALL_VK(
      vkAllocateMemory(logical_device_, &alloc_info, nullptr,
                       &depth_texture_infos_[current_frame].texture_memory));
  vkBindImageMemory(logical_device_,
                    depth_texture_infos_[current_frame].texture_image,
                    depth_texture_infos_[current_frame].texture_memory, 0);

  // Create the new image view for the resized image.
  VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = depth_texture_infos_[current_frame].texture_image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R16_UNORM,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .levelCount = 1,
              .layerCount = 1,
          },
  };
  CALL_VK(vkCreateImageView(
      logical_device_, &view_info, nullptr,
      &depth_texture_infos_[current_frame].texture_image_view));

  // Create the new persistent staging buffer with the correct size.
  CreateBuffer(physical_device_, plane_data_size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               depth_staging_buffers_[current_frame],
               depth_staging_buffer_memories_[current_frame]);

  // Persistently map the new staging buffer's memory.
  CALL_VK(vkMapMemory(
      logical_device_, depth_staging_buffer_memories_[current_frame], 0,
      plane_data_size, 0, &mapped_depth_staging_buffers_[current_frame]));

  // Set the image layout to undefined.
  depth_texture_layouts_[current_frame] = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_texture_widths_[current_frame] = width;
  depth_texture_heights_[current_frame] = height;
}

void VulkanHandler::DestroyDepthResources(int current_frame) {
  CleanTextureImageInfo(depth_texture_infos_[current_frame]);
  if (mapped_depth_staging_buffers_[current_frame] != nullptr) {
    vkUnmapMemory(logical_device_,
                  depth_staging_buffer_memories_[current_frame]);
    mapped_depth_staging_buffers_[current_frame] = nullptr;
  }
  if (depth_staging_buffers_[current_frame] != VK_NULL_HANDLE) {
    vkDestroyBuffer(logical_device_, depth_staging_buffers_[current_frame],
                    nullptr);
    depth_staging_buffers_[current_frame] = VK_NULL_HANDLE;
  }
  if (depth_staging_buffer_memories_[current_frame] != VK_NULL_HANDLE) {
    vkFreeMemory(logical_device_, depth_staging_buffer_memories_[current_frame],
                 nullptr);
    depth_staging_buffer_memories_[current_frame] = VK_NULL_HANDLE;
  }
  depth_texture_widths_[current_frame] = 0;
  depth_texture_heights_[current_frame] = 0;
}

}  // namespace hello_ar_vulkan
