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

#include "plane_renderer.h"

#include <cstdint>

#include "../assets/shaders/plane_frag.spv.h"
#include "../assets/shaders/plane_vert.spv.h"
#include "android_vulkan_loader.h"
#include "util.h"
#include "vulkan_handler.h"

namespace hello_ar_vulkan {

PlaneRenderer::PlaneRenderer(VulkanHandler* vulkan_handler)
    : vulkan_handler_(vulkan_handler) {
  CHECK(vulkan_handler_ != nullptr);

  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  vertex_shader_module_ = vulkan_handler_->LoadShader(
      logical_device, plane_vert, sizeof(plane_vert));
  fragment_shader_module_ = vulkan_handler_->LoadShader(
      logical_device, plane_frag, sizeof(plane_frag));
  CreateUniformBuffers();
  CreateTexture();
  CreateDescriptorSetLayout();
  CreateDescriptorPool();
  CreateDescriptorSets();
  CreateVertexAndIndexBuffers();
  CreatePlanePipeline();
}

PlaneRenderer::~PlaneRenderer() {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();

  // Clean up pipeline resources
  vkDestroyPipelineLayout(logical_device, pipeline_layout_, nullptr);
  vkDestroyPipeline(logical_device, pipeline_, nullptr);

  // Clean up vertex and index buffers
  for (int i = 0; i < vertex_buffers_.size(); ++i) {
    vulkan_handler_->CleanBuffer(vertex_buffers_[i], vertex_buffer_memories_[i],
                                 vertex_buffers_mapped_[i]);
    vulkan_handler_->CleanBuffer(index_buffers_[i], index_buffer_memories_[i],
                                 index_buffers_mapped_[i]);
  }

  // clean up descriptor resources
  vkDestroyDescriptorPool(logical_device, descriptor_pool_, nullptr);
  vkDestroyDescriptorSetLayout(logical_device, descriptor_set_layout_, nullptr);

  // clean up texture resources
  vkDestroySampler(logical_device, sampler_, nullptr);
  vulkan_handler_->CleanTextureImageInfo(texture_image_info_);

  // clean up uniform buffers
  for (size_t i = 0; i < uniform_buffers_.size(); ++i) {
    vulkan_handler_->CleanBuffer(frag_uniform_buffers_[i],
                                 frag_uniform_buffers_memory_[i],
                                 frag_uniform_buffers_mapped_[i]);
    vulkan_handler_->CleanBuffer(uniform_buffers_[i],
                                 uniform_buffers_memory_[i],
                                 uniform_buffers_mapped_[i]);
  }
  // clean up shader modules
  vkDestroyShaderModule(logical_device, vertex_shader_module_, nullptr);
  vkDestroyShaderModule(logical_device, fragment_shader_module_, nullptr);
}

int PlaneRenderer::Draw(int current_frame, const ArSession& ar_session,
                        const ArTrackableList* plane_list,
                        const glm::mat4& view_matrix,
                        const glm::mat4& projection_matrix,
                        const glm::mat3& texture_transform_matrix,
                        float use_depth_for_occlusion) {
  // Generate vertices and draw commands
  std::vector<glm::vec3> all_vertices;
  std::vector<uint16_t> all_indices;
  std::vector<PlaneDrawParams> draw_commands;

  int32_t plane_list_size = 0;
  ArTrackableList_getSize(&ar_session, plane_list, &plane_list_size);

  // Loop through all planes and generate vertices and draw commands
  for (int i = 0; i < plane_list_size; ++i) {
    ArTrackable* ar_trackable = nullptr;
    ArTrackableList_acquireItem(&ar_session, plane_list, i, &ar_trackable);

    // Check if the plane is tracking
    ArTrackingState tracking_state;
    ArTrackable_getTrackingState(&ar_session, ar_trackable, &tracking_state);
    if (tracking_state != AR_TRACKING_STATE_TRACKING) {
      ArTrackable_release(ar_trackable);
      continue;
    }
    // Check if the plane is subsumed by another plane
    ArPlane* ar_plane = ArAsPlane(ar_trackable);
    ArPlane* subsumed_by = nullptr;
    ArPlane_acquireSubsumedBy(&ar_session, ar_plane, &subsumed_by);
    if (subsumed_by != nullptr) {
      ArTrackable_release(ArAsTrackable(subsumed_by));
      ArTrackable_release(ar_trackable);
      continue;
    }

    // Check if the plane is valid
    int32_t polygon_length;
    ArPlane_getPolygonSize(&ar_session, ar_plane, &polygon_length);
    if (polygon_length == 0) {
      ArTrackable_release(ar_trackable);
      continue;
    }

    // Get the plane's vertices
    const int32_t vertices_size = polygon_length / 2;
    std::vector<glm::vec2> raw_vertices(vertices_size);
    ArPlane_getPolygon(&ar_session, ar_plane,
                       glm::value_ptr(raw_vertices.front()));

    // Following the logic in the OpenGL version, we create two sets of vertices
    // for each plane:
    // - Outer vertices (alpha = 0)
    // - Inner vertices (alpha = 1)
    std::vector<glm::vec3> plane_vertices;
    // Outer vertices (alpha = 0)
    for (int32_t j = 0; j < vertices_size; ++j) {
      plane_vertices.push_back(
          glm::vec3(raw_vertices[j].x, raw_vertices[j].y, 0.0f));
    }
    // Inner vertices (alpha = 1)
    const float kFeatherLength = 0.2f;
    const float kFeatherScale = 0.2f;
    for (int32_t j = 0; j < vertices_size; ++j) {
      glm::vec2 v = raw_vertices[j];
      const float scale =
          1.0f - std::min((kFeatherLength / glm::length(v)), kFeatherScale);
      plane_vertices.push_back(glm::vec3(scale * v.x, scale * v.y, 1.0f));
    }

    // Triangulate the polygon
    std::vector<uint16_t> plane_indices;
    const int32_t half_vertices_length = plane_vertices.size() / 2;
    // Inner polygon
    for (int j = half_vertices_length + 1; j < plane_vertices.size() - 1; ++j) {
      plane_indices.push_back(half_vertices_length);
      plane_indices.push_back(j);
      plane_indices.push_back(j + 1);
    }
    // Create a Feathered edge
    for (int j = 0; j < half_vertices_length; ++j) {
      plane_indices.push_back(j);
      plane_indices.push_back((j + 1) % half_vertices_length);
      plane_indices.push_back(j + half_vertices_length);

      plane_indices.push_back(j + half_vertices_length);
      plane_indices.push_back((j + 1) % half_vertices_length);
      plane_indices.push_back((j + half_vertices_length + 1) %
                                  half_vertices_length +
                              half_vertices_length);
    }

    // Get the plane's pose, model matrix and normal
    util::ScopedArPose scopedArPose(&ar_session);
    ArPlane_getCenterPose(&ar_session, ar_plane, scopedArPose.GetArPose());
    glm::mat4 model_matrix;
    ArPose_getMatrix(&ar_session, scopedArPose.GetArPose(),
                     glm::value_ptr(model_matrix));
    glm::vec3 plane_normal =
        util::GetPlaneNormal(ar_session, *scopedArPose.GetArPose());

    // Create and store the draw command for this plane
    PlaneDrawParams cmd;
    cmd.model_matrix = model_matrix;
    cmd.plane_normal = plane_normal;
    cmd.index_count = plane_indices.size();
    cmd.first_index = all_indices.size();
    cmd.vertex_offset = all_vertices.size();
    draw_commands.push_back(cmd);

    // Add this plane's vertices and indices to the global lists
    all_vertices.insert(all_vertices.end(), plane_vertices.begin(),
                        plane_vertices.end());
    all_indices.insert(all_indices.end(), plane_indices.begin(),
                       plane_indices.end());

    ArTrackable_release(ar_trackable);
  }

  // If there's nothing to draw, return early
  if (draw_commands.empty()) {
    return 0;
  }

  // Upload all vertex/index data to the GPU in one go
  memcpy(vertex_buffers_mapped_[current_frame], all_vertices.data(),
         all_vertices.size() * sizeof(glm::vec3));

  memcpy(index_buffers_mapped_[current_frame], all_indices.data(),
         all_indices.size() * sizeof(uint16_t));

  // Update the shared uniform buffer (view/projection)
  UpdateUniformBuffers(current_frame, projection_matrix, view_matrix,
                       texture_transform_matrix, use_depth_for_occlusion);

  // Record the draw commands
  VkCommandBuffer command_buffer =
      vulkan_handler_->GetCommandBuffer(current_frame);

  // Bind pipeline and buffers once for all planes.
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffers_[current_frame],
                         offsets);
  vkCmdBindIndexBuffer(command_buffer, index_buffers_[current_frame], 0,
                       VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout_, 0, 1,
                          &descriptor_sets_[current_frame], 0, nullptr);

  // Loop through our clean list of commands and draw each plane.
  for (const auto& cmd : draw_commands) {
    // Push constants (model matrix and normal for this specific plane)
    PlanePushConstants push_constants = {
        .model_matrix = cmd.model_matrix,
        .plane_normal = cmd.plane_normal,
    };
    vkCmdPushConstants(command_buffer, pipeline_layout_,
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(PlanePushConstants), &push_constants);

    // Draw the indexed triangle mesh for this specific plane
    vkCmdDrawIndexed(command_buffer, cmd.index_count, 1, cmd.first_index,
                     cmd.vertex_offset, 0);
  }
  return plane_list_size;
}

void PlaneRenderer::CreateUniformBuffers() {
  VkDeviceSize buffer_size = sizeof(PlaneUniformBuffer);
  VkDeviceSize frag_buffer_size = sizeof(PlaneFragUniformBuffer);

  int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  VkPhysicalDevice physical_device = vulkan_handler_->GetPhysicalDevice();
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  // resize relevant UBO vectors
  uniform_buffers_.resize(max_frames_in_flight);
  uniform_buffers_memory_.resize(max_frames_in_flight);
  uniform_buffers_mapped_.resize(max_frames_in_flight);
  frag_uniform_buffers_.resize(max_frames_in_flight);
  frag_uniform_buffers_memory_.resize(max_frames_in_flight);
  frag_uniform_buffers_mapped_.resize(max_frames_in_flight);

  // create uniform buffers
  for (size_t i = 0; i < max_frames_in_flight; ++i) {
    vulkan_handler_->CreateBuffer(
        physical_device, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        uniform_buffers_[i], uniform_buffers_memory_[i]);
    vkMapMemory(logical_device, uniform_buffers_memory_[i], 0, buffer_size, 0,
                &uniform_buffers_mapped_[i]);
    vulkan_handler_->CreateBuffer(
        physical_device, frag_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        frag_uniform_buffers_[i], frag_uniform_buffers_memory_[i]);
    vkMapMemory(logical_device, frag_uniform_buffers_memory_[i], 0,
                frag_buffer_size, 0, &frag_uniform_buffers_mapped_[i]);
  }
}

void PlaneRenderer::CreateTexture() {
  // --- 1. Load the trigrid.png file into a staging buffer ---
  uint32_t width = 0;
  uint32_t height = 0;
  VkDeviceSize image_size = 0;
  void* pixel_data = nullptr;
  CHECK(util::LoadPngFromAssetManager("models/trigrid.png", width, height,
                                      image_size, pixel_data));

  // --- 2. Use Vulkan to create a staging buffer and upload data ---
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  VkPhysicalDevice physical_device = vulkan_handler_->GetPhysicalDevice();
  vulkan_handler_->CreateBuffer(physical_device, image_size,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
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
                        &texture_image_info_.texture_image));

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(
      logical_device, texture_image_info_.texture_image, &mem_requirements);
  VkMemoryAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex = vulkan_handler_->FindMemoryType(
          physical_device, mem_requirements.memoryTypeBits,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

  CALL_VK(vkAllocateMemory(logical_device, &alloc_info, nullptr,
                           &texture_image_info_.texture_memory));
  CALL_VK(vkBindImageMemory(logical_device, texture_image_info_.texture_image,
                            texture_image_info_.texture_memory, 0));

  // --- 4. Transition the image layout to TransferDstOptimal ---
  vulkan_handler_->TransitionImageLayout(texture_image_info_.texture_image,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  // --- 5. Copy the staging buffer into the image ---
  vulkan_handler_->CopyBufferToImage(
      staging_buffer, texture_image_info_.texture_image, width, height);
  // --- 6. Transition the image layout to ShaderReadOnlyOptimal ---
  vulkan_handler_->TransitionImageLayout(
      texture_image_info_.texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // Clean up the staging buffer
  vkDestroyBuffer(logical_device, staging_buffer, nullptr);
  vkFreeMemory(logical_device, staging_buffer_memory, nullptr);

  // --- 7. Create the texture image view ---
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
  // ---8. Create the sampler ---
  const VkSamplerCreateInfo sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
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

void PlaneRenderer::CreateDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uniform_buffer_binding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };
  VkDescriptorSetLayoutBinding sampler_layout_binding = {
      .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = &sampler_,
  };
  VkDescriptorSetLayoutBinding depth_sampler_layout_binding = {
      .binding = 2,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = nullptr,
  };
  VkDescriptorSetLayoutBinding frag_uniform_buffer_binding = {
      .binding = 3,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  VkDescriptorSetLayoutBinding bindings[4] = {
      uniform_buffer_binding, sampler_layout_binding,
      depth_sampler_layout_binding, frag_uniform_buffer_binding};
  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 4,
      .pBindings = bindings,
  };
  CALL_VK(vkCreateDescriptorSetLayout(vulkan_handler_->GetLogicalDevice(),
                                      &descriptor_set_layout_create_info,
                                      nullptr, &descriptor_set_layout_));
}

void PlaneRenderer::CreateDescriptorPool() {
  int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  VkDescriptorPoolSize pool_size_uniform_buffer = {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = static_cast<uint32_t>(max_frames_in_flight * 2),
  };
  VkDescriptorPoolSize pool_size_sampler = {
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = static_cast<uint32_t>(max_frames_in_flight * 2),
  };
  VkDescriptorPoolSize pool_sizes[2] = {pool_size_uniform_buffer,
                                        pool_size_sampler};

  VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = static_cast<uint32_t>(max_frames_in_flight),
      .poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]),
      .pPoolSizes = pool_sizes,
  };
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  CALL_VK(vkCreateDescriptorPool(logical_device, &descriptor_pool_create_info,
                                 nullptr, &descriptor_pool_));
}

void PlaneRenderer::CreateDescriptorSets() {
  int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  descriptor_sets_.resize(max_frames_in_flight);

  std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight,
                                             descriptor_set_layout_);
  VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data(),
  };
  CALL_VK(vkAllocateDescriptorSets(logical_device, &alloc_info,
                                   descriptor_sets_.data()));

  for (int i = 0; i < max_frames_in_flight; i++) {
    // Update descriptor sets for plane uniform buffer.
    VkDescriptorBufferInfo buffer_info = {
        .buffer = uniform_buffers_[i],
        .offset = 0,
        .range = sizeof(PlaneUniformBuffer),
    };
    VkWriteDescriptorSet write_descriptor_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &buffer_info,
    };

    // Update descriptor sets for plane texture.
    VkDescriptorImageInfo image_info = {
        .sampler = VK_NULL_HANDLE,
        .imageView = texture_image_info_.texture_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet write_descriptor_set_sampler = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_[i],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &image_info,
    };

    // Update descriptor sets for fragment uniform buffer.
    VkDescriptorBufferInfo frag_buffer_info = {
        .buffer = frag_uniform_buffers_[i],
        .offset = 0,
        .range = sizeof(PlaneFragUniformBuffer),
    };
    VkWriteDescriptorSet write_descriptor_set_frag_buffer = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_sets_[i],
        .dstBinding = 3,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &frag_buffer_info,
    };

    VkWriteDescriptorSet write_descriptor_sets[3] = {
        write_descriptor_set, write_descriptor_set_sampler,
        write_descriptor_set_frag_buffer};

    vkUpdateDescriptorSets(logical_device, 3, write_descriptor_sets, 0,
                           nullptr);
  }
}

void PlaneRenderer::CreateVertexAndIndexBuffers() {
  int max_frames_in_flight = vulkan_handler_->GetMaxFramesInFlight();

  vertex_buffers_.resize(max_frames_in_flight);
  vertex_buffer_memories_.resize(max_frames_in_flight);
  vertex_buffers_mapped_.resize(max_frames_in_flight);
  index_buffers_.resize(max_frames_in_flight);
  index_buffer_memories_.resize(max_frames_in_flight);
  index_buffers_mapped_.resize(max_frames_in_flight);

  // Currently, we only support 8192 vertices with a static buffer size.
  VkDeviceSize vertex_buffer_size = sizeof(glm::vec3) * kMaxPlaneVertices;
  VkDeviceSize index_buffer_size = sizeof(uint16_t) * kMaxPlaneIndices;

  VkPhysicalDevice physical_device = vulkan_handler_->GetPhysicalDevice();
  for (int i = 0; i < max_frames_in_flight; ++i) {
    vulkan_handler_->CreateBuffer(
        physical_device, vertex_buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertex_buffers_[i], vertex_buffer_memories_[i]);
    vkMapMemory(vulkan_handler_->GetLogicalDevice(), vertex_buffer_memories_[i],
                0, vertex_buffer_size, 0, &vertex_buffers_mapped_[i]);
    vulkan_handler_->CreateBuffer(physical_device, index_buffer_size,
                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  index_buffers_[i], index_buffer_memories_[i]);
    vkMapMemory(vulkan_handler_->GetLogicalDevice(), index_buffer_memories_[i],
                0, index_buffer_size, 0, &index_buffers_mapped_[i]);
  }
}

void PlaneRenderer::CreatePlanePipeline() {
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  VkSurfaceCapabilitiesKHR surface_capabilities =
      vulkan_handler_->GetSurfaceCapabilities();

  VkPushConstantRange push_constant_range = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .offset = 0,
      .size = sizeof(PlanePushConstants),
  };

  // Create pipeline layout
  VkPipelineLayoutCreateInfo pipeline_layout_create_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout_,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constant_range,
  };
  CALL_VK(vkCreatePipelineLayout(logical_device, &pipeline_layout_create_info,
                                 nullptr, &pipeline_layout_));

  // Create shader stages
  VkPipelineShaderStageCreateInfo shaderStages[] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = vertex_shader_module_,
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = fragment_shader_module_,
       .pName = "main"}};

  // Vertex input info contains just the plane vertices
  VkVertexInputBindingDescription vertex_input_bindings = {
      .binding = 0,
      .stride = sizeof(glm::vec3),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  VkVertexInputAttributeDescription vertex_input_attributes = {
      .location = 0,
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = 0,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertex_input_bindings,
      .vertexAttributeDescriptionCount = 1,
      .pVertexAttributeDescriptions = &vertex_input_attributes,
  };

  // Create input assembly info
  VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  // Create viewport and scissor
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
  // Create viewport state
  VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
  };
  // Create rasterizer state
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
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
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

  // Create multisampling state
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
  // Create depth stencil state
  VkPipelineDepthStencilStateCreateInfo depthStencil{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
  };
  // Create graphics pipeline
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

void PlaneRenderer::UpdateUniformBuffers(
    int current_frame, const glm::mat4& projection_matrix,
    const glm::mat4& view_matrix, const glm::mat3& texture_transform_matrix,
    float use_depth_for_occlusion) {
  // Update the uniform buffer for the plane.
  PlaneUniformBuffer plane_uniform_buffer = {
      .projection_view_matrix = projection_matrix * view_matrix,
      .view_matrix = view_matrix,
  };
  VkDevice logical_device = vulkan_handler_->GetLogicalDevice();
  memcpy(uniform_buffers_mapped_[current_frame], &plane_uniform_buffer,
         sizeof(PlaneUniformBuffer));

  // Update the fragment uniform buffer for the plane.
  PlaneFragUniformBuffer plane_frag_uniform_buffer = {
      .depth_uv_transform = glm::mat4(
          texture_transform_matrix),  // This is the transform for depth
      .depth_aspect_ratio = vulkan_handler_->GetDepthAspectRatio(current_frame),
      .use_depth_for_occlusion = use_depth_for_occlusion,
  };
  memcpy(frag_uniform_buffers_mapped_[current_frame],
         &plane_frag_uniform_buffer, sizeof(PlaneFragUniformBuffer));

  // Update the depth texture descriptor.
  VkImageView depth_texture_view =
      vulkan_handler_->GetDepthTextureView(current_frame);
  if (depth_texture_view == VK_NULL_HANDLE) {
    return;
  }
  VkDescriptorImageInfo image_info = {
      .sampler = vulkan_handler_->GetDepthSampler(),
      .imageView = depth_texture_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  VkWriteDescriptorSet write_descriptor_set = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptor_sets_[current_frame],
      .dstBinding = 2,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info,
  };
  vkUpdateDescriptorSets(logical_device, 1, &write_descriptor_set, 0, nullptr);
}
}  // namespace hello_ar_vulkan
