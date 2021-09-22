#version 300 es
/*
 * Copyright 2020 Google LLC
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

// The virtual scene as rendered to a texture via a framebuffer. This will be
// composed with the background image depending on which modes were set in
// DepthCompositionRenderer.setDepthModes.
uniform sampler2D u_VirtualSceneColorTexture;

#if USE_OCCLUSION
// The AR camera depth texture.
uniform sampler2D u_CameraDepthTexture;
// The depth texture for the virtual scene.
uniform sampler2D u_VirtualSceneDepthTexture;
// The near and far clipping planes, used to transform the virtual scene depth
// back into view space to compare with the camera depth texture.
uniform float u_ZNear;
uniform float u_ZFar;
// The aspect ratio of the screen. This is used during to create uniform
// blurring for occluded objects.
uniform float u_DepthAspectRatio;
#endif  // USE_OCCLUSION

#if USE_OCCLUSION
in vec2 v_CameraTexCoord;
#endif  // USE_OCCLUSION
in vec2 v_VirtualSceneTexCoord;

layout(location = 0) out vec4 o_FragColor;

#if USE_OCCLUSION

float Depth_GetCameraDepthInMillimeters(const sampler2D depthTexture,
                                        const vec2 depthUv) {
  // Depth is packed into the red and green components of its texture.
  // The texture is a normalized format, storing millimeters.
  vec3 packedDepthAndVisibility = texture(depthTexture, depthUv).xyz;
  return dot(packedDepthAndVisibility.xy, vec2(255.0, 256.0 * 255.0));
}

float Depth_GetVirtualSceneDepthMillimeters(const sampler2D depthTexture,
                                            const vec2 depthUv, float zNear,
                                            float zFar) {
  // Determine the depth of the virtual scene fragment in millimeters.
  const float kMetersToMillimeters = 1000.0;
  // This value was empirically chosen to correct errors with objects appearing
  // to phase through the floor. In millimeters.
  const float kBias = -80.0;
  float ndc = 2.0 * texture(depthTexture, depthUv).x - 1.0;
  return 2.0 * zNear * zFar / (zFar + zNear - ndc * (zFar - zNear)) *
             kMetersToMillimeters +
         kBias;
}

// Returns a value between 0.0 (completely visible) and 1.0 (completely
// occluded), representing how visible or occluded is the pixel in relation to
// the depth map.
float Depth_GetOcclusion(const sampler2D depthTexture, const vec2 depthUv,
                         float assetDepthMm) {
  float depthMm = Depth_GetCameraDepthInMillimeters(depthTexture, depthUv);

  // Instead of a hard z-buffer test, allow the asset to fade into the
  // background along a 2 * kDepthTolerancePerMm * assetDepthMm
  // range centered on the background depth.
  const float kDepthTolerancePerMm = 0.01;
  return clamp(1.0 -
                   0.5 * (depthMm - assetDepthMm) /
                       (kDepthTolerancePerMm * assetDepthMm) +
                   0.5,
               0.0, 1.0);
}

float Depth_GetBlurredOcclusionAroundUV(const sampler2D depthTexture,
                                        const vec2 uv, float assetDepthMm) {
  // Kernel used:
  // 0   4   7   4   0
  // 4   16  26  16  4
  // 7   26  41  26  7
  // 4   16  26  16  4
  // 0   4   7   4   0
  const float kKernelTotalWeights = 269.0;
  float sum = 0.0;

  const float kOcclusionBlurAmount = 0.01;
  vec2 blurriness =
      vec2(kOcclusionBlurAmount, kOcclusionBlurAmount * u_DepthAspectRatio);

  float current = 0.0;

  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-1.0, -2.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+1.0, -2.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-1.0, +2.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+1.0, +2.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-2.0, +1.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+2.0, +1.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-2.0, -1.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+2.0, -1.0) * blurriness, assetDepthMm);
  sum += current * 4.0;

  current = 0.0;
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-2.0, -0.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+2.0, +0.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+0.0, +2.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-0.0, -2.0) * blurriness, assetDepthMm);
  sum += current * 7.0;

  current = 0.0;
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-1.0, -1.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+1.0, -1.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-1.0, +1.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+1.0, +1.0) * blurriness, assetDepthMm);
  sum += current * 16.0;

  current = 0.0;
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+0.0, +1.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-0.0, -1.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(-1.0, -0.0) * blurriness, assetDepthMm);
  current += Depth_GetOcclusion(
      depthTexture, uv + vec2(+1.0, +0.0) * blurriness, assetDepthMm);
  sum += current * 26.0;

  sum += Depth_GetOcclusion(depthTexture, uv, assetDepthMm) * 41.0;

  return sum / kKernelTotalWeights;
}
#endif  // USE_OCCLUSION

void main() {
  o_FragColor = texture(u_VirtualSceneColorTexture, v_VirtualSceneTexCoord);

#if USE_OCCLUSION
  if (o_FragColor.a == 0.0) {
    // There's no sense in calculating occlusion for a fully transparent pixel.
    return;
  }
  float assetDepthMm = Depth_GetVirtualSceneDepthMillimeters(
      u_VirtualSceneDepthTexture, v_VirtualSceneTexCoord, u_ZNear, u_ZFar);

  float occlusion = Depth_GetBlurredOcclusionAroundUV(
      u_CameraDepthTexture, v_CameraTexCoord, assetDepthMm);

  // If the above blur operation is too expensive, you can replace it with the
  // following lines.
  /* float occlusion = Depth_GetOcclusion(u_CameraDepthTexture,
    v_CameraTexCoord, assetDepthMm); */

  // The virtual object mask is blurred, we make the falloff steeper to simulate
  // erosion operator. This is needed to make the fully occluded virtual object
  // invisible.
  float objectMaskEroded = pow(occlusion, 10.0);

  // occlusionTransition equal to 1 means fully occluded object. This operation
  // boosts occlusion near the edges of the virtual object, but does not affect
  // occlusion within the object.
  float occlusionTransition =
      clamp(occlusion * (2.0 - objectMaskEroded), 0.0, 1.0);

  // Clips occlusion if we want to partially show fully occluded object.
  float kMaxOcclusion = 1.0;
  occlusionTransition = min(occlusionTransition, kMaxOcclusion);

  o_FragColor *= 1.0 - occlusion;

#endif  // USE_OCCLUSION
}
