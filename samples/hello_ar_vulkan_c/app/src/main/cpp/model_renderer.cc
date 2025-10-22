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

#include "model_renderer.h"

#include <cstddef>
#include <string>
#include <vector>

#include "../assets/shaders/model_frag.spv.h"
#include "../assets/shaders/model_vert.spv.h"
#include "jni_interface.h"
#include "util.h"

namespace hello_ar_vulkan {
ModelRenderer::ModelRenderer(VulkanHandler* vulkan_handler,
                             AAssetManager* asset_manager)
    : vulkan_handler_(vulkan_handler) {
  CHECK(vulkan_handler_ != nullptr);
  vertex_shader_module_ = vulkan_handler_->LoadShader(
      vulkan_handler_->GetLogicalDevice(),
      reinterpret_cast<const uint32_t*>(model_vert), sizeof(model_vert));
  fragment_shader_module_ = vulkan_handler_->LoadShader(
      vulkan_handler_->GetLogicalDevice(),
      reinterpret_cast<const uint32_t*>(model_frag), sizeof(model_frag));
  CreateUniformBuffers();  // Creates the uniform buffers, memory, and mapped
                           // data.
  CreateTexture();         // Creates the texture image and sampler.
  CreateDescriptorSetLayout();  // Creates the descriptor set layout.
  CreateDescriptorPool();       // Creates the descriptor pool.
  CreateDescriptorSet();        // Creates the descriptor set.

  // loading obj file and creating vertex/index buffers
  util::LoadObjFile(std::string("models/andy.obj"), asset_manager, vertices_,
                    normals_, uvs_, indices_);

  for (int i = 0; i < vertices_.size() / 3; i++) {
    Vertex vertex;
    vertex.position =
        glm::vec3(vertices_[i * 3], vertices_[i * 3 + 1], vertices_[i * 3 + 2]);
    vertex.uv = glm::vec2(uvs_[i * 2], uvs_[i * 2 + 1]);
    vertex.normal =
        glm::vec3(normals_[i * 3], normals_[i * 3 + 1], normals_[i * 3 + 2]);
    model_vertices_.push_back(vertex);
  }
  CreateModelVertexBuffer();  // Creates the vertex buffer and its memory.
  CreateModelIndexBuffer();   // Creates the index buffer and its memory.
  CreateInstanceBuffers();    // Creates the instance buffers and their memory.
  CreateModelPipeline();      // Creates the pipeline.
}

ModelRenderer::~ModelRenderer() {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();

  // Created by DrawModel() function.
  vkDestroyPipeline(logical_device, pipeline_, nullptr);
  vkDestroyPipelineLayout(logical_device, pipeline_layout_, nullptr);

  // Created by CreateInstanceBuffers() function.
  for (int i = 0; i < instance_buffers_.size(); ++i) {
    vulkan_handler_->CleanBuffer(instance_buffers_[i],
                                 instance_buffer_memories_[i],
                                 instance_buffers_mapped_[i]);
  }

  // Created by CreateModelIndexBuffer() function.
  vulkan_handler_->CleanBuffer(index_buffer_, index_buffer_memory_);

  // Created by CreateModelVertexBuffer() function.
  vulkan_handler_->CleanBuffer(vertex_buffer_, vertex_buffer_memory_);

  // Created by CreateDescriptorPool() function.
  vkDestroyDescriptorPool(logical_device, descriptor_pool_, nullptr);

  // Created by CreateDescriptorSetLayout() function.
  vkDestroyDescriptorSetLayout(logical_device, descriptor_set_layout_, nullptr);

  // Created by CreateTexture() function.
  vulkan_handler_->CleanTextureImageInfo(texture_image_info_);
  vkDestroySampler(logical_device, sampler_, nullptr);

  // Created by CreateUniformBuffers() function.
  for (size_t i = 0; i < vulkan_handler_->GetMaxFramesInFlight(); i++) {
    vulkan_handler_->CleanBuffer(fragment_uniform_buffers_[i],
                                 fragment_uniform_buffers_memory_[i],
                                 fragment_uniform_buffers_mapped_[i]);
  }

  // Created by constructor.
  vkDestroyShaderModule(logical_device, vertex_shader_module_, nullptr);
  vkDestroyShaderModule(logical_device, fragment_shader_module_, nullptr);
}

void ModelRenderer::DrawModel(int current_frame,
                              std::vector<util::InstanceData>& instance_data,
                              const glm::mat4& projection_matrix,
                              const glm::mat3& texture_transform_matrix,
                              float use_depth_for_occlusion,
                              const float* color_correction) {
  if (instance_data.empty()) {
    return;
  }

  memcpy(instance_buffers_mapped_[current_frame], instance_data.data(),
         sizeof(InstanceData) * instance_data.size());

  VkCommandBuffer command_buffer =
      vulkan_handler_->GetCommandBuffer(current_frame);

  // Update the depth descriptor sets.
  UpdateFragmentDescriptorSets(current_frame, texture_transform_matrix,
                               use_depth_for_occlusion, color_correction);
  vkCmdPushConstants(command_buffer, pipeline_layout_,
                     VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                     &projection_matrix);

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

  VkDeviceSize offsets[2] = {0, 0};
  VkBuffer vertex_and_instance_buffer[2] = {vertex_buffer_,
                                            instance_buffers_[current_frame]};
  vkCmdBindVertexBuffers(command_buffer, 0, 2, vertex_and_instance_buffer,
                         offsets);
  vkCmdBindIndexBuffer(command_buffer, index_buffer_, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout_, 0, 1,
                          &descriptor_sets_[current_frame], 0, nullptr);

  vkCmdDrawIndexed(command_buffer, indices_.size(), instance_data.size(), 0, 0,
                   0);
}

void ModelRenderer::CreateModelPipeline() {
  VkSurfaceCapabilitiesKHR surface_capabilities =
      vulkan_handler_->GetSurfaceCapabilities();

  VkPushConstantRange push_constant_range = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .offset = 0,
      .size = sizeof(glm::mat4),
  };

  // create pipeline layout
  VkPipelineLayoutCreateInfo pipeline_layout_create_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout_,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constant_range,
  };
  CALL_VK(vkCreatePipelineLayout(vulkan_handler_->GetLogicalDevice(),
                                 &pipeline_layout_create_info, nullptr,
                                 &pipeline_layout_));
  // create shader stages
  VkPipelineShaderStageCreateInfo shaderStages[] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = vertex_shader_module_,
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = fragment_shader_module_,
       .pName = "main"}};

  // create vertex input info
  VkVertexInputBindingDescription per_vertex_input_bindings = {
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  // create instance input info
  VkVertexInputBindingDescription per_instance_input_bindings = {
      .binding = 1,
      .stride = sizeof(InstanceData),
      .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,  // Rate is per instance.
  };

  VkVertexInputBindingDescription binding_descriptions[2] = {
      per_vertex_input_bindings, per_instance_input_bindings};

  VkVertexInputAttributeDescription vertex_input_attributes = {
      .location = 0,
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = 0,
  };

  VkVertexInputAttributeDescription uv_input_attributes = {
      .location = 1,
      .binding = 0,
      .format = VK_FORMAT_R32G32_SFLOAT,
      .offset = offsetof(Vertex, uv),
  };

  VkVertexInputAttributeDescription normal_input_attributes = {
      .location = 2,
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = offsetof(Vertex, normal),
  };

  // A mat4 is treated as 4 vec4s
  VkVertexInputAttributeDescription model_view_input_attributes1 = {
      .location = 3,
      .binding = 1,
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      .offset = offsetof(InstanceData, model_view),
  };

  VkVertexInputAttributeDescription model_view_input_attributes2 = {
      .location = 4,
      .binding = 1,
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      .offset = offsetof(InstanceData, model_view) + sizeof(glm::vec4),
  };

  VkVertexInputAttributeDescription model_view_input_attributes3 = {
      .location = 5,
      .binding = 1,
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      .offset = offsetof(InstanceData, model_view) + (sizeof(glm::vec4) * 2),
  };

  VkVertexInputAttributeDescription model_view_input_attributes4 = {
      .location = 6,
      .binding = 1,
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      .offset = offsetof(InstanceData, model_view) + (sizeof(glm::vec4) * 3),
  };
  // End of mat4, start of object_color
  VkVertexInputAttributeDescription color_input_attributes = {
      .location = 7,
      .binding = 1,
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      .offset = offsetof(InstanceData, object_color),
  };

  VkVertexInputAttributeDescription vertex_attributes_info[8] = {
      vertex_input_attributes,      uv_input_attributes,
      normal_input_attributes,      model_view_input_attributes1,
      model_view_input_attributes2, model_view_input_attributes3,
      model_view_input_attributes4, color_input_attributes};

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 2,
      .pVertexBindingDescriptions = binding_descriptions,
      .vertexAttributeDescriptionCount = 8,
      .pVertexAttributeDescriptions = vertex_attributes_info,
  };

  // create input assembly info
  VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  // create viewport and scissor
  VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(surface_capabilities.currentExtent.width),
      .height = static_cast<float>(surface_capabilities.currentExtent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  VkRect2D scissor{
      .offset = {0, 0},
      .extent = surface_capabilities.currentExtent,
  };
  // create viewport state
  VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
  };
  // create rasterizer state
  VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1,
  };
  VkPipelineColorBlendAttachmentState attachment_states = {
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
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
  VkPipelineDepthStencilStateCreateInfo depth_stencil{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
  };
  // create graphics pipeline
  VkGraphicsPipelineCreateInfo pipeline_info{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shaderStages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly_info,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depth_stencil,
      .pColorBlendState = &color_blend_info,
      .layout = pipeline_layout_,
      .renderPass = vulkan_handler_->GetRenderPass(),
      .subpass = 0,
  };
  CALL_VK(vkCreateGraphicsPipelines(vulkan_handler_->GetLogicalDevice(),
                                    VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                    &pipeline_));
}

void ModelRenderer::CreateTexture() {
  // call JNI to get pixel data
  uint32_t width = 0;
  uint32_t height = 0;
  VkDeviceSize image_size = 0;
  void* pixel_data = nullptr;
  CHECK(util::LoadPngFromAssetManager("models/andy.png", width, height,
                                      image_size, pixel_data));

  // use Vulkan to create a staging buffer and upload data
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  vulkan_handler_->CreateBuffer(vulkan_handler_->GetPhysicalDevice(),
                                image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                staging_buffer, staging_buffer_memory);

  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();

  // copy pixel data from Java into the staging buffer
  void* mapped_data;
  vkMapMemory(logical_device, staging_buffer_memory, 0, image_size, 0,
              &mapped_data);
  memcpy(mapped_data, pixel_data, static_cast<size_t>(image_size));
  vkUnmapMemory(vulkan_handler_->GetLogicalDevice(), staging_buffer_memory);
  free(pixel_data);

  // create the final VkImage and copy the staging buffer into it
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
                        &texture_image_info_.texture_image));

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(
      logical_device, texture_image_info_.texture_image, &mem_requirements);
  VkMemoryAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex = vulkan_handler_->FindMemoryType(
          vulkan_handler_->GetPhysicalDevice(), mem_requirements.memoryTypeBits,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

  CALL_VK(vkAllocateMemory(logical_device, &alloc_info, nullptr,
                           &texture_image_info_.texture_memory));
  CALL_VK(vkBindImageMemory(logical_device, texture_image_info_.texture_image,
                            texture_image_info_.texture_memory, 0));

  // transition the image layout to TransferDstOptimal ---
  vulkan_handler_->TransitionImageLayout(texture_image_info_.texture_image,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  // copy the staging buffer into the image ---
  vulkan_handler_->CopyBufferToImage(
      staging_buffer, texture_image_info_.texture_image, width, height);
  // transition the image layout to ShaderReadOnlyOptimal
  vulkan_handler_->TransitionImageLayout(
      texture_image_info_.texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // clean up the staging buffer
  vkDestroyBuffer(logical_device, staging_buffer, nullptr);
  vkFreeMemory(logical_device, staging_buffer_memory, nullptr);

  // create the texture image view
  const VkImageViewCreateInfo texture_image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .flags = 0,
      .image = texture_image_info_.texture_image,
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
  CALL_VK(vkCreateImageView(logical_device, &texture_image_view_create_info,
                            nullptr, &texture_image_info_.texture_image_view));
  // create the sampler
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
                          &sampler_));
}
void ModelRenderer::CreateDescriptorSetLayout() {
  // Vertex shader sampler binding
  VkDescriptorSetLayoutBinding model_texture_sampler_layout_binding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = &sampler_,
  };
  // Fragment shader sampler binding
  VkDescriptorSetLayoutBinding depth_texture_sampler_layout_binding = {
      .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = nullptr,
  };
  // Fragment shader uniform buffer binding
  VkDescriptorSetLayoutBinding fragment_shader_uniform_buffer_binding = {
      .binding = 2,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  VkDescriptorSetLayoutBinding bindings[3] = {
      model_texture_sampler_layout_binding,
      depth_texture_sampler_layout_binding,
      fragment_shader_uniform_buffer_binding,
  };
  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 3,
      .pBindings = bindings,
  };
  CALL_VK(vkCreateDescriptorSetLayout(vulkan_handler_->GetLogicalDevice(),
                                      &descriptor_set_layout_create_info,
                                      nullptr, &descriptor_set_layout_));
}

void ModelRenderer::CreateDescriptorPool() {
  int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  // Two uniform buffers are needed for each frame.
  VkDescriptorPoolSize pool_size_uniform_buffer = {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = static_cast<uint32_t>(max_frames_in_flight),
  };
  // Two samplers are needed for each frame. One for the model texture and one
  // for the depth texture.
  VkDescriptorPoolSize pool_size_sampler = {
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = static_cast<uint32_t>(max_frames_in_flight * 2),
  };
  VkDescriptorPoolSize pool_sizes[2] = {pool_size_uniform_buffer,
                                        pool_size_sampler};

  VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = static_cast<uint32_t>(max_frames_in_flight),
      .poolSizeCount = 2,
      .pPoolSizes = pool_sizes,
  };
  CALL_VK(vkCreateDescriptorPool(vulkan_handler_->GetLogicalDevice(),
                                 &descriptor_pool_create_info, nullptr,
                                 &descriptor_pool_));
}

void ModelRenderer::CreateDescriptorSet() {
  int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  descriptor_sets_.resize(max_frames_in_flight);
  std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight,
                                             descriptor_set_layout_);
  VkDescriptorSetAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data(),
  };
  CALL_VK(vkAllocateDescriptorSets(vulkan_handler_->GetLogicalDevice(),
                                   &alloc_info, descriptor_sets_.data()));

  for (int i = 0; i < max_frames_in_flight; ++i) {
    VkDescriptorImageInfo image_info = {
        .sampler = sampler_,
        .imageView = texture_image_info_.texture_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet write_descriptor_set_sampler = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &image_info,
    };

    VkDescriptorBufferInfo fragment_buffer_info = {
        .buffer = fragment_uniform_buffers_[i],
        .offset = 0,
        .range = sizeof(FragmentUniformBuffer),
    };
    VkWriteDescriptorSet write_descriptor_set_fragment_buffer = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_[i],
        .dstBinding = 2,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &fragment_buffer_info,
    };

    // Depth map does not need to be updated on initialization, so we don't need
    // to include it in the write descriptor sets.
    VkWriteDescriptorSet write_descriptor_sets[2] = {
        write_descriptor_set_sampler, write_descriptor_set_fragment_buffer};

    vkUpdateDescriptorSets(vulkan_handler_->GetLogicalDevice(), 2,
                           write_descriptor_sets, 0, nullptr);
  }
}

void ModelRenderer::CreateUniformBuffers() {
  VkDeviceSize fragment_buffer_size = sizeof(FragmentUniformBuffer);
  const int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  VkPhysicalDevice physical_device = vulkan_handler_->GetPhysicalDevice();

  // Resize the uniform buffers to the max number of frames in flight.
  fragment_uniform_buffers_.resize(max_frames_in_flight);
  fragment_uniform_buffers_memory_.resize(max_frames_in_flight);
  fragment_uniform_buffers_mapped_.resize(max_frames_in_flight);

  // create uniform buffers
  for (size_t i = 0; i < max_frames_in_flight; ++i) {
    vulkan_handler_->CreateBuffer(physical_device, fragment_buffer_size,
                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  fragment_uniform_buffers_[i],
                                  fragment_uniform_buffers_memory_[i]);
    vkMapMemory(vulkan_handler_->GetLogicalDevice(),
                fragment_uniform_buffers_memory_[i], 0, fragment_buffer_size, 0,
                &fragment_uniform_buffers_mapped_[i]);
  }
}

void ModelRenderer::CreateModelVertexBuffer() {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  VkPhysicalDevice physical_device = vulkan_handler_->GetPhysicalDevice();
  VkDeviceSize buffer_size =
      sizeof(model_vertices_[0]) * model_vertices_.size();

  // 1. Create a temporary, CPU-visible staging buffer
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  vulkan_handler_->CreateBuffer(physical_device, buffer_size,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                staging_buffer, staging_buffer_memory);

  // 2. Map memory and copy data to the staging buffer
  void* mapped_data;
  vkMapMemory(logical_device, staging_buffer_memory, 0, buffer_size, 0,
              &mapped_data);
  memcpy(mapped_data, model_vertices_.data(), buffer_size);
  vkUnmapMemory(logical_device, staging_buffer_memory);

  // 3. Create the final, GPU-only vertex buffer
  vulkan_handler_->CreateBuffer(
      physical_device, buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer_,
      vertex_buffer_memory_);

  // 4. Copy data from the staging buffer to the vertex buffer on the GPU
  vulkan_handler_->CopyBuffer(staging_buffer, vertex_buffer_, buffer_size);

  // 5. Clean up the temporary staging buffer
  vkDestroyBuffer(logical_device, staging_buffer, nullptr);
  vkFreeMemory(logical_device, staging_buffer_memory, nullptr);
}

void ModelRenderer::CreateModelIndexBuffer() {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  VkDeviceSize buffer_size = sizeof(indices_[0]) * indices_.size();

  // Create a temporary, CPU-visible staging buffer
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  vulkan_handler_->CreateBuffer(vulkan_handler_->GetPhysicalDevice(),
                                buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                staging_buffer, staging_buffer_memory);

  // Map memory and copy data to the staging buffer
  void* mapped_data;
  vkMapMemory(logical_device, staging_buffer_memory, 0, buffer_size, 0,
              &mapped_data);
  memcpy(mapped_data, indices_.data(), buffer_size);
  vkUnmapMemory(logical_device, staging_buffer_memory);

  vulkan_handler_->CreateBuffer(
      vulkan_handler_->GetPhysicalDevice(), buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      index_buffer_, index_buffer_memory_);

  // Copy data from the staging buffer to the index buffer on the GPU
  vulkan_handler_->CopyBuffer(staging_buffer, index_buffer_, buffer_size);
  // Clean up the temporary staging buffer
  vkDestroyBuffer(logical_device, staging_buffer, nullptr);
  vkFreeMemory(logical_device, staging_buffer_memory, nullptr);
}

void ModelRenderer::CreateInstanceBuffers() {
  int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  instance_buffers_.resize(max_frames_in_flight);
  instance_buffer_memories_.resize(max_frames_in_flight);
  instance_buffers_mapped_.resize(max_frames_in_flight);
  VkDeviceSize buffer_size =
      sizeof(InstanceData) * util::kMaxNumberOfAndroidsToRender;
  for (int i = 0; i < max_frames_in_flight; ++i) {
    vulkan_handler_->CreateBuffer(
        vulkan_handler_->GetPhysicalDevice(), buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        instance_buffers_[i], instance_buffer_memories_[i]);
    vkMapMemory(vulkan_handler_->GetLogicalDevice(),
                instance_buffer_memories_[i], 0, buffer_size, 0,
                &instance_buffers_mapped_[i]);
  }
}

void ModelRenderer::UpdateFragmentDescriptorSets(
    int current_frame, const glm::mat3& texture_transform_matrix,
    float use_depth_for_occlusion, const float* color_correction4) {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();

  VkImageView depth_texture_view =
      vulkan_handler_->GetDepthTextureView(current_frame);
  if (depth_texture_view == VK_NULL_HANDLE) {
    LOGI("depth texture view is null, not updating depth descriptor sets");
    return;
  }

  // Update the depth texture
  VkDescriptorImageInfo image_info = {
      .sampler = vulkan_handler_->GetDepthSampler(),
      .imageView = depth_texture_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  VkWriteDescriptorSet write_descriptor_set = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptor_sets_[current_frame],
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info,
  };
  vkUpdateDescriptorSets(logical_device, 1, &write_descriptor_set, 0, nullptr);

  // Material parameters
  const float ambient = 0.0f;
  const float diffuse = 2.0f;
  const float specular = 0.5f;
  const float specular_power = 6.0f;
  const glm::vec4 material_parameters =
      glm::vec4(ambient, diffuse, specular, specular_power);

  glm::vec4 color_correction_parameters =
      glm::vec4(color_correction4[0], color_correction4[1],
                color_correction4[2], color_correction4[3]);

  // Update the fragment uniform buffer
  FragmentUniformBuffer fragment_ubo = {
      .depth_uv_transform = glm::mat4(texture_transform_matrix),
      .color_correction_parameters = color_correction_parameters,
      .material_parameters = material_parameters,
      .depth_aspect_ratio = vulkan_handler_->GetDepthAspectRatio(current_frame),
      .use_depth_for_occlusion = use_depth_for_occlusion,
  };

  memcpy(fragment_uniform_buffers_mapped_[current_frame], &fragment_ubo,
         sizeof(fragment_ubo));
}

}  // namespace hello_ar_vulkan
