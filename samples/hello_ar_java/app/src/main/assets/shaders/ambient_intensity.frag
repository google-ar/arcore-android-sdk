#version 300 es
/*
 * Copyright 2017 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
precision mediump float;

// This shader will light scenes based on the estimations of ARCore's Ambient
// Intensity mode, using a very simple lighting model. As such, this does not
// support many parameters that would normally be available in a modern
// rendering pipeline.

// The albedo texture.
uniform sampler2D u_AlbedoTexture;

// A color to be multiplied with the albedo before any lighting operations.
uniform vec3 u_AlbedoColor;

// The diffuse reflection factor of light emitted from the upper hemisphere of
// the light source.
uniform float u_UpperDiffuseIntensity;

// The diffuse reflection factor of light emitted from the lower hemisphere of
// the light source.
uniform float u_LowerDiffuseIntensity;

// The surface's specular reflection intensity.
uniform float u_SpecularIntensity;

// Lower values tend towards larger specular highlights, while higher values
// tend towards smaller specular highlights.
uniform float u_SpecularPower;

// The direction of the light source's upper hemisphere.
uniform vec4 u_ViewLightDirection;

// This must be set to output of LightEstimate.getColorCorrection() to match
// color to the scene.
uniform vec4 u_ColorCorrection;

#if USE_DEPTH_FOR_OCCLUSION
uniform sampler2D u_DepthTexture;
uniform mat3 u_DepthUvTransform;
uniform float u_DepthAspectRatio;
#endif

in vec3 v_ViewPosition;
in vec3 v_ViewNormal;
in vec2 v_TexCoord;
in vec3 v_ScreenSpacePosition;

layout(location = 0) out vec4 o_FragColor;

#if USE_DEPTH_FOR_OCCLUSION

float DepthGetMillimeters(in sampler2D depth_texture, in vec2 depth_uv) {
  // Depth is packed into the red and green components of its texture.
  // The texture is a normalized format, storing millimeters.
  vec3 packedDepthAndVisibility = texture(depth_texture, depth_uv).xyz;
  return dot(packedDepthAndVisibility.xy, vec2(255.0, 256.0 * 255.0));
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
  float depth_mm = DepthGetMillimeters(depth_texture, depth_uv);

  // Instead of a hard z-buffer test, allow the asset to fade into the
  // background along a 2 * kDepthTolerancePerMm * asset_depth_mm
  // range centered on the background depth.
  const float kDepthTolerancePerMm = 0.015;
  float visibility_occlusion = clamp(0.5 * (depth_mm - asset_depth_mm) /
    (kDepthTolerancePerMm * asset_depth_mm) + 0.5, 0.0, 1.0);

  // Depth close to zero is most likely invalid, do not use it for occlusions.
  float visibility_depth_near = 1.0 - DepthInverseLerp(
      depth_mm, /*min_depth_mm=*/150.0, /*max_depth_mm=*/200.0);

  // Same for very high depth values.
  float visibility_depth_far = DepthInverseLerp(
      depth_mm, /*min_depth_mm=*/7500.0, /*max_depth_mm=*/8000.0);

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

  const float kOcclusionBlurAmount = 0.01;
  vec2 blurriness = vec2(kOcclusionBlurAmount,
                         kOcclusionBlurAmount * u_DepthAspectRatio);

  float current = 0.0;

  current += DepthGetVisibility(depth_texture, uv + vec2(-1.0, -2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+1.0, -2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-1.0, +2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+1.0, +2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-2.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+2.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-2.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+2.0, -1.0) * blurriness, asset_depth_mm);
  sum += current * 4.0;

  current = 0.0;
  current += DepthGetVisibility(depth_texture, uv + vec2(-2.0, -0.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+2.0, +0.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+0.0, +2.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-0.0, -2.0) * blurriness, asset_depth_mm);
  sum += current * 7.0;

  current = 0.0;
  current += DepthGetVisibility(depth_texture, uv + vec2(-1.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+1.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-1.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+1.0, +1.0) * blurriness, asset_depth_mm);
  sum += current * 16.0;

  current = 0.0;
  current += DepthGetVisibility(depth_texture, uv + vec2(+0.0, +1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-0.0, -1.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(-1.0, -0.0) * blurriness, asset_depth_mm);
  current += DepthGetVisibility(depth_texture, uv + vec2(+1.0, +0.0) * blurriness, asset_depth_mm);
  sum += current * 26.0;

  sum += DepthGetVisibility(depth_texture, uv , asset_depth_mm) * 41.0;

  return sum / kKernelTotalWeights;
}

#endif

void main() {
  const float kGamma = 0.4545454;
  const float kInverseGamma = 2.2;
  const float kMiddleGrayGamma = 0.466;

  // Unpack lighting and material parameters for better naming.
  vec3 colorShift = u_ColorCorrection.rgb;
  float averagePixelIntensity = u_ColorCorrection.a;

  // Normalize directional vectors.
  vec3 viewDirection = -normalize(v_ViewPosition);
  vec3 viewNormal = normalize(v_ViewNormal);
  vec3 viewLightDirection = normalize(u_ViewLightDirection.xyz);

  // Addressing from the top-left of the albedo texture, read and convert the
  // SRGB color to linear color space.
  vec4 albedoColor = texture(u_AlbedoTexture, vec2(v_TexCoord.x, 1.0 - v_TexCoord.y));
  albedoColor.rgb *= u_AlbedoColor;
  albedoColor.rgb = pow(albedoColor.rgb, vec3(kInverseGamma));

  // Compute diffuse term by approximating a hemisphere light.
  float hemisphereFactor = 0.5 + (dot(viewNormal, viewLightDirection) * 0.5);
  float diffuseTerm = u_UpperDiffuseIntensity * hemisphereFactor + (1.0 - hemisphereFactor) * u_LowerDiffuseIntensity;

  // Compute specular term (Blinn-Phong)
  vec3 halfwayDirection = normalize(viewDirection + viewLightDirection);
  float specularBase = max(0.0, dot(viewNormal, halfwayDirection));
  float specularTerm = u_SpecularIntensity * pow(specularBase, u_SpecularPower);

  // Compute final color, convert back to gamma color space, and apply ARCore
  // color correction.
  vec3 color = albedoColor.rgb * diffuseTerm + specularTerm;
  color.rgb = pow(color, vec3(kGamma));
  color *= colorShift * (averagePixelIntensity / kMiddleGrayGamma);

  o_FragColor.rgb = color;
  o_FragColor.a = albedoColor.a;

  #if USE_DEPTH_FOR_OCCLUSION
  const float kMetersToMillimeters = 1000.0;
  float asset_depth_mm = v_ViewPosition.z * kMetersToMillimeters * -1.;
  // Computes the texture coordinates to sample from the depth image.
  vec2 depth_uvs = (u_DepthUvTransform * vec3(v_ScreenSpacePosition.xy, 1)).xy;

  // The following step is very costly. Replace the last line with the
  // commented line if it's too expensive.
  // o_FragColor *= DepthGetVisibility(u_DepthTexture, depth_uvs, asset_depth_mm);
  o_FragColor *= DepthGetBlurredVisibilityAroundUV(u_DepthTexture, depth_uvs, asset_depth_mm);
  #endif
}
