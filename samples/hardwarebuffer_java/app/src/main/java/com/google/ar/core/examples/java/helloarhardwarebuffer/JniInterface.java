/*
 * Copyright 2019 Google LLC
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
package com.google.ar.core.examples.java.helloarhardwarebuffer;

/** JNI interface to native layer. */
public class JniInterface {
  static {
    System.loadLibrary("hello_ar_hardwarebuffer_native");
  }

  private static final String TAG = "JniInterface";

  public static native long createEglImage(Object buffer);

  public static native void destroyEglImage(long imageAddress);

  public static native void bindEglImageToTexture(long imageAddress, long textureId);
}
