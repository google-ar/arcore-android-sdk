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

// Input vec4 contains {X, Y, Z, Confidence}.
layout(location = 0) in vec4 inPosition;

// The combined View-Projection matrix, sent via a push constant.
layout(push_constant) uniform PushConstants {
    mat4 ViewProjectionMatrix;
} push;

// Output the confidence value to the fragment shader.
layout(location = 0) out float outConfidence;

void main() {
    // Calculate the final screen position of the point.
    // We use .xyz because the 4th component is confidence, not a coordinate.
    gl_Position = push.ViewProjectionMatrix * vec4(inPosition.xyz, 1.0);

    // Set a fixed size for the points.
    gl_PointSize = 5.0;

    // Pass the confidence value in the .w component to the fragment shader.
    outConfidence = inPosition.w;
}
