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

#version 450 core

// Inputs from the vertex shader.
layout(location = 0) in vec2 in_uv;
layout(location = 1) in float in_alpha;
layout(location = 2) in vec3 view_position; // Post-projection
layout(location = 3) in vec3 screen_space_position;
layout(location = 4) in float in_view_depth; // Input view space Z

// The combined image sampler for the grid texture.
layout(binding = 1) uniform sampler2D u_grid_texture;
layout(binding = 2) uniform sampler2D u_depth_texture;

layout(binding = 3) uniform UboFrag {
    mat4 depth_uv_transform;
    float depth_aspect_ratio;
    float use_depth_for_occlusion;
} ubo_frag;

// The final output color.
layout(location = 0) out vec4 out_frag_color;

float DepthGetMillimeters(in sampler2D depth_texture, in vec2 depth_uv) {
  const float kMaxDepthValue = 65535.0;
  return texture(depth_texture, depth_uv).r * kMaxDepthValue;
}

// Returns linear interpolation position of value between min and max bounds.
// E.g., DepthInverseLerp(1100, 1000, 2000) returns 0.1.
float DepthInverseLerp(in float value, in float min_bound, in float max_bound) {
  return clamp((value - min_bound) / (max_bound - min_bound), 0.0, 1.0);
}

// Returns a value between 0.0 (not visible) and 1.0 (completely visible)
// Which represents how visible or occluded is the pixel in relation to the
// depth map.
float DepthGetVisibility(in sampler2D depth_texture, in vec2 depth_uv,
                         in float asset_depth_mm) {
  const float kMinDepthMm = 150.0;
  const float kMaxDepthMm = 200.0;
  const float kMinFarDepthMm = 7500.0;
  const float kMaxFarDepthMm = 8000.0;

  float depth_mm = DepthGetMillimeters(depth_texture, depth_uv);

  // visibility_occlusion is 1.0 if scene is much farther than asset, 0.0 if much closer.
  const float kDepthTolerancePerMm = 0.3;  // Adjust tolerance as needed.
  float visibility_occlusion = clamp(0.5 * (depth_mm - asset_depth_mm) /
    max(kDepthTolerancePerMm * asset_depth_mm, 1.0) + 0.5, 0.0, 1.0);

  // visibility_depth_near is 1.0 if depth_mm > kMaxDepthMm, 0.0 if depth_mm < kMinDepthMm.
  // This means the scene depth is valid (not too close).
  float visibility_depth_near = DepthInverseLerp(
      depth_mm, kMinDepthMm, kMaxDepthMm);

  // visibility_depth_far is 1.0 if depth_mm < kMinFarDepthMm, 0.0 if depth_mm > kMaxFarDepthMm.
  // This means the scene depth is valid (not too far).
  float visibility_depth_far = 1.0 - DepthInverseLerp(
      depth_mm, kMinFarDepthMm, kMaxFarDepthMm);

  // The final visibility is the minimum of all factors.
  float visibility = visibility_occlusion;
  visibility = min(visibility, visibility_depth_near);
  visibility = min(visibility, visibility_depth_far);

  return visibility;
}

float DepthGetBlurredVisibilityAroundUV(in sampler2D depth_texture, in vec2 uv,
                                        in float asset_depth_mm) {
  // Kernel used:
  // 0   4   7   4   0
  // 4   16  26  16  4
  // 7   26  41  26  7
  // 4   16  26  16  4
  // 0   4   7   4   0
  const float kKernelTotalWeights = 269.0;
  float sum = 0.0;

  const float kOcclusionBlurAmount = 0.005;
  vec2 blurriness = vec2(kOcclusionBlurAmount,
                         kOcclusionBlurAmount * ubo_frag.depth_aspect_ratio);

  float current = 0.0;

  current += DepthGetVisibility
  (depth_texture, uv + vec2(-1.0, -2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+1.0, -2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(-1.0, +2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+1.0, +2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(-2.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+2.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(-2.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+2.0, -1.0) * blurriness, asset_depth_mm);
  sum += current * 4.0;

  current = DepthGetVisibility(depth_texture, uv + vec2(-2.0, -0.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+2.0, +0.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+0.0, +2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-0.0, -2.0) * blurriness, asset_depth_mm);
  sum += current * 7.0;

  current = DepthGetVisibility(depth_texture, uv + vec2(-1.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+1.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-1.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+1.0, +1.0) * blurriness, asset_depth_mm);
  sum += current * 16.0;

  current = DepthGetVisibility(depth_texture, uv + vec2(+0.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-0.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-1.0, -0.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+1.0, +0.0) * blurriness, asset_depth_mm);
  sum += current * 26.0;

  sum += DepthGetVisibility(depth_texture, uv , asset_depth_mm) * 41.0;

  return sum / kKernelTotalWeights;
}

void main() {
    float r = texture(u_grid_texture, in_uv).r;
    vec4 grid_color = vec4(r * in_alpha);

    if (ubo_frag.use_depth_for_occlusion == 1.0) {
      // in_view_depth is the Z component in View Space.
      // Conventionally, objects in front of the camera have negative Z in View Space.
      // Depth is a positive distance.
      float asset_depth_mm = -in_view_depth * 1000.0;

      // Add a bias to make the plane appear closer to the camera for occlusion purposes.
      const float kDepthBiasMm = 100.0;  // Experiment with this value as needed.
      float biased_asset_depth_mm = asset_depth_mm - kDepthBiasMm;

      // Convert from openGL to vulkan coordinate systems.
      vec3 screen_space_position_vec = screen_space_position * .5 + .5;
      // Apply the depth uv transform to get the depth texture coordinates.
      vec2 depth_uv = (mat3(ubo_frag.depth_uv_transform) *
          vec3(screen_space_position_vec.xy, 1.0)).xy;

      float visibility = DepthGetBlurredVisibilityAroundUV(
          u_depth_texture, depth_uv, biased_asset_depth_mm);

      // Apply visibility to the alpha channel.
      out_frag_color = grid_color * visibility;

    } else {
      out_frag_color = grid_color;
    }
}
