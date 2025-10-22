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

// Input from your C++ vertex buffer
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in mat4 inModelView; // Occupies 4 consecutive locations
layout(location = 7) in vec4 inObjectColor;

// Output to the fragment shader
layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 view_position;
layout(location = 2) out vec3 screen_space_position;
layout(location = 3) out vec3 view_normal;
layout(location = 4) out vec4 object_color;
layout(location = 5) out vec4 lighting_parameters;

// Push constant, updated once per frame.
layout(push_constant) uniform VertexPushConstants {
    mat4 projection_matrix;
} push;

void main() {
    const vec4 kLightDirection = vec4(0.0, 1.0, 0.0, 0.0);

    // Calculate the final position of the vertex on screen
    gl_Position = push.projection_matrix *
        inModelView * vec4(inPosition, 1.0);
    view_position = (inModelView * vec4(inPosition, 1.0)).xyz;
    screen_space_position = gl_Position.xyz / gl_Position.w;
    view_normal = normalize((inModelView * vec4(inNormal, 0.0)).xyz);
    object_color = inObjectColor;
    lighting_parameters = inModelView * kLightDirection;
    lighting_parameters.w = 1.0;

    // Pass the texture coordinate to the fragment shader
    outTexCoord = vec2(inTexCoord.x, 1.0 - inTexCoord.y);
}
