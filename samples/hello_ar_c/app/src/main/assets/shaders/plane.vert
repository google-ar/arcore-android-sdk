/*
 * Copyright 2018 Google LLC
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
 
precision highp float;
precision highp int;
attribute vec3 vertex;
varying vec2 v_textureCoords;
varying float v_alpha;

uniform mat4 mvp;
uniform mat4 model_mat;
uniform vec3 normal;

void main() {
  // Vertex Z value is used as the alpha in this shader.
  v_alpha = vertex.z;

  vec4 local_pos = vec4(vertex.x, 0.0, vertex.y, 1.0);
  gl_Position = mvp * local_pos;
  vec4 world_pos = model_mat * local_pos;

  // Construct two vectors that are orthogonal to the normal.
  // This arbitrary choice is not co-linear with either horizontal
  // or vertical plane normals.
  const vec3 arbitrary = vec3(1.0, 1.0, 0.0);
  vec3 vec_u = normalize(cross(normal, arbitrary));
  vec3 vec_v = normalize(cross(normal, vec_u));

  // Project vertices in world frame onto vec_u and vec_v.
  v_textureCoords = vec2(
  dot(world_pos.xyz, vec_u), dot(world_pos.xyz, vec_v));
}
