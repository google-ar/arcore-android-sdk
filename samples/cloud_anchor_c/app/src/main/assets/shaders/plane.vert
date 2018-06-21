/*
 * Copyright 2018 Google Inc. All Rights Reserved.
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
uniform mat4 texture_mat;

void main() {
  vec4 position = vec4(vertex.x, 0.0, vertex.y, 1.0);
  gl_Position = mvp * position;
  // Vertex Z value is used as the alpha in this shader.
  v_alpha = vertex.z;
  vec4 world_pos = texture_mat * position;
  // Vector from center pose to this vertex in world frame.
  vec2 xz_vec = vec2(world_pos.x - texture_mat[3].x, world_pos.z - texture_mat[3].z);
  // If (world_pos.y - u_Model[3].y) almost equal to zero, this is a horizontal plane,
  // if it's horizontal plane, we use world frame for texture to make it tied to the world,
  // if it's vertical plane, we map the texture to the vertices on plane in world frame.
  v_textureCoords = abs(world_pos.y - texture_mat[3].y) < 0.0001 ?
                    world_pos.xz :
                    vec2(texture_mat[3].x + length(xz_vec) * sign(xz_vec.x), world_pos.y);
}
