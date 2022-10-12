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

// The number of mipmap levels in the cubemap is equal to the number of
// roughness levels that we precalculate for filtering the cubemap for roughness
// in real-time.
const int kNumberOfRoughnessLevels = NUMBER_OF_MIPMAP_LEVELS;

// The number of importance samples to use for roughness filtering of the
// cubemap.
const int kNumberOfImportanceSamples = NUMBER_OF_IMPORTANCE_SAMPLES;

struct ImportanceSampleCacheEntry {
  vec3 direction;      // Direction to sample in tangent space
  float contribution;  // Weighted contribution of the sample's radiance
  float level;         // The mipmap level to sample from the cubemap. Can be
                       // in-between integer levels for trilinear filtering.
};

struct ImportanceSampleCache {
  int number_of_entries;
  ImportanceSampleCacheEntry entries[kNumberOfImportanceSamples];
};

// This array's length is one less than the number of roughness levels since the
// first roughness level can be skipped.
uniform ImportanceSampleCache
    u_ImportanceSampleCaches[kNumberOfRoughnessLevels - 1];

// The source radiance cubemap to be filtered.
uniform samplerCube u_Cubemap;

// The roughness level that we are filtering for.
uniform int u_RoughnessLevel;

in vec2 v_Position;

#ifdef PX_LOCATION
layout(location = PX_LOCATION) out vec4 o_FragColorPX;
#endif
#ifdef NX_LOCATION
layout(location = NX_LOCATION) out vec4 o_FragColorNX;
#endif
#ifdef PY_LOCATION
layout(location = PY_LOCATION) out vec4 o_FragColorPY;
#endif
#ifdef NY_LOCATION
layout(location = NY_LOCATION) out vec4 o_FragColorNY;
#endif
#ifdef PZ_LOCATION
layout(location = PZ_LOCATION) out vec4 o_FragColorPZ;
#endif
#ifdef NZ_LOCATION
layout(location = NZ_LOCATION) out vec4 o_FragColorNZ;
#endif

vec4 Filter(const vec3 n) {
  if (u_RoughnessLevel == 0) {
    // Roughness level 0 is just a straight copy.
    return vec4(textureLod(u_Cubemap, n, 0.0).rgb, 1.0);
  }

  vec3 up = abs(n.z) < 0.9999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);

  mat3 tangentToWorld;
  tangentToWorld[0] = normalize(cross(up, n));
  tangentToWorld[1] = cross(n, tangentToWorld[0]);
  tangentToWorld[2] = n;

  // TODO(b/243456272): This clamp should not be necessary, but is here due to a
  // driver issue with certain devices.
  ImportanceSampleCache cache = u_ImportanceSampleCaches[max(0, u_RoughnessLevel - 1)];
  vec3 radiance = vec3(0.0);
  for (int i = 0; i < cache.number_of_entries; ++i) {
    ImportanceSampleCacheEntry entry = cache.entries[i];
    radiance +=
        textureLod(u_Cubemap, tangentToWorld * entry.direction, entry.level)
            .rgb *
        entry.contribution;
  }
  return vec4(radiance, 1.0);
}

void main() {
  float u = v_Position.x;
  float v = v_Position.y;
#ifdef PX_LOCATION
  o_FragColorPX = Filter(normalize(vec3(+1, -v, -u)));
#endif
#ifdef NX_LOCATION
  o_FragColorNX = Filter(normalize(vec3(-1, -v, +u)));
#endif
#ifdef PY_LOCATION
  o_FragColorPY = Filter(normalize(vec3(+u, +1, +v)));
#endif
#ifdef NY_LOCATION
  o_FragColorNY = Filter(normalize(vec3(+u, -1, -v)));
#endif
#ifdef PZ_LOCATION
  o_FragColorPZ = Filter(normalize(vec3(+u, -v, +1)));
#endif
#ifdef NZ_LOCATION
  o_FragColorNZ = Filter(normalize(vec3(-u, -v, -1)));
#endif
}
