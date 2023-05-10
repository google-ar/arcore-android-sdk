/*
 * Copyright 2022 Google LLC
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
#version 330
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
precision mediump float;

layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 a_TexCoords;
layout (location = 0) out vec2 v_TexCoords;

void main() {
   gl_Position = vec4(a_Position, 0, 1);
   v_TexCoords = a_TexCoords;
}
