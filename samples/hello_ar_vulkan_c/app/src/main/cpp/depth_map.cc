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

#include "depth_map.h"

#include <android/asset_manager.h>
#include <jni.h>

#include <cstdlib>
#include <cstring>

#include "../assets/shaders/depth_map_frag.spv.h"
#include "../assets/shaders/depth_map_vert.spv.h"
#include "arcore_c_api.h"
#include "android_vulkan_loader.h"
#include "jni_interface.h"
#include "util.h"
#include "vulkan_handler.h"

namespace hello_ar_vulkan {

DepthMapRenderer::DepthMapRenderer(VulkanHandler* vulkan_handler)
    : vulkan_handler_(vulkan_handler) {
  CHECK(vulkan_handler_ != nullptr);

  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  depth_map_vertex_shader_ = vulkan_handler_->LoadShader(
      logical_device, depth_map_vert, sizeof(depth_map_vert));
  depth_map_fragment_shader_ = vulkan_handler_->LoadShader(
      logical_device, depth_map_frag, sizeof(depth_map_frag));

  CreatePaletteTexture();
  CreateDescriptorSetLayout();
  CreateDescriptorPool();
  CreateDescriptorSets();
  CreatePipeline();
}

DepthMapRenderer::~DepthMapRenderer() {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();

  // Clean up pipeline resources
  vkDestroyPipeline(logical_device, pipeline_, nullptr);
  vkDestroyPipelineLayout(logical_device, pipeline_layout_, nullptr);

  // Clean up descriptor resources
  vkDestroyDescriptorPool(logical_device, descriptor_pool_, nullptr);
  vkDestroyDescriptorSetLayout(logical_device, descriptor_set_layout_, nullptr);

  // Clean up textures and samplers
  vkDestroySampler(logical_device, palette_sampler_, nullptr);
  vulkan_handler_->CleanTextureImageInfo(palette_texture_info_);
  vkDestroyShaderModule(logical_device, depth_map_vertex_shader_, nullptr);
  vkDestroyShaderModule(logical_device, depth_map_fragment_shader_, nullptr);
}

void DepthMapRenderer::DrawDepthMap(int current_frame) {
  // Refresh the depth texture.
  UpdateDescriptorSets(current_frame);

  VkCommandBuffer command_buffer =
      vulkan_handler_->GetCommandBuffer(current_frame);

  // Bind to the command buffer.
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  vkCmdSetViewport(command_buffer, 0, 1, &viewport_);
  vkCmdSetScissor(command_buffer, 0, 1, &scissor_);

  VkDeviceSize offset = 0;

  // We use the same vertex and index buffer as the background renderer.
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

void DepthMapRenderer::CreatePaletteTexture() {
  // --- 1. Call JNI to get pixel data ---
  uint32_t width = 0;
  uint32_t height = 0;
  VkDeviceSize image_size = 0;
  void* pixel_data = nullptr;
  CHECK(util::LoadPngFromAssetManager("models/depth_color_palette.png", width,
                                      height, image_size, pixel_data));

  // --- 2. Use Vulkan to create a staging buffer and upload data ---
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  vulkan_handler_->CreateBuffer(vulkan_handler_->GetPhysicalDevice(),
                                image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                staging_buffer, staging_buffer_memory);

  // Copy pixel data from Java into the staging buffer
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  void* mapped_data;
  vkMapMemory(logical_device, staging_buffer_memory, 0, image_size, 0,
              &mapped_data);
  memcpy(mapped_data, pixel_data, static_cast<size_t>(image_size));
  vkUnmapMemory(logical_device, staging_buffer_memory);
  free(pixel_data);

  // --- 3. Create the final VkImage and copy the staging buffer into it ---

  const VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  CALL_VK(vkCreateImage(logical_device, &image_create_info, nullptr,
                        &palette_texture_info_.texture_image));

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(
      logical_device, palette_texture_info_.texture_image, &mem_requirements);
  VkMemoryAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex = vulkan_handler_->FindMemoryType(
          vulkan_handler_->GetPhysicalDevice(), mem_requirements.memoryTypeBits,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
  CALL_VK(vkAllocateMemory(logical_device, &alloc_info, nullptr,
                           &palette_texture_info_.texture_memory));
  CALL_VK(vkBindImageMemory(logical_device, palette_texture_info_.texture_image,
                            palette_texture_info_.texture_memory, 0));

  // --- 4. Transition the image layout to TransferDstOptimal ---
  vulkan_handler_->TransitionImageLayout(palette_texture_info_.texture_image,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  // --- 5. Copy the staging buffer into the image ---
  vulkan_handler_->CopyBufferToImage(
      staging_buffer, palette_texture_info_.texture_image, width, height);

  // --- 6. Transition the image layout to ShaderReadOnlyOptimal ---
  vulkan_handler_->TransitionImageLayout(
      palette_texture_info_.texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // Clean up the staging buffer
  vkDestroyBuffer(logical_device, staging_buffer, nullptr);
  vkFreeMemory(logical_device, staging_buffer_memory, nullptr);

  // --- 7. Create the texture image view ---
  const VkImageViewCreateInfo color_texture_image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .flags = 0,
      .image = palette_texture_info_.texture_image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  CALL_VK(vkCreateImageView(logical_device,
                            &color_texture_image_view_create_info, nullptr,
                            &palette_texture_info_.texture_image_view));

  // ---8. Create the sampler ---
  const VkSamplerCreateInfo sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 1.f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
      .unnormalizedCoordinates = VK_FALSE,
  };
  CALL_VK(vkCreateSampler(logical_device, &sampler_create_info, nullptr,
                          &palette_sampler_));
}

void DepthMapRenderer::CreateDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding bindings[2];

  bindings[0] = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = nullptr,
  };
  bindings[1] = {
      .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = nullptr,
  };

  VkDescriptorSetLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 2,
      .pBindings = bindings,
  };
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  CALL_VK(vkCreateDescriptorSetLayout(logical_device, &layout_info, nullptr,
                                      &descriptor_set_layout_));
}

void DepthMapRenderer::CreateDescriptorPool() {
  int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  // Descriptor pool size is twice the number of frames in flight because we
  // have two samplers (one for depth and one for palette).
  VkDescriptorPoolSize pool_size{
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = static_cast<uint32_t>(max_frames_in_flight * 2),
  };
  VkDescriptorPoolCreateInfo descriptor_pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = static_cast<uint32_t>(max_frames_in_flight),
      .poolSizeCount = 1,
      .pPoolSizes = &pool_size,
  };
  CALL_VK(vkCreateDescriptorPool(vulkan_handler_->GetLogicalDevice(),
                                 &descriptor_pool_info, nullptr,
                                 &descriptor_pool_));
}

void DepthMapRenderer::CreateDescriptorSets() {
  int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  descriptor_sets_.resize(max_frames_in_flight);

  // A vector which contains the same descriptor set layout for each frame.
  std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight,
                                             descriptor_set_layout_);
  VkDescriptorSetAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = static_cast<uint32_t>(max_frames_in_flight),
      .pSetLayouts = layouts.data(),
  };
  CALL_VK(vkAllocateDescriptorSets(logical_device, &alloc_info,
                                   descriptor_sets_.data()));

  // Update the descriptor sets with the palette texture sampler
  VkDescriptorImageInfo palette_image_info = {
      .sampler = palette_sampler_,
      .imageView = palette_texture_info_.texture_image_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  VkWriteDescriptorSet descriptor_writes[1];
  descriptor_writes[0] = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptor_sets_[0],
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &palette_image_info,
  };

  for (int i = 0; i < max_frames_in_flight; ++i) {
    descriptor_writes[0].dstSet = descriptor_sets_[i];
    vkUpdateDescriptorSets(logical_device, 1, descriptor_writes, 0, nullptr);
  }
}

void DepthMapRenderer::CreatePipeline() {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  // create pipeline layout
  VkPipelineLayoutCreateInfo pipeline_layout_create_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout_,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
  };
  CALL_VK(vkCreatePipelineLayout(logical_device, &pipeline_layout_create_info,
                                 nullptr, &pipeline_layout_));

  // create shader stages
  VkPipelineShaderStageCreateInfo shaderStages[] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = depth_map_vertex_shader_,
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = depth_map_fragment_shader_,
       .pName = "main"}};

  // create vertex input info
  // Passes two vec2s for position and uv.
  VkVertexInputBindingDescription vertex_input_bindings = {
      .binding = 0,
      .stride = 4 * sizeof(float),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  VkVertexInputAttributeDescription attributes[2] = {
      {
          // position
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = 0,
      },
      {
          // uv
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = 2 * sizeof(float),
      }};

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertex_input_bindings,
      .vertexAttributeDescriptionCount = 2,
      .pVertexAttributeDescriptions = attributes,
  };

  // create input assembly info
  VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  VkSurfaceCapabilitiesKHR surface_capabilities =
      vulkan_handler_->GetSurfaceCapabilities();
  // create viewport and scissor
  viewport_ = {
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(surface_capabilities.currentExtent.width),
      .height = static_cast<float>(surface_capabilities.currentExtent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  scissor_ = {
      .offset = {0, 0},
      .extent = surface_capabilities.currentExtent,
  };
  // create viewport state
  VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport_,
      .scissorCount = 1,
      .pScissors = &scissor_,
  };
  // create rasterizer state
  VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth =
          1,  // We need to set either lineWidth or VK_DYNAMIC_STATE_LINE_WIDTH
  };
  VkPipelineColorBlendAttachmentState attachment_states = {
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo color_blend_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &attachment_states,
  };

  // create multisampling state
  VkSampleMask sample_mask = ~0u;
  VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0,
      .pSampleMask = &sample_mask,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE,
  };
  // create depth stencil state
  VkPipelineDepthStencilStateCreateInfo depthStencil{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
  };
  // create graphics pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shaderStages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly_info,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &color_blend_info,
      .layout = pipeline_layout_,
      .renderPass = vulkan_handler_->GetRenderPass(),
      .subpass = 0,
  };
  CALL_VK(vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1,
                                    &pipelineInfo, nullptr, &pipeline_));
}

void DepthMapRenderer::UpdateDescriptorSets(int current_frame) {
  VkImageView depth_view = vulkan_handler_->GetDepthTextureView(current_frame);
  if (depth_view == VK_NULL_HANDLE) {
    return;  // The depth texture hasn't been created yet.
  }

  VkDescriptorImageInfo depth_image_info = {
      .sampler = vulkan_handler_->GetDepthSampler(),
      .imageView = depth_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  VkWriteDescriptorSet descriptor_write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptor_sets_[current_frame],
      .dstBinding = 0,  // Depth is at binding 0
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &depth_image_info,
  };

  vkUpdateDescriptorSets(vulkan_handler_->GetLogicalDevice(), 1,
                         &descriptor_write, 0, nullptr);
}

}  // namespace hello_ar_vulkan
