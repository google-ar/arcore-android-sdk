/*
 * Copyright 2017 Google LLC
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

precision highp float;
uniform sampler2D u_Texture;
uniform vec4 u_gridControl;  // dotThreshold, lineThreshold, lineFadeShrink, occlusionShrink
varying vec3 v_TexCoordAlpha;

void main() {
  vec4 control = texture2D(u_Texture, v_TexCoordAlpha.xy);
  float dotScale = v_TexCoordAlpha.z;
  float lineFade = max(0.0, u_gridControl.z * v_TexCoordAlpha.z - (u_gridControl.z - 1.0));
  float alpha = (control.r * dotScale > u_gridControl.x) ? 1.0
              : (control.g > u_gridControl.y)            ? lineFade
                                                         : (0.25 * lineFade);
  gl_FragColor = vec4(alpha * v_TexCoordAlpha.z);
}
