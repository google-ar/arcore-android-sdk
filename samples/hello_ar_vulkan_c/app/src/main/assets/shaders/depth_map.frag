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

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;

// Bindings for the two textures.
layout(binding = 0) uniform sampler2D depthTexture;
layout(binding = 1) uniform sampler2D colorPalette;

// Constants for mapping depth values to the color palette.
const float kMidDepthMeters = 8.0;
const float kMaxDepthMeters = 30.0;
TODO(b/436580516) : Don't harcode the constants.

// Maps a value from [min_bound, max_bound] to [0.0, 1.0].
float inverseLerp(float value, float min_bound, float max_bound) {
  return clamp((value - min_bound) / (max_bound - min_bound), 0.0, 1.0);
}

void main() {
    // Sample the raw 16-bit depth value (in millimeters).
    // The .r component contains the value from the R16_UNORM texture.
    // We multiply by 65535.0 to convert back to millimeters.
    float depth_mm = texture(depthTexture, v_TexCoord).r * 65535.0;
    float depth_meters = depth_mm / 1000.0; // Convert to meters.

    // Map close values to the first half of the color palette
    // far values to the second half.
    float normalized_depth = 0.0;
    if (depth_meters < kMidDepthMeters) {
        normalized_depth =
            inverseLerp(depth_meters, 0.0, kMidDepthMeters) * 0.5;
    } else {
        normalized_depth =
            inverseLerp(depth_meters, kMidDepthMeters, kMaxDepthMeters) * 0.5
            + 0.5;
    }

    // Sample the color from the palette texture.
    vec3 depth_viz_color = texture(colorPalette,
        vec2(normalized_depth, 0.5)).rgb;

    // If depth is 0 (invalid), make the color black.
    depth_viz_color *= sign(depth_mm);

    // Output the final depth visualization color.
    outColor = vec4(depth_viz_color, 1.0);
}
