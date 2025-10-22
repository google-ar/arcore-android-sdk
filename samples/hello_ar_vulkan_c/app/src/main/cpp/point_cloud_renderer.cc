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

#include "point_cloud_renderer.h"

#include <vulkan/vulkan.h>

#include "../assets/shaders/point_cloud_frag.spv.h"
#include "../assets/shaders/point_cloud_vert.spv.h"
#include "arcore_c_api.h"
#include "android_vulkan_loader.h"
#include "glm.h"
#include "util.h"
#include "vulkan_handler.h"

namespace hello_ar_vulkan {
PointCloudRenderer::PointCloudRenderer(VulkanHandler* vulkan_handler,
                                       ArSession* ar_session)
    : vulkan_handler_(vulkan_handler), ar_session_(ar_session) {
  CHECK(vulkan_handler != nullptr);
  CHECK(ar_session != nullptr);

  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  vertex_shader_module_ = vulkan_handler_->LoadShader(
      logical_device, point_cloud_vert, sizeof(point_cloud_vert));
  fragment_shader_module_ = vulkan_handler_->LoadShader(
      logical_device, point_cloud_frag, sizeof(point_cloud_frag));
  CreateVertexBuffer();
  CreatePipeline();
}

PointCloudRenderer::~PointCloudRenderer() {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  vkDestroyPipeline(logical_device, pipeline_, nullptr);
  vkDestroyPipelineLayout(logical_device, pipeline_layout_, nullptr);
  vkDestroyShaderModule(logical_device, vertex_shader_module_, nullptr);
  vkDestroyShaderModule(logical_device, fragment_shader_module_, nullptr);
  for (int i = 0; i < vertex_buffers_.size(); ++i) {
    vulkan_handler_->CleanBuffer(vertex_buffers_[i], vertex_buffer_memories_[i],
                                 vertex_buffers_mapped_[i]);
  }
}

void PointCloudRenderer::Draw(int current_frame, ArPointCloud* point_cloud,
                              const glm::mat4& view_matrix,
                              const glm::mat4& projection_matrix) {
  glm::mat4 vp = projection_matrix * view_matrix;

  VkCommandBuffer command_buffer =
      vulkan_handler_->GetCommandBuffer(current_frame);
  vkCmdPushConstants(command_buffer, pipeline_layout_,
                     VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &vp);

  int32_t num_points = 0;
  ArPointCloud_getNumberOfPoints(ar_session_, point_cloud, &num_points);
  if (num_points <= 0) {
    return;
  }
  const float* point_cloud_data = nullptr;
  ArPointCloud_getData(ar_session_, point_cloud, &point_cloud_data);

  memcpy(vertex_buffers_mapped_[current_frame], point_cloud_data,
         sizeof(glm::vec4) * num_points);

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffers_[current_frame],
                         offsets);
  vkCmdDraw(command_buffer, num_points, 1, 0, 0);
}

void PointCloudRenderer::CreateVertexBuffer() {
  const int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  vertex_buffers_.resize(max_frames_in_flight);
  vertex_buffer_memories_.resize(max_frames_in_flight);
  vertex_buffers_mapped_.resize(max_frames_in_flight);

  // Currently, we only support 4096 points with a static buffer size.
  VkDeviceSize buffer_size = sizeof(glm::vec4) * kMaxPoints;

  for (int i = 0; i < max_frames_in_flight; ++i) {
    vulkan_handler_->CreateBuffer(
        vulkan_handler_->GetPhysicalDevice(), buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertex_buffers_[i], vertex_buffer_memories_[i]);
    vkMapMemory(vulkan_handler_->GetLogicalDevice(), vertex_buffer_memories_[i],
                0, buffer_size, 0, &vertex_buffers_mapped_[i]);
  }
}

void PointCloudRenderer::CreatePipeline() {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  VkPushConstantRange push_constant_range = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .offset = 0,
      .size = sizeof(glm::mat4),
  };

  // create pipeline layout
  VkPipelineLayoutCreateInfo pipeline_layout_create_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pSetLayouts = nullptr,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constant_range,
  };
  CALL_VK(vkCreatePipelineLayout(logical_device, &pipeline_layout_create_info,
                                 nullptr, &pipeline_layout_));

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
  VkVertexInputBindingDescription vertex_input_bindings = {
      .binding = 0,
      .stride = sizeof(glm::vec4),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  VkVertexInputAttributeDescription vertex_input_attributes = {
      .location = 0,
      .binding = 0,
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      .offset = 0,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertex_input_bindings,
      .vertexAttributeDescriptionCount = 1,
      .pVertexAttributeDescriptions = &vertex_input_attributes,
  };

  // create input assembly info
  VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  // create viewport and scissor
  const VkSurfaceCapabilitiesKHR surface_capabilities =
      vulkan_handler_->GetSurfaceCapabilities();
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
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
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

}  // namespace hello_ar_vulkan
