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

#version 450 core

// Inputs
layout(location = 0) in vec3 vertex;

// Uniform Buffer, updated once per frame.
layout(binding = 0) uniform Ubo {
    mat4 projection_view_matrix;
    mat4 view_matrix; // Add view matrix
} ubo;

// Push constant, updated once per frame.
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 normal;
} push_const;

// Outputs
layout(location = 0) out vec2 out_uv;
layout(location = 1) out float out_alpha;
layout(location = 2) out vec3 view_position;
layout(location = 3) out vec3 screen_space_position;
layout(location = 4) out float out_view_depth; // Output view space Z

void main() {
    out_alpha = vertex.z;
    vec4 local_pos = vec4(vertex.x, 0.0, vertex.y, 1.0);
    vec4 world_pos = push_const.model * local_pos;
    gl_Position = ubo.projection_view_matrix * world_pos;

    view_position = gl_Position.xyz;
    screen_space_position = gl_Position.xyz / gl_Position.w;

    // Calculate true view space position for depth
    vec4 true_view_pos = ubo.view_matrix * world_pos;
    out_view_depth = true_view_pos.z;

    // This arbitrary choice is not co-linear with either horizontal
    // or vertical plane normals.
    const vec3 arbitrary = vec3(1.0, 1.0, 0.0);
    vec3 vec_u = normalize(cross(push_const.normal, arbitrary));
    vec3 vec_v = normalize(cross(push_const.normal, vec_u));

    // Project vertices in world frame onto vec_u and vec_v.
    out_uv = vec2(dot(world_pos.xyz, vec_u), dot(world_pos.xyz, vec_v));
}
