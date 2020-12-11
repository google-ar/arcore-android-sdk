# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Generates a DFG lookup texture, pre-integrated for multiscattering.

This algorithm is implemented according to the method described here:
https://google.github.io/filament/Filament.html#lighting/imagebasedlights
"""
import argparse
import math
import struct
import numpy


def saturate(value):
  return min(max(value, 0.0), 1.0)


def hammersley(i, number_of_samples):
  tof = 0.5 / 0x80000000
  bits = i
  bits = (bits << 16) | numpy.right_shift(bits, 16)
  bits = ((bits & 0x55555555) << 1) | numpy.right_shift(bits & 0xAAAAAAAA, 1)
  bits = ((bits & 0x33333333) << 2) | numpy.right_shift(bits & 0xCCCCCCCC, 2)
  bits = ((bits & 0x0F0F0F0F) << 4) | numpy.right_shift(bits & 0xF0F0F0F0, 4)
  bits = ((bits & 0x00FF00FF) << 8) | numpy.right_shift(bits & 0xFF00FF00, 8)
  return (i / float(number_of_samples), bits * tof)


def hemisphere_importance_sample_dggx(u, a):
  phi = 2.0 * math.pi * u[0]
  # NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
  cos_theta2 = (1.0 - u[1]) / (1.0 + (a + 1.0) * ((a - 1.0) * u[1]))
  cos_theta = math.sqrt(cos_theta2)
  sin_theta = math.sqrt(1.0 - cos_theta2)
  return (sin_theta * math.cos(phi), sin_theta * math.sin(phi), cos_theta)


def visibility(nov, nol, a):
  """Compute visibility using height-correlated GGX.

  Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based
  BRDFs"

  Args:
    nov: Normal dot view direction.
    nol: Normal dot light direction.
    a: Linear roughness.

  Returns:
    The geometric visibility (V) term.
  """
  a2 = a * a
  ggxl = nov * math.sqrt((nol - nol * a2) * nol + a2)
  ggxv = nol * math.sqrt((nov - nov * a2) * nov + a2)
  return 0.5 / (ggxv + ggxl)


def calculate_dfg(nov, a, number_of_samples):
  """Calculate the DFG1 and DFG2 terms in a list of length 2."""
  v = (
      math.sqrt(1.0 - nov * nov),
      0.0,
      nov,
  )

  r = [0.0, 0.0]
  for i in range(number_of_samples):
    u = hammersley(i, number_of_samples)
    h = hemisphere_importance_sample_dggx(u, a)
    l = 2.0 * numpy.multiply(numpy.dot(v, h), h) - v

    voh = saturate(numpy.dot(v, h))
    nol = saturate(l[2])
    noh = saturate(h[2])

    if nol > 0.0:
      vis = visibility(nov, nol, a) * nol * (voh / noh)
      fc = (1.0 - voh)**5.0
      r[0] += vis * fc
      r[1] += vis

  return numpy.multiply(r, 4.0 / float(number_of_samples))


def write_dfg(fp, dfg, file_format):
  assert len(dfg) == 2
  if file_format == 'raw':
    # Write two channels as half-floats
    fp.write(numpy.array(dfg, dtype=numpy.float16).tobytes())
  elif file_format == 'ppm':
    # Write red, green, and blue channels
    fp.write(struct.pack('BBB', *(int(saturate(c) * 255) for c in dfg), 0))


def main():
  parser = argparse.ArgumentParser(
      description='Generate raw DFG textures for environmental HDR.')
  parser.add_argument(
      '-r',
      '--resolution',
      dest='resolution',
      default=64,
      help='Horizontal and vertical resolution of the image.')
  parser.add_argument(
      '-s',
      '--samples',
      dest='samples',
      default=1024,
      type=int,
      help='Number of importance samples.')
  parser.add_argument(
      '-f',
      '--format',
      dest='file_format',
      default='raw',
      choices=['raw', 'ppm'],
      help='Output format.')
  parser.add_argument(
      '-o', '--output', dest='output', required=True, help='output file')

  args = parser.parse_args()

  with open(args.output, 'wb') as fp:
    if args.file_format == 'ppm':
      fp.write(f'P6\n{args.resolution} {args.resolution}\n255\n'.encode())
    for t in range(args.resolution):
      for s in range(args.resolution):
        nov = (s + 0.5) / float(args.resolution)
        perceptual_roughness = (t + 0.5) / float(args.resolution)
        a = perceptual_roughness**2
        dfg = calculate_dfg(nov, a, args.samples)
        write_dfg(fp, dfg, args.file_format)


if __name__ == '__main__':
  main()
