#version 310 es
/*
 * Copyright 2023 Google LLC
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
uniform sampler2D uSemanticImage;
uniform sampler2D uSemanticColorMap;

in vec2 vTextureCoord;
out vec4 oFragColor;

const float kNumberOfLabels = 12.0;

void main() {
  // Get pixel value from semantic image.
  float pixel_value = texture(uSemanticImage, vTextureCoord.xy).r;

  // Convert pixel value to semantic class id.
  float semantic_label = pixel_value * 255.0;

  // Find normalized position of semantic class id in colormap image.
  // Adding 0.5 puts position in the middle of a color swatch.
  float colormap_position = (semantic_label + 0.5) / kNumberOfLabels;

  // Grab the rgb values from the colormap.
  vec3 color = texture(uSemanticColorMap, vec2(colormap_position, 0.5)).rgb;

  // Set alpha value to allow some transparency.
  vec4 semantic_color = vec4(color, 0.8);

  oFragColor = semantic_color;
}
