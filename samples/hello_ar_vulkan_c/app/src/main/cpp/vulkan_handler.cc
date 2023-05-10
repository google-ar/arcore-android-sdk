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
#include <cstring>
#include <vector>

#include "../assets/shaders/background_frag.spv.h"
#include "../assets/shaders/background_vert.spv.h"
#include "android_vulkan_loader.h"
#include "util.h"

namespace simple_vulkan {
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

  render_pass_ = CreateRenderPass(logical_device_);
  command_pool_ = CreateCommandPool(logical_device_, queue_family_index_);
  descriptor_pool_ =
      CreateDescriptorPool(logical_device_, max_frames_in_flight_);
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_,
                                            &surface_capabilities_);
  surface_format_ = GetSurfaceFormat(surface_, physical_device_);
  swapchain_ = CreateSwapchain(logical_device_, surface_, surface_capabilities_,
                               surface_format_, queue_family_index_);
  CALL_VK(vkGetSwapchainImagesKHR(logical_device_, swapchain_,
                                  &swapchain_length_,
                                  /* pSwapchainImages=*/nullptr));

  InitSwapchainImageRelatives(logical_device_, swapchain_, render_pass_,
                              surface_capabilities_, surface_format_,
                              swapchain_length_);

  descriptor_sets_.resize(max_frames_in_flight_);
  texture_image_infos_.resize(max_frames_in_flight_);
  vertex_buffers_.resize(max_frames_in_flight_);
  vertex_buffers_memory_.resize(max_frames_in_flight_);
  index_buffers_.resize(max_frames_in_flight_);
  index_buffers_memory_.resize(max_frames_in_flight_);
  index_count_.resize(max_frames_in_flight_);

  InitCommandBuffers(logical_device_, command_pool_, max_frames_in_flight_);
  // Init semaphores and fences
  InitSyncObjects(logical_device_, max_frames_in_flight_);
}

VulkanHandler::~VulkanHandler() {
  CALL_VK(vkWaitForFences(logical_device_, fences_.size(), fences_.data(),
                          VK_TRUE, kFenceTimeoutNs));
  for (uint32_t i = 0; i < max_frames_in_flight_; i++) {
    CleanTextureImageInfo(i);
    CleanVertiesAndIndies(i);
  }

  vkDestroySamplerYcbcrConversion(logical_device_, conversion_,
                                  /* pAllocator=*/nullptr);
  vkDestroySampler(logical_device_, sampler_, /* pAllocator=*/nullptr);
  vkDestroyPipelineLayout(logical_device_, pipeline_layout_,
                          /* pAllocator=*/nullptr);
  vkDestroyDescriptorSetLayout(logical_device_, descriptor_set_layout_,
                               /* pAllocator=*/nullptr);

  vkDestroyDescriptorPool(logical_device_, descriptor_pool_,
                          /* pAllocator=*/nullptr);

  vkDestroyPipeline(logical_device_, graphics_pipeline_,
                    /* pAllocator=*/nullptr);

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

void VulkanHandler::RenderFromHardwareBuffer(int current_frame,
                                             AHardwareBuffer* hardware_buffer) {
  CleanTextureImageInfo(current_frame);

  AHardwareBuffer_Desc buffer_desc = {};

  if (__builtin_available(android 26, *)) {
    AHardwareBuffer_describe(hardware_buffer, &buffer_desc);
  } else {
    LOGE("Android API 26+ Required.");
    exit(0);
  }

  // Contains how to sample the image
  VkAndroidHardwareBufferFormatPropertiesANDROID format_info = {
      .sType =
          VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID,
  };
  // Contains only memory information (size and type)
  VkAndroidHardwareBufferPropertiesANDROID properties = {
      .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID,
      .pNext = &format_info,
  };
  CALL_VK(vkGetAndroidHardwareBufferPropertiesANDROID(
      logical_device_, hardware_buffer, &properties));

  // Create an image to bind to our AHardwareBuffer
  const VkExternalFormatANDROID external_format = {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID,
      .externalFormat = format_info.externalFormat,
  };
  const VkExternalMemoryImageCreateInfo external_create_info = {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
      .pNext = &external_format,
      .handleTypes =
          VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
  };
  const VkImageCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = &external_create_info,
      .flags = 0u,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format_info.format,
      .extent =
          {
              buffer_desc.width,
              buffer_desc.height,
              1u,
          },
      .mipLevels = 1u,
      .arrayLayers = 1u,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  CALL_VK(vkCreateImage(logical_device_, &create_info, nullptr,
                        &texture_image_infos_[current_frame].texture_image));

  // Allocate the device memory for the image
  const VkImportAndroidHardwareBufferInfoANDROID android_hardware_buffer_info =
      {
          .sType =
              VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,
          .buffer = hardware_buffer,
      };
  const VkMemoryDedicatedAllocateInfo memory_dedicated_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
      .pNext = &android_hardware_buffer_info,
      .image = texture_image_infos_[current_frame].texture_image,
      .buffer = VK_NULL_HANDLE,
  };
  const VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memory_dedicated_allocate_info,
      .allocationSize = properties.allocationSize,
      .memoryTypeIndex =
          FindMemoryType(physical_device_, properties.memoryTypeBits, 0),
  };
  CALL_VK(
      vkAllocateMemory(logical_device_, &memory_allocate_info, nullptr,
                       &texture_image_infos_[current_frame].texture_memory));

  // Bind the allocated memory to the image
  CALL_VK(vkBindImageMemory(
      logical_device_, texture_image_infos_[current_frame].texture_image,
      texture_image_infos_[current_frame].texture_memory, 0));

  // Create YUV conversion, this is needed to sample a texture image with
  // external format
  if (conversion_ == VK_NULL_HANDLE) {
    const VkSamplerYcbcrConversionCreateInfo conversion_create_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO,
        .pNext = &external_format,
        .format = format_info.format,
        .ycbcrModel = format_info.suggestedYcbcrModel,
        .ycbcrRange = format_info.suggestedYcbcrRange,
        .components = format_info.samplerYcbcrConversionComponents,
        .xChromaOffset = format_info.suggestedXChromaOffset,
        .yChromaOffset = format_info.suggestedYChromaOffset,
        .chromaFilter = VK_FILTER_NEAREST,
        .forceExplicitReconstruction = VK_FALSE,
    };

    CALL_VK(vkCreateSamplerYcbcrConversion(
        logical_device_, &conversion_create_info, nullptr, &conversion_));
  }

  VkSamplerYcbcrConversionInfo sampler_conversion_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO,
      .conversion = conversion_,
  };

  if (sampler_ == VK_NULL_HANDLE) {
    sampler_ = CreateSampler(logical_device_, sampler_conversion_info);
  }

  if (descriptor_set_layout_ == VK_NULL_HANDLE) {
    descriptor_set_layout_ =
        CreateDescriptorSetLayout(logical_device_, sampler_);
    InitDescriptorSets(logical_device_, descriptor_pool_,
                       descriptor_set_layout_, max_frames_in_flight_);
  }

  if (pipeline_layout_ == VK_NULL_HANDLE) {
    pipeline_layout_ =
        CreatePipelineLayout(logical_device_, descriptor_set_layout_);
    graphics_pipeline_ =
        CreateGraphicsPipeline(logical_device_, render_pass_, pipeline_layout_);
  }

  // Update image and view
  const VkImageViewCreateInfo view_create_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = &sampler_conversion_info,
      .flags = 0,
      .image = texture_image_infos_[current_frame].texture_image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_UNDEFINED,
      .components =
          {
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
          },
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  CALL_VK(vkCreateImageView(
      logical_device_, &view_create_info,
      /* pAllocator=*/nullptr,
      &texture_image_infos_[current_frame].texture_image_view));

  // Transfer Image Layout
  TransitionImageLayout(texture_image_infos_[current_frame].texture_image,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  // Update Descriptor Sets
  VkDescriptorImageInfo image_info{
      .sampler = sampler_,
      .imageView = texture_image_infos_[current_frame].texture_image_view,
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
  };

  VkWriteDescriptorSet descriptor_writes[1];

  descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_writes[0].dstSet = descriptor_sets_[current_frame];
  descriptor_writes[0].dstBinding = 0;
  descriptor_writes[0].dstArrayElement = 0;
  descriptor_writes[0].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptor_writes[0].descriptorCount = 1;
  descriptor_writes[0].pImageInfo = &image_info;

  vkUpdateDescriptorSets(logical_device_, 1, descriptor_writes, 0, nullptr);

  // Update Viewport and scissor
  VkViewport viewport = {
      .x = 0,
      .y = 0,
      .width = static_cast<float>(surface_capabilities_.currentExtent.width),
      .height = static_cast<float>(surface_capabilities_.currentExtent.height),
      .minDepth = 0.0,
      .maxDepth = 1.0};

  VkRect2D scissor = {
      .extent = {.width = static_cast<uint32_t>(
                     surface_capabilities_.currentExtent.width),
                 .height = static_cast<uint32_t>(
                     surface_capabilities_.currentExtent.height)},
  };

  scissor.offset = {.x = 0, .y = 0};

  // Bind to the command buffer.
  vkCmdBindPipeline(command_buffers_[current_frame],
                    VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);
  vkCmdSetViewport(command_buffers_[current_frame], 0, 1, &viewport);
  vkCmdSetScissor(command_buffers_[current_frame], 0, 1, &scissor);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(command_buffers_[current_frame], 0, 1,
                         &vertex_buffers_[current_frame], &offset);

  vkCmdBindIndexBuffer(command_buffers_[current_frame],
                       index_buffers_[current_frame], 0, VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(command_buffers_[current_frame],
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0,
                          1, &descriptor_sets_[current_frame], 0, nullptr);
  vkCmdDrawIndexed(command_buffers_[current_frame], index_count_[current_frame],
                   1, 0, 0, 0);
}

bool VulkanHandler::IsVerticesSetForFrame(int current_frame) {
  return vertex_buffers_[current_frame] != VK_NULL_HANDLE;
}

void VulkanHandler::SetVerticesAndIndicesForFrame(
    int current_frame, std::vector<VertexInfo> vertices,
    std::vector<uint16_t> indices) {
  CleanVertiesAndIndies(current_frame);

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
  const VkClearValue clear_vals{.color = {.float32 = {0.0f, 1.0f, 0.0f, 1.0f}}};
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
      .clearValueCount = 1,
      .pClearValues = &clear_vals};
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
      .pApplicationName = "simple_vulkan",
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
  const VkAttachmentDescription attachment_descriptions{
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  const VkAttachmentReference colour_reference = {
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  const VkSubpassDescription subpass_description{
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = nullptr,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colour_reference,
      .pResolveAttachments = nullptr,
      .pDepthStencilAttachment = nullptr,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr,
  };
  const VkRenderPassCreateInfo render_pass_create_info{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &attachment_descriptions,
      .subpassCount = 1,
      .pSubpasses = &subpass_description,
      .dependencyCount = 0,
      .pDependencies = nullptr,
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

VkDescriptorPool VulkanHandler::CreateDescriptorPool(VkDevice logical_device,
                                                     int max_frames_in_flight) {
  VkDescriptorPool descriptor_pool;
  VkDescriptorPoolSize pool_size{
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = static_cast<uint32_t>(max_frames_in_flight),
  };

  VkDescriptorPoolCreateInfo pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = 1,
      .pPoolSizes = &pool_size,
      .maxSets = static_cast<uint32_t>(max_frames_in_flight),
  };

  CALL_VK(vkCreateDescriptorPool(logical_device, &pool_info, nullptr,
                                 &descriptor_pool));
  return descriptor_pool;
}

VkSampler VulkanHandler::CreateSampler(
    VkDevice logical_device,
    VkSamplerYcbcrConversionInfo sampler_conversion_info) {
  VkSampler sampler;
  const VkSamplerCreateInfo sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = &sampler_conversion_info,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 1,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_NEVER,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
      .unnormalizedCoordinates = VK_FALSE,
  };
  CALL_VK(
      vkCreateSampler(logical_device, &sampler_create_info, nullptr, &sampler));
  return sampler;
}

VkDescriptorSetLayout VulkanHandler::CreateDescriptorSetLayout(
    VkDevice logical_device, VkSampler sampler) {
  VkDescriptorSetLayout descriptor_set_layout;

  VkDescriptorSetLayoutBinding bindings[1];

  VkDescriptorSetLayoutBinding sampler_layout_binding{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = &sampler,
  };
  bindings[0] = sampler_layout_binding;

  VkDescriptorSetLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = bindings,
  };
  CALL_VK(vkCreateDescriptorSetLayout(logical_device, &layout_info, nullptr,
                                      &descriptor_set_layout));

  return descriptor_set_layout;
}

VkPipelineLayout VulkanHandler::CreatePipelineLayout(
    VkDevice logical_device, VkDescriptorSetLayout descriptor_set_layout) {
  VkPipelineLayout pipeline_layout;
  VkPipelineLayoutCreateInfo pipeline_layout_create_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
  };
  CALL_VK(vkCreatePipelineLayout(logical_device, &pipeline_layout_create_info,
                                 nullptr, &pipeline_layout));

  return pipeline_layout;
}

VkPipeline VulkanHandler::CreateGraphicsPipeline(
    VkDevice logical_device, VkRenderPass render_pass,
    VkPipelineLayout pipeline_layout) {
  VkPipeline graphics_pipeline;

  VkShaderModule vertex_shader =
      LoadShader(logical_device, background_vert, sizeof(background_vert));
  VkShaderModule fragment_shader =
      LoadShader(logical_device, background_frag, sizeof(background_frag));

  // Specify vertex and fragment shader stages
  VkPipelineShaderStageCreateInfo vertex_shader_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .flags = 0,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertex_shader,
      .pName = "main",
      .pSpecializationInfo = nullptr,
  };
  VkPipelineShaderStageCreateInfo fragment_shader_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .flags = 0,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragment_shader,
      .pName = "main",
      .pSpecializationInfo = nullptr,
  };

  // Specify viewport info
  VkPipelineViewportStateCreateInfo viewport_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr,
  };

  // Specify multisample info
  VkSampleMask sample_mask = ~0u;
  VkPipelineMultisampleStateCreateInfo multisample_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0,
      .pSampleMask = &sample_mask,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE,
  };

  // Specify color blend state
  VkPipelineColorBlendAttachmentState attachment_states = {
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo color_blend_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &attachment_states,
  };

  // Specify rasterizer info
  VkPipelineRasterizationStateCreateInfo raster_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1,
  };

  // Specify input assembler state
  VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
      .primitiveRestartEnable = VK_FALSE,
  };

  // Specify vertex input state
  VkVertexInputBindingDescription vertex_input_bindings = {
      .binding = 0,
      .stride = 4 * sizeof(float),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  VkVertexInputAttributeDescription vertex_input_attributes[2] = {
      {
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = 0,
      },
      {
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = sizeof(float) * 2,
      }};

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertex_input_bindings,
      .vertexAttributeDescriptionCount = 2,
      .pVertexAttributeDescriptions = vertex_input_attributes,
  };

  VkDynamicState dynamic_state_enables[2] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_state_enables};

  VkPipelineDepthStencilStateCreateInfo depth_stencil = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE};

  // Create the pipeline
  VkPipelineShaderStageCreateInfo shader_stages[2] = {vertex_shader_state,
                                                      fragment_shader_state};
  VkGraphicsPipelineCreateInfo pipeline_create_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .flags = 0,
      .stageCount = 2,
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly_info,
      .pTessellationState = nullptr,
      .pViewportState = &viewport_info,
      .pRasterizationState = &raster_info,
      .pMultisampleState = &multisample_info,
      .pDepthStencilState = &depth_stencil,
      .pColorBlendState = &color_blend_info,
      .pDynamicState = &dynamic_state_info,
      .layout = pipeline_layout,
      .renderPass = render_pass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
  };
  CALL_VK(vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1,
                                    &pipeline_create_info, nullptr,
                                    &graphics_pipeline));

  vkDestroyShaderModule(logical_device, vertex_shader, nullptr);
  vkDestroyShaderModule(logical_device, fragment_shader, nullptr);

  return graphics_pipeline;
}

void VulkanHandler::InitDescriptorSets(
    VkDevice logical_device, VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout, int max_frames_in_flight) {
  descriptor_sets_.resize(max_frames_in_flight);
  std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight,
                                             descriptor_set_layout);
  VkDescriptorSetAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptor_pool,
      .descriptorSetCount = static_cast<uint32_t>(max_frames_in_flight),
      .pSetLayouts = layouts.data(),
  };

  CALL_VK(vkAllocateDescriptorSets(logical_device, &alloc_info,
                                   descriptor_sets_.data()));
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

    VkImageView attachments[2] = {
        swapchain_image_relatives_[i].swapchain_view,
    };
    VkFramebufferCreateInfo fb_create_info{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,  // 2 if using depth
        .pAttachments = attachments,
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

void VulkanHandler::CleanTextureImageInfo(int index) {
  if (texture_image_infos_[index].texture_image != VK_NULL_HANDLE) {
    vkDestroyImageView(logical_device_,
                       texture_image_infos_[index].texture_image_view,
                       /* vkDestroyImageView=*/nullptr);
    texture_image_infos_[index].texture_image_view = VK_NULL_HANDLE;
    vkFreeMemory(logical_device_, texture_image_infos_[index].texture_memory,
                 /* pAllocator=*/nullptr);
    texture_image_infos_[index].texture_memory = VK_NULL_HANDLE;
    vkDestroyImage(logical_device_, texture_image_infos_[index].texture_image,
                   /* pAllocator=*/nullptr);
    texture_image_infos_[index].texture_image = VK_NULL_HANDLE;
  }
}

void VulkanHandler::CleanVertiesAndIndies(int frame_index) {
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

  LOGE("failed to find suitable memory type!");
  exit(0);
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
                                          VkImageLayout new_layout) {
  VkCommandBufferAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(logical_device_, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkImageMemoryBarrier barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
  };

  VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags destination_stage =
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &command_buffer,
  };

  vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue_);

  vkFreeCommandBuffers(logical_device_, command_pool_, 1, &command_buffer);
}

}  // namespace simple_vulkan
