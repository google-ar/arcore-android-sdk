/*
 * Copyright 2018 Google Inc. All Rights Reserved.
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
#extension GL_OES_EGL_image_external : require

precision mediump float;
varying vec2 v_TexCoord;
varying vec2 v_ImgCoord;
uniform float s_SplitterPosition;
uniform samplerExternalOES TexVideo;
uniform sampler2D TexCpuImageGrayscale;

void main() {
  if (v_TexCoord.x < s_SplitterPosition)
  {
    gl_FragColor = texture2D(TexVideo, v_TexCoord);
  }
  else
  {
    gl_FragColor.xyz = texture2D(TexCpuImageGrayscale, v_ImgCoord).rrr;
    gl_FragColor.a = 1.0;
  }
}
