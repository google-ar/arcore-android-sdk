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

// This shader pair shows the depth estimation instead of the camera image as
// the background. This behavior is mostly only useful as a demonstration of the
// depth feature.

uniform sampler2D u_CameraDepthTexture;
uniform sampler2D u_ColorMap;

in vec2 v_CameraTexCoord;

layout(location = 0) out vec4 o_FragColor;

float Depth_GetCameraDepthInMillimeters(const sampler2D depthTexture,
                                        const vec2 depthUv) {
  // Depth is packed into the red and green components of its texture.
  // The texture is a normalized format, storing millimeters.
  vec3 packedDepthAndVisibility = texture(depthTexture, depthUv).xyz;
  return dot(packedDepthAndVisibility.xy, vec2(255.0, 256.0 * 255.0));
}

// Returns a color corresponding to the depth passed in.
//
// Uses Turbo color mapping:
// https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
//
// The input x is normalized in range 0 to 1.
vec3 Depth_GetColorVisualization(float x) {
  return texture(u_ColorMap, vec2(x, 0.5)).rgb;
}

// Returns linear interpolation position of value between min and max bounds.
// E.g. InverseLerp(1100, 1000, 2000) returns 0.1.
float InverseLerp(float value, float min_bound, float max_bound) {
  return clamp((value - min_bound) / (max_bound - min_bound), 0.0, 1.0);
}

void main() {
  const float kMidDepthMeters = 8.0;
  const float kMaxDepthMeters = 30.0;

  // Interpolating in units of meters is more stable, due to limited floating
  // point precision on GPU.
  float depth_mm =
      Depth_GetCameraDepthInMillimeters(u_CameraDepthTexture, v_CameraTexCoord);
  float depth_meters = depth_mm * 0.001;

  // Selects the portion of the color palette to use.
  float normalizedDepth = 0.0;
  if (depth_meters < kMidDepthMeters) {
    // Short-range depth (0m to 8m) maps to first half of the color palette.
    normalizedDepth = InverseLerp(depth_meters, 0.0, kMidDepthMeters) * 0.5;
  } else {
    // Long-range depth (8m to 30m) maps to second half of the color palette.
    normalizedDepth =
        InverseLerp(depth_meters, kMidDepthMeters, kMaxDepthMeters) * 0.5 + 0.5;
  }

  // Converts depth to color by with the selected value in the color map.
  vec4 depth_color = vec4(Depth_GetColorVisualization(normalizedDepth), 1.0);

  // Invalid depth (pixels with value 0) mapped to black.
  depth_color.rgb *= sign(depth_meters);
  o_FragColor = depth_color;
}
