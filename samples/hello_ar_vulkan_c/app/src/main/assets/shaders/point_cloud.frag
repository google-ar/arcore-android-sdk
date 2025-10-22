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

// Input from the vertex shader.
layout(location = 0) in float inConfidence;

// The final output color for the pixel.
layout(location = 0) out vec4 outColor;

void main() {
    // Cyan color.
    vec3 color = vec3(31.0/255.0, 188.0/255.0, 210.0/255.0);

    // Use the confidence value as the point's alpha (transparency).
    // This makes low-confidence points appear more transparent.
    outColor = vec4(color, inConfidence);
}
