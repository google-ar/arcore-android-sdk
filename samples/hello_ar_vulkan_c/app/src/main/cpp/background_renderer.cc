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

#include "background_renderer.h"

#include <android/log.h>
#include <vulkan/vulkan.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "../assets/shaders/background_frag.spv.h"
#include "../assets/shaders/background_vert.spv.h"
#include "android_vulkan_loader.h"
#include "util.h"
#include "vulkan_handler.h"

namespace hello_ar_vulkan {

BackgroundRenderer::BackgroundRenderer(VulkanHandler* vulkan_handler)
    : vulkan_handler_(vulkan_handler) {
  CHECK(vulkan_handler_ != nullptr);
  texture_image_infos_.resize(vulkan_handler_->GetMaxFramesInFlight());
}

BackgroundRenderer::~BackgroundRenderer() {
  // Prevent double-freeing resources if Init() was not called.
  if (!is_initialized_) {
    return;
  }
  for (uint32_t i = 0; i < texture_image_infos_.size(); ++i) {
    vulkan_handler_->CleanTextureImageInfo(texture_image_infos_[i]);
  }
  vkDestroyPipeline(vulkan_handler_->GetLogicalDevice(), pipeline_,
                    /* pAllocator=*/nullptr);
  vkDestroyPipelineLayout(vulkan_handler_->GetLogicalDevice(), pipeline_layout_,
                          /* pAllocator=*/nullptr);
  vkDestroyDescriptorSetLayout(vulkan_handler_->GetLogicalDevice(),
                               descriptor_set_layout_,
                               /* pAllocator=*/nullptr);
  vkDestroyDescriptorPool(vulkan_handler_->GetLogicalDevice(), descriptor_pool_,
                          /* pAllocator=*/nullptr);
  // Implicitly destroy descriptor sets when pool is destroyed
  vkDestroySampler(vulkan_handler_->GetLogicalDevice(), sampler_,
                   /* pAllocator=*/nullptr);
  vkDestroySamplerYcbcrConversion(vulkan_handler_->GetLogicalDevice(),
                                  conversion_,
                                  /* pAllocator=*/nullptr);
}

void BackgroundRenderer::DrawBackground(int current_frame,
                                        AHardwareBuffer* hardware_buffer) {
  CHECK(current_frame >= 0 && current_frame < texture_image_infos_.size());
  vulkan_handler_->CleanTextureImageInfo(texture_image_infos_[current_frame]);

  AHardwareBuffer_Desc buffer_desc = {};

  if (__builtin_available(android 27, *)) {
    AHardwareBuffer_describe(hardware_buffer, &buffer_desc);
  } else {
    LOGE("Android API 27+ Required.");
    CHECK(false);
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
      vulkan_handler_->GetLogicalDevice(), hardware_buffer, &properties));

  // Initialize the all resources when we have the format information.
  Init(format_info);

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
  CALL_VK(vkCreateImage(vulkan_handler_->GetLogicalDevice(), &create_info,
                        nullptr,
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
      .memoryTypeIndex = vulkan_handler_->FindMemoryType(
          vulkan_handler_->GetPhysicalDevice(), properties.memoryTypeBits, 0),
  };
  CALL_VK(vkAllocateMemory(
      vulkan_handler_->GetLogicalDevice(), &memory_allocate_info, nullptr,
      &texture_image_infos_[current_frame].texture_memory));

  // Bind the allocated memory to the image
  CALL_VK(vkBindImageMemory(vulkan_handler_->GetLogicalDevice(),
                            texture_image_infos_[current_frame].texture_image,
                            texture_image_infos_[current_frame].texture_memory,
                            0));

  VkSamplerYcbcrConversionInfo sampler_conversion_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO,
      .conversion = conversion_,
  };

  // Update image and view
  const VkImageViewCreateInfo view_create_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = &sampler_conversion_info,
      .flags = 0,
      .image = texture_image_infos_[current_frame].texture_image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_UNDEFINED,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  CALL_VK(vkCreateImageView(
      vulkan_handler_->GetLogicalDevice(), &view_create_info,
      /* pAllocator=*/nullptr,
      &texture_image_infos_[current_frame].texture_image_view));

  // Transfer Image Layout to shader read only optimal since the image is being
  // copied to the shader.
  vulkan_handler_->TransitionImageLayout(
      texture_image_infos_[current_frame].texture_image,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      /*synchronous=*/true, current_frame);

  // Update Descriptor Sets
  VkDescriptorImageInfo image_info{
      .sampler = sampler_,
      .imageView = texture_image_infos_[current_frame].texture_image_view,
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
  };

  VkWriteDescriptorSet descriptor_writes[1];
  descriptor_writes[0] = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptor_sets_[current_frame],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info,
  };

  vkUpdateDescriptorSets(vulkan_handler_->GetLogicalDevice(), 1,
                         descriptor_writes, 0, nullptr);

  // Update Viewport and scissor
  const VkSurfaceCapabilitiesKHR& surface_capabilities =
      vulkan_handler_->GetSurfaceCapabilities();

  VkViewport viewport = {
      .x = 0,
      .y = 0,
      .width = static_cast<float>(surface_capabilities.currentExtent.width),
      .height = static_cast<float>(surface_capabilities.currentExtent.height),
      .minDepth = 0.0,
      .maxDepth = 1.0};

  VkRect2D scissor = {
      .extent = {.width = static_cast<uint32_t>(
                     surface_capabilities.currentExtent.width),
                 .height = static_cast<uint32_t>(
                     surface_capabilities.currentExtent.height)},
  };

  scissor.offset = {.x = 0, .y = 0};

  VkCommandBuffer command_buffer =
      vulkan_handler_->GetCommandBuffer(current_frame);

  // Bind to the command buffer.
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(command_buffer, 0, 1,
                         vulkan_handler_->GetVertexBuffer(current_frame),
                         &offset);

  vkCmdBindIndexBuffer(command_buffer,
                       vulkan_handler_->GetIndexBuffer(current_frame), 0,
                       VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout_, 0, 1,
                          &descriptor_sets_[current_frame], 0, nullptr);
  vkCmdDrawIndexed(command_buffer,
                   vulkan_handler_->GetIndexCount(current_frame), 1, 0, 0, 0);
}

void BackgroundRenderer::Init(
    VkAndroidHardwareBufferFormatPropertiesANDROID format_info) {
  //
  if (is_initialized_) {
    return;
  }

  VkExternalFormatANDROID external_format_for_conversion = {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID,
      .pNext = nullptr,
      .externalFormat = format_info.externalFormat,
  };

  const VkSamplerYcbcrConversionCreateInfo conversion_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO,
      .pNext = &external_format_for_conversion,
      .format = VK_FORMAT_UNDEFINED,
      .ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709,
      .ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_FULL,
      .components = {/* ... */},
      .xChromaOffset = VK_CHROMA_LOCATION_COSITED_EVEN,
      .yChromaOffset = VK_CHROMA_LOCATION_COSITED_EVEN,
      .chromaFilter = VK_FILTER_LINEAR,
      .forceExplicitReconstruction = VK_FALSE,
  };

  CALL_VK(vkCreateSamplerYcbcrConversion(vulkan_handler_->GetLogicalDevice(),
                                         &conversion_create_info, nullptr,
                                         &conversion_));

  VkSamplerYcbcrConversionInfo sampler_conversion_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO,
      .conversion = conversion_,
  };

  sampler_ = CreateSampler(sampler_conversion_info);
  descriptor_set_layout_ = CreateDescriptorSetLayout();
  InitDescriptorSets();
  CreateBackgroundPipelineLayout();
  CreateBackgroundPipeline();
  is_initialized_ = true;
}

VkShaderModule BackgroundRenderer::LoadShader(const uint32_t* const content,
                                              size_t size) const {
  VkShaderModule shader;
  VkShaderModuleCreateInfo shader_module_create_info{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .flags = 0,
      .codeSize = size,
      .pCode = content,
  };
  CALL_VK(vkCreateShaderModule(vulkan_handler_->GetLogicalDevice(),
                               &shader_module_create_info, nullptr, &shader));

  return shader;
}

VkDescriptorPool BackgroundRenderer::CreateDescriptorPool() {
  VkDescriptorPool descriptor_pool;
  // For some device, such as Samsung S22 5G, each descriptor set might use
  // more than 1 slot, so we need to increase the pool size to keep it safe.
  uint32_t safe_pool_size = vulkan_handler_->GetMaxFramesInFlight() * 3;

  VkDescriptorPoolSize pool_size{
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = safe_pool_size,
  };

  VkDescriptorPoolCreateInfo pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = safe_pool_size,
      .poolSizeCount = 1,
      .pPoolSizes = &pool_size,
  };

  CALL_VK(vkCreateDescriptorPool(vulkan_handler_->GetLogicalDevice(),
                                 &pool_info, nullptr, &descriptor_pool));
  return descriptor_pool;
}

VkSampler BackgroundRenderer::CreateSampler(
    VkSamplerYcbcrConversionInfo sampler_conversion_info) {
  VkSampler sampler;
  const VkSamplerCreateInfo sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = &sampler_conversion_info,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
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
  CALL_VK(vkCreateSampler(vulkan_handler_->GetLogicalDevice(),
                          &sampler_create_info, nullptr, &sampler));
  return sampler;
}

VkDescriptorSetLayout BackgroundRenderer::CreateDescriptorSetLayout() {
  VkDescriptorSetLayout descriptor_set_layout;

  VkDescriptorSetLayoutBinding bindings[1];

  VkDescriptorSetLayoutBinding sampler_layout_binding{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = &sampler_,
  };
  bindings[0] = sampler_layout_binding;

  VkDescriptorSetLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = bindings,
  };
  CALL_VK(vkCreateDescriptorSetLayout(vulkan_handler_->GetLogicalDevice(),
                                      &layout_info, nullptr,
                                      &descriptor_set_layout));

  return descriptor_set_layout;
}

void BackgroundRenderer::CreateBackgroundPipelineLayout() {
  VkPipelineLayoutCreateInfo pipeline_layout_create_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout_,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
  };
  CALL_VK(vkCreatePipelineLayout(vulkan_handler_->GetLogicalDevice(),
                                 &pipeline_layout_create_info, nullptr,
                                 &pipeline_layout_));
}

void BackgroundRenderer::CreateBackgroundPipeline() {
  VkShaderModule vertex_shader =
      LoadShader(background_vert, sizeof(background_vert));
  VkShaderModule fragment_shader =
      LoadShader(background_frag, sizeof(background_frag));

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
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
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
      .layout = pipeline_layout_,
      .renderPass = vulkan_handler_->GetRenderPass(),
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
  };
  CALL_VK(vkCreateGraphicsPipelines(vulkan_handler_->GetLogicalDevice(),
                                    VK_NULL_HANDLE, 1, &pipeline_create_info,
                                    nullptr, &pipeline_));

  vkDestroyShaderModule(vulkan_handler_->GetLogicalDevice(), vertex_shader,
                        nullptr);
  vkDestroyShaderModule(vulkan_handler_->GetLogicalDevice(), fragment_shader,
                        nullptr);
}

void BackgroundRenderer::InitDescriptorSets() {
  const int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  descriptor_sets_.resize(max_frames_in_flight);
  std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight,
                                             descriptor_set_layout_);
  descriptor_pool_ = CreateDescriptorPool();
  VkDescriptorSetAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = static_cast<uint32_t>(max_frames_in_flight),
      .pSetLayouts = layouts.data(),
  };

  CALL_VK(vkAllocateDescriptorSets(vulkan_handler_->GetLogicalDevice(),
                                   &alloc_info, descriptor_sets_.data()));
}

}  // namespace hello_ar_vulkan
