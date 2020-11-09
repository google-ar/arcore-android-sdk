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

// This shader will light scenes based on ARCore's Environmental HDR mode with a
// physically based rendering model.
//
// This shader and all ARCore Java samples do not use the HDR cubemap from
// ARCore
// (https://developers.google.com/ar/develop/java/light-estimation#hdr-cubemap).
// This is reasonable for materials with low glossiness.
//
// If more detailed specular environmental reflections are desired, we would
// filter the cubemap for each roughness level CPU-side and upload the mipmaps
// to a texture. The Filament documentation has excellent documentation on this
// concept:
//
// https://google.github.io/filament/Filament.md.html#lighting/imagebasedlights
//
// When using the HDR Cubemap from ARCore for specular reflections, please note
// that the following equation is true of ARCore's Environmental HDR lighting
// estimation, where E(x) is irradiance of x.
//
// E(spherical harmonics) + E(main light) == E(cubemap)
//
// In order to not duplicate the specular lighting contribution of the main
// light, we must use the following equation, where Lo is total reflected
// radiance (i.e. linear color output), Ld(x) is the reflected diffuse radiance
// of x, and Ls(x) is reflected specular radiance of x.
//
// Lo = Ld(spherical harmonics) + Ld(main light) + Ls(cubemap)
//
// The sample as it is written uses the following equation instead. As you can
// see, the environmental specular component is absent.
//
// Lo = Ld(spherical harmonics) + Ld(main light) + Ls(main light)
//
// See the definitions of Pbr_CalculateMainLightRadiance and
// Pbr_CalculateEnvironmentalRadiance.

// The albedo and roughness/metallic textures.
uniform sampler2D u_AlbedoTexture;
uniform sampler2D u_RoughnessMetallicAmbientOcclusionTexture;

// The intensity of the main directional light.
uniform vec3 u_LightIntensity;

// The direction of the main directional light in view space.
uniform vec4 u_ViewLightDirection;

// The coefficients for the spherical harmonic function which models the diffuse
// irradiance of a distant environmental light for a given surface normal in
// world space. These coefficients must be premultiplied with their
// corresponding spherical harmonics constants. See
// HelloArActivity.updateSphericalHarmonicsCoefficients for more information.
uniform vec3 u_SphericalHarmonicsCoefficients[9];

// Inverse view matrix. Used for converting normals back into world space for
// environmental radiance calculations.
uniform mat4 u_ViewInverse;

// If the current light estimate is valid. Used to short circuit the entire
// shader when the light estimate is not valid.
uniform bool u_LightEstimateIsValid;

struct MaterialParameters {
  vec3 diffuse;
  float roughness;  // non-perceptually linear roughness
  float metallic;
  float ambientOcclusion;
  vec3 f0;  // reflectance
};

struct ShadingParameters {
  // Halfway here refers to halfway between the view and light directions.
  float normalDotView;
  float normalDotHalfway;
  float normalDotLight;
  float viewDotHalfway;
  float oneMinusNormalDotHalfwaySquared;

  // This unit vector is in world space and is used for the environmental
  // lighting math.
  vec3 worldNormalDirection;
};

in vec3 v_ViewPosition;
in vec3 v_ViewNormal;
in vec2 v_TexCoord;

layout(location = 0) out vec4 o_FragColor;

const float kPi = 3.14159265359;

float Pbr_D_GGX(const ShadingParameters shading,
                const MaterialParameters material) {
  // Normal distribution factor, adapted from
  // https://github.com/google/filament/blob/main/shaders/src/brdf.fs#L54
  float roughness = material.roughness;
  float NoH = shading.normalDotHalfway;
  float oneMinusNoHSquared = shading.oneMinusNormalDotHalfwaySquared;

  float a = NoH * roughness;
  float k = roughness / (oneMinusNoHSquared + a * a);
  float d = k * k * (1.0 / kPi);
  return clamp(d, 0.0, 1.0);
}

float Pbr_V_SmithGGXCorrelated_Fast(const ShadingParameters shading,
                                    const MaterialParameters material) {
  // Visibility factor, adapted from
  // https://github.com/google/filament/blob/main/shaders/src/brdf.fs#L115
  //
  // The visibility factor is the combination of the geometry factor and the
  // denominator of the Cook-Torrance BRDF function, i.e. V = G / (4 * NoV *
  // NoL)
  float roughness = material.roughness;
  float NoV = shading.normalDotView;
  float NoL = shading.normalDotLight;
  float v = 0.5 / mix(2.0 * NoL * NoV, NoL + NoV, roughness);
  return clamp(v, 0.0, 1.0);
}

vec3 Pbr_F_Schlick(const ShadingParameters shading,
                   const MaterialParameters material) {
  // Fresnel factor, adapted from
  // https://github.com/google/filament/blob/main/shaders/src/brdf.fs#L146
  vec3 f0 = material.f0;
  float VoH = shading.viewDotHalfway;
  float f = pow(1.0 - VoH, 5.0);
  return f + f0 * (1.0 - f);
}

vec3 Pbr_CalculateMainLightRadiance(const ShadingParameters shading,
                                    const MaterialParameters material,
                                    const vec3 mainLightIntensity) {
  // Lambertian diffuse
  vec3 diffuseTerm = material.diffuse / kPi;

  // Cook-Torrance specular.
  //
  // Note that if we were using the HDR cubemap from ARCore for specular
  // lighting, we would *not* add this contribution. See the note at the top of
  // this file for a more detailed explanation.
  float D = Pbr_D_GGX(shading, material);
  float V = Pbr_V_SmithGGXCorrelated_Fast(shading, material);
  vec3 F = Pbr_F_Schlick(shading, material);

  vec3 specularTerm = D * V * F;

  return (specularTerm + diffuseTerm) * mainLightIntensity *
         shading.normalDotLight;
}

vec3 Pbr_CalculateDiffuseEnvironmentalRadiance(const vec3 normal,
                                               const vec3 coefficients[9]) {
  // See HelloArActivity.updateSphericalHarmonicsCoefficients() for more
  // information about this calculation.
  vec3 radiance = coefficients[0] + coefficients[1] * (normal.y) +
                  coefficients[2] * (normal.z) + coefficients[3] * (normal.x) +
                  coefficients[4] * (normal.y * normal.x) +
                  coefficients[5] * (normal.y * normal.z) +
                  coefficients[6] * (3.0 * normal.z * normal.z - 1.0) +
                  coefficients[7] * (normal.z * normal.x) +
                  coefficients[8] * (normal.x * normal.x - normal.y * normal.y);
  return max(radiance, 0.0);
}

vec3 Pbr_CalculateEnvironmentalRadiance(
    const ShadingParameters shading, const MaterialParameters material,
    const vec3 sphericalHarmonicsCoefficients[9]) {
  // The lambertian diffuse BRDF term (1/pi) is baked into
  // HelloArActivity.sphericalHarmonicsFactors.
  vec3 diffuseTerm =
      Pbr_CalculateDiffuseEnvironmentalRadiance(
          shading.worldNormalDirection, sphericalHarmonicsCoefficients) *
      material.diffuse * material.ambientOcclusion;
  // If we wanted to use ARCore's cubemap, this would be the place to add the
  // specular contribution. See the note at the top of this file for a more
  // detailed explanation.
  vec3 specularTerm = vec3(0.0, 0.0, 0.0);
  return diffuseTerm + specularTerm;
}

void Pbr_CreateShadingParameters(const in vec3 viewNormal,
                                 const in vec3 viewPosition,
                                 const in vec4 viewLightDirection,
                                 const in mat4 viewInverse,
                                 out ShadingParameters shading) {
  vec3 normalDirection = normalize(viewNormal);
  vec3 viewDirection = -normalize(viewPosition);
  vec3 lightDirection = normalize(viewLightDirection.xyz);
  vec3 halfwayDirection = normalize(viewDirection + lightDirection);

  // Clamping the minimum bound yields better results with values less than or
  // equal to 0, which would otherwise cause discontinuity in the geometry
  // factor. Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline
  // for The Order: 1886"
  shading.normalDotView = max(dot(normalDirection, viewDirection), 1e-4);
  shading.normalDotHalfway =
      clamp(dot(normalDirection, halfwayDirection), 0.0, 1.0);
  shading.normalDotLight =
      clamp(dot(normalDirection, lightDirection), 0.0, 1.0);
  shading.viewDotHalfway =
      clamp(dot(viewDirection, halfwayDirection), 0.0, 1.0);

  // The following calculation can be proven as being equivalent to 1-(N.H)^2 by
  // using Lagrange's identity.
  //
  // ||a x b||^2 = ||a||^2 ||b||^2 - (a . b)^2
  //
  // Since we're using unit vectors: ||N x H||^2 = 1 - (N . H)^2
  //
  // We are calculating it in this way to preserve floating point precision.
  vec3 NxH = cross(normalDirection, halfwayDirection);
  shading.oneMinusNormalDotHalfwaySquared = dot(NxH, NxH);

  shading.worldNormalDirection = (viewInverse * vec4(normalDirection, 0.0)).xyz;
}

void Pbr_CreateMaterialParameters(const in vec2 texCoord,
                                  const in sampler2D albedoTexture,
                                  const in sampler2D pbrTexture,
                                  out MaterialParameters material) {
  // Read the material parameters from the textures
  vec3 albedo = texture(albedoTexture, texCoord).rgb;
  vec3 roughnessMetallicAmbientOcclusion = texture(pbrTexture, texCoord).rgb;
  // Roughness inputs are perceptually linear; convert them to regular roughness
  // values. Roughness levels approaching 0 will make specular reflections
  // completely invisible, so cap the lower bound. This value was chosen such
  // that (kMinPerceptualRoughness^4) > 0 in fp16 (i.e. 2^(-14/4), rounded up).
  // https://github.com/google/filament/blob/main/shaders/src/common_material.fs#L2
  const float kMinPerceptualRoughness = 0.089;
  float perceptualRoughness =
      max(roughnessMetallicAmbientOcclusion.r, kMinPerceptualRoughness);
  material.roughness = perceptualRoughness * perceptualRoughness;
  material.metallic = roughnessMetallicAmbientOcclusion.g;
  material.ambientOcclusion = roughnessMetallicAmbientOcclusion.b;

  material.diffuse = albedo * (1.0 - material.metallic);
  // F0 is defined as "Fresnel reflectance at 0 degrees", i.e. specular
  // reflectance when light is grazing a surface perfectly perpendicularly. This
  // value is derived from the index of refraction for a material. Most
  // dielectric materials have an F0 value of 0.00-0.08, which leaves 0.04 as a
  // reasonable constant for a simple roughness/metallic material workflow as
  // implemented by this shader.
  material.f0 = mix(vec3(0.04), albedo, material.metallic);
}

vec3 LinearToSrgb(const vec3 color) {
  vec3 kGamma = vec3(1.0 / 2.2);
  return clamp(pow(color, kGamma), 0.0, 1.0);
}

void main() {
  // Mirror texture coordinates over the X axis
  vec2 texCoord = vec2(v_TexCoord.x, 1.0 - v_TexCoord.y);

  // Skip all lighting calculations if the estimation is not valid.
  if (!u_LightEstimateIsValid) {
    o_FragColor = vec4(texture(u_AlbedoTexture, texCoord).rgb, 1.0);
    return;
  }

  ShadingParameters shading;
  Pbr_CreateShadingParameters(v_ViewNormal, v_ViewPosition,
                              u_ViewLightDirection, u_ViewInverse, shading);

  MaterialParameters material;
  Pbr_CreateMaterialParameters(texCoord, u_AlbedoTexture,
                               u_RoughnessMetallicAmbientOcclusionTexture,
                               material);

  // Combine the radiance contributions of both the main light and environment
  vec3 mainLightRadiance =
      Pbr_CalculateMainLightRadiance(shading, material, u_LightIntensity);

  vec3 environmentalRadiance = Pbr_CalculateEnvironmentalRadiance(
      shading, material, u_SphericalHarmonicsCoefficients);

  vec3 radiance = mainLightRadiance + environmentalRadiance;

  // Convert final color to sRGB color space
  o_FragColor = vec4(LinearToSrgb(radiance), 1.0);
}
