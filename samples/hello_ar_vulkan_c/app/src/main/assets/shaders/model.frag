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

#version 450

// Input from the vertex shader
layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 view_position;
layout(location = 2) in vec3 screen_space_position;
layout(location = 3) in vec3 view_normal;
layout(location = 4) in vec4 object_color;
layout(location = 5) in vec4 lighting_parameters;

// The final output color
layout(location = 0) out vec4 outColor;

// The texture we will sample from
layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2D depthTexture;
layout(binding = 2) uniform FragmentUniformBuffer {
    mat4 depth_uv_transform;
    vec4 color_correction_parameters;
    vec4 material_parameters;
    float depth_aspect_ratio;
    float use_depth_for_occlusion;
} ubo;


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

  // Instead of a hard z-buffer test, allow the asset to fade into the
  // background along a 2 * kDepthTolerancePerMm * asset_depth_mm
  // range centered on the background depth.
  const float kDepthTolerancePerMm = 0.015;
  float visibility_occlusion = clamp(0.5 * (depth_mm - asset_depth_mm) /
    (kDepthTolerancePerMm * asset_depth_mm) + 0.5, 0.0, 1.0);

  // Depth close to zero is most likely invalid, do not use it for occlusions.
  float visibility_depth_near = 1.0 - DepthInverseLerp(
      depth_mm, kMinDepthMm, kMaxDepthMm);

  // Same for very high depth values.
  float visibility_depth_far = DepthInverseLerp(
      depth_mm, kMinFarDepthMm, kMaxFarDepthMm);

  const float kOcclusionAlpha = 0.0;
  float visibility =
      max(max(visibility_occlusion, kOcclusionAlpha),
          max(visibility_depth_near, visibility_depth_far));

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

  const float kOcclusionBlurAmount = 0.007;
  vec2 blurriness = vec2(kOcclusionBlurAmount,
                         kOcclusionBlurAmount * ubo.depth_aspect_ratio);

  float current = 0.0;

  sum += DepthGetVisibility(depth_texture, uv , asset_depth_mm) * 41.0;

  current = 0.0;
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+0.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(-0.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(-1.0, -0.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+1.0, +0.0) * blurriness, asset_depth_mm);
  sum += current * 26.0;

  if (sum > 130) {
   return sum / 145.0; // Early exit if we are probably fully visible.
  }

  if( sum < 15){
    return 0.0; // Early exit if we are probably fully occluded.
  }

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

  current = 0.0;
  current += DepthGetVisibility
  (depth_texture, uv + vec2(-2.0, -0.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+2.0, +0.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+0.0, +2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(-0.0, -2.0) * blurriness, asset_depth_mm);
  sum += current * 7.0;

  current = 0.0;
  current += DepthGetVisibility
  (depth_texture, uv + vec2(-1.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+1.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(-1.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility
  (depth_texture, uv + vec2(+1.0, +1.0) * blurriness, asset_depth_mm);
  sum += current * 16.0;

  return sum / kKernelTotalWeights;
}

void main() {

  // Sample the color from the texture using the coordinates and output it
  float visibility = 1.0;
  if (ubo.use_depth_for_occlusion == 1.0) {
    float asset_depth_mm = view_position.z * -1000.0;
    // Convert from openGL to vulkan coordinate systems.
    vec3 screen_space_position_vec = screen_space_position * .5 + .5;
    // Apply the depth uv transform to get the depth texture coordinates.
    vec2 depth_uv = (mat3(ubo.depth_uv_transform) *
        vec3(screen_space_position_vec.xy, 1.0)).xy;
    visibility = DepthGetBlurredVisibilityAroundUV(
        depthTexture, depth_uv, asset_depth_mm);
  }
  if (visibility != 0.0) {
    // We support approximate sRGB gamma.
    const float kGamma = 0.4545454;
    const float kInverseGamma = 2.2;
    const float kMiddleGrayGamma = 0.466;

    // Unpack lighting and material parameters for better naming.
    vec3 viewLightDirection = lighting_parameters.xyz;
    vec3 colorShift = ubo.color_correction_parameters.rgb;
    float averagePixelIntensity = ubo.color_correction_parameters.a;

    float materialAmbient = ubo.material_parameters.x;
    float materialDiffuse = ubo.material_parameters.y;
    float materialSpecular = ubo.material_parameters.z;
    float materialSpecularPower = ubo.material_parameters.w;

    // Normalize varying parameters, because they are linearly interpolated in the vertex shader.
    vec3 viewFragmentDirection = normalize(view_position);
    vec3 viewNormal = normalize(view_normal);

    // Flip the y-texture coordinate to address the texture from top-left.
    // Sample the texture.
    vec4 textureColor = texture(texSampler, inTexCoord);

    // Tint the texture color with the object_color passed from the vertex shader.
    vec4 coloredTexture = textureColor * object_color;

    // Apply inverse SRGB gamma to the texture before making lighting calculations.
    vec3 linearColor = pow(coloredTexture.rgb, vec3(kInverseGamma));

    // Approximate a hemisphere light (not a harsh directional light).
    float diffuse = materialDiffuse *
            0.5 * (dot(viewNormal, viewLightDirection) + 1.0);

    // Compute specular light. Textures are loaded with with premultiplied alpha
    // (https://developer.android.com/reference/android/graphics/BitmapFactory.Options#inPremultiplied),
    // so premultiply the specular color by alpha as well.
    vec3 reflectedLightDirection = reflect(viewLightDirection, viewNormal);
    float specularStrength = max(0.0, dot(viewFragmentDirection, reflectedLightDirection));
    float specular = coloredTexture.a * materialSpecular *
            pow(specularStrength, materialSpecularPower);

    vec3 color = linearColor * (materialAmbient + diffuse) + specular;
    // Apply SRGB gamma before writing the fragment color.
    color.rgb = pow(color, vec3(kGamma));
    // Apply average pixel intensity and color shift
    color *= colorShift * (averagePixelIntensity / kMiddleGrayGamma);
    outColor.rgb = color;
    outColor.a = coloredTexture.a;
  }

    // Fade out the color based on the visibility.
    outColor *= visibility;
}
