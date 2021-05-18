/*
 * Copyright 2021 Google LLC
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

uniform mat4 u_ModelViewProjection;
uniform float u_PointSize;
uniform float u_ConfidenceThreshold;

attribute vec4 a_Position;
attribute vec3 a_Color;

varying vec4 v_Color;

void main() {
   v_Color = vec4(a_Color, 1.0);
   gl_Position = u_ModelViewProjection * vec4(a_Position.xyz, 1.0);

   // Set w of low confidence points to 0 to hide those points.
   gl_Position.w *= step(u_ConfidenceThreshold, a_Position.w);

   gl_PointSize = u_PointSize;
}
