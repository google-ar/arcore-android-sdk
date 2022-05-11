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

uniform sampler2D u_DepthTexture;
uniform sampler2D u_ColorMap;

varying vec2 v_TexCoord;

const float kMidDepthMeters = 8.0;
const float kMaxDepthMeters = 30.0;

float DepthGetMillimeters(in sampler2D depth_texture, in vec2 depth_uv) {
  // Depth is packed into the red and green components of its texture.
  // The texture is a normalized format, storing millimeters.
  vec3 packedDepthAndVisibility = texture2D(depth_texture, depth_uv).xyz;
  return dot(packedDepthAndVisibility.xy, vec2(255.0, 256.0 * 255.0));
}

// Returns linear interpolation position of value between min and max bounds.
// E.g. InverseLerp(1100, 1000, 2000) returns 0.1.
float InverseLerp(float value, float min_bound, float max_bound) {
  return clamp((value - min_bound) / (max_bound - min_bound), 0.0, 1.0);
}

// Returns a color corresponding to the depth passed in.
// The input x is normalized in range 0 to 1.
vec3 DepthGetColorVisualization(in float x) {
  return texture2D(u_ColorMap, vec2(x, 0.5)).rgb;
}

void main() {
  // Interpolating in units of meters is more stable, due to limited floating
  // point precision on GPU.
  float depth_mm = DepthGetMillimeters(u_DepthTexture, v_TexCoord.xy);
  float depth_meters = depth_mm * 0.001;

  // Selects the portion of the color palette to use.
  float normalized_depth = 0.0;
  if (depth_meters < kMidDepthMeters) {
    // Short-range depth (0m to 8m) maps to first half of the color palette.
    normalized_depth = InverseLerp(depth_meters, 0.0, kMidDepthMeters) * 0.5;
  } else {
    // Long-range depth (8m to 30m) maps to second half of the color palette.
    normalized_depth =
        InverseLerp(depth_meters, kMidDepthMeters, kMaxDepthMeters) * 0.5 + 0.5;
  }

  // Converts depth to color by with the selected value in the color map.
  vec4 depth_color = vec4(DepthGetColorVisualization(normalized_depth), 1.0);

  // Invalid depth (pixels with value 0) mapped to black.
  depth_color.rgb *= sign(depth_meters);
  gl_FragColor = depth_color;
}
