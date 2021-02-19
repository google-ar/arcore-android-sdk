#version 300 es
/*
 * Copyright 2020 Google LLC
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
precision mediump float;

// This shader will light scenes based on the ARCore's Ambient Intensity mode,
// using a very simple lighting model. As such, this does not support many
// parameters that would normally be available in a modern rendering pipeline.

// The albedo texture.
uniform sampler2D u_AlbedoTexture;

// The diffuse reflection factor of light emitted from the upper hemisphere of
// the light source.
uniform float u_UpperDiffuseIntensity;

// The diffuse reflection factor of light emitted from the lower hemisphere of
// the light source.
uniform float u_LowerDiffuseIntensity;

// The surface's specular reflection intensity.
uniform float u_SpecularIntensity;

// Lower values tend towards larger specular highlights, while higher values
// tend towards smaller specular highlights.
uniform float u_SpecularPower;

// The direction of the light source's upper hemisphere.
uniform vec4 u_ViewLightDirection;

// This must be set to output of LightEstimate.getColorCorrection() to match
// color to the scene.
uniform vec4 u_ColorCorrection;

in vec3 v_ViewPosition;
in vec3 v_ViewNormal;
in vec2 v_TexCoord;

layout(location = 0) out vec4 o_FragColor;

vec3 LinearToSrgb(const vec3 color) {
  return clamp(pow(color, vec3(1.0 / 2.2)), 0.0, 1.0);
}

void main() {
  // Unpack lighting and material parameters for better naming.
  vec3 colorShift = u_ColorCorrection.rgb;
  float averagePixelIntensity = u_ColorCorrection.a;

  // Normalize directional vectors.
  vec3 viewDirection = -normalize(v_ViewPosition);
  vec3 viewNormal = normalize(v_ViewNormal);
  vec3 viewLightDirection = normalize(u_ViewLightDirection.xyz);

  // Addressing from the top-left of the albedo texture, read and convert the
  // SRGB color to linear color space.
  vec4 albedoColor =
      texture(u_AlbedoTexture, vec2(v_TexCoord.x, 1.0 - v_TexCoord.y));

  // Compute diffuse term by approximating a hemisphere light.
  float hemisphereFactor = 0.5 + (dot(viewNormal, viewLightDirection) * 0.5);
  float diffuseTerm = u_UpperDiffuseIntensity * hemisphereFactor +
                      (1.0 - hemisphereFactor) * u_LowerDiffuseIntensity;

  // Compute specular term (Blinn-Phong)
  vec3 halfwayDirection = normalize(viewDirection + viewLightDirection);
  float specularBase = max(0.0, dot(viewNormal, halfwayDirection));
  float specularTerm = u_SpecularIntensity * pow(specularBase, u_SpecularPower);

  // Compute final color, convert back to gamma color space, and apply ARCore
  // color correction.
  const float kMiddleGrayGamma = 0.466;
  vec3 color = LinearToSrgb(albedoColor.rgb * diffuseTerm + specularTerm);
  color *= colorShift * (averagePixelIntensity / kMiddleGrayGamma);

  o_FragColor.rgb = color;
  o_FragColor.a = albedoColor.a;
}
