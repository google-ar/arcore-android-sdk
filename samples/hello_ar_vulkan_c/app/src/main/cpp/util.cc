/*
 * Copyright 2017 Google LLC
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
#include "util.h"

#include <unistd.h>

#include <sstream>
#include <string>

#include "jni_interface.h"

namespace simple_vulkan {
namespace util {

void ThrowJavaException(JNIEnv* env, const char* msg) {
  LOGE("Throw Java exception: %s", msg);
  jclass c = env->FindClass("java/lang/RuntimeException");
  env->ThrowNew(c, msg);
}

bool LoadTextFileFromAssetManager(const char* file_name,
                                  AAssetManager* asset_manager,
                                  std::string* out_file_text_string) {
  // If the file hasn't been uncompressed, load it to the internal storage.
  // Note that AAsset_openFileDescriptor doesn't support compressed
  // files (.obj).
  AAsset* asset =
      AAssetManager_open(asset_manager, file_name, AASSET_MODE_STREAMING);
  if (asset == nullptr) {
    LOGE("Error opening asset %s", file_name);
    return false;
  }

  off_t file_size = AAsset_getLength(asset);
  out_file_text_string->resize(file_size);
  int ret = AAsset_read(asset, &out_file_text_string->front(), file_size);

  if (ret <= 0) {
    LOGE("Failed to open file: %s", file_name);
    AAsset_close(asset);
    return false;
  }

  AAsset_close(asset);
  return true;
}

bool LoadPngFromAssetManager(int target, const char* path) {
  JNIEnv* env = GetJniEnv();

  // Put all the JNI values in a structure that is statically initialized on the
  // first call to this method.  This makes it thread safe in the unlikely case
  // of multiple threads calling this method.
  static struct JNIData {
    jclass helper_class;
    jmethodID load_image_method;
    jmethodID load_texture_method;
  } jniIds = [env]() -> JNIData {
    constexpr char kHelperClassName[] =
        "com/google/ar/core/examples/c/simplevulkan/JniInterface";
    constexpr char kLoadImageMethodName[] = "loadImage";
    constexpr char kLoadImageMethodSignature[] =
        "(Ljava/lang/String;)Landroid/graphics/Bitmap;";
    constexpr char kLoadTextureMethodName[] = "loadTexture";
    constexpr char kLoadTextureMethodSignature[] =
        "(ILandroid/graphics/Bitmap;)V";
    jclass helper_class = FindClass(kHelperClassName);
    if (helper_class) {
      helper_class = static_cast<jclass>(env->NewGlobalRef(helper_class));
      jmethodID load_image_method = env->GetStaticMethodID(
          helper_class, kLoadImageMethodName, kLoadImageMethodSignature);
      jmethodID load_texture_method = env->GetStaticMethodID(
          helper_class, kLoadTextureMethodName, kLoadTextureMethodSignature);
      return {helper_class, load_image_method, load_texture_method};
    }
    LOGE("simple_vulkan::util::Could not find Java helper class %s",
         kHelperClassName);
    return {};
  }();

  if (!jniIds.helper_class) {
    return false;
  }

  jstring j_path = env->NewStringUTF(path);

  jobject image_obj = env->CallStaticObjectMethod(
      jniIds.helper_class, jniIds.load_image_method, j_path);

  if (j_path) {
    env->DeleteLocalRef(j_path);
  }

  env->CallStaticVoidMethod(jniIds.helper_class, jniIds.load_texture_method,
                            target, image_obj);
  return true;
}

void Log4x4Matrix(const float raw_matrix[16]) {
  LOGI(
      "%f, %f, %f, %f\n"
      "%f, %f, %f, %f\n"
      "%f, %f, %f, %f\n"
      "%f, %f, %f, %f\n",
      raw_matrix[0], raw_matrix[1], raw_matrix[2], raw_matrix[3], raw_matrix[4],
      raw_matrix[5], raw_matrix[6], raw_matrix[7], raw_matrix[8], raw_matrix[9],
      raw_matrix[10], raw_matrix[11], raw_matrix[12], raw_matrix[13],
      raw_matrix[14], raw_matrix[15]);
}

}  // namespace util
}  // namespace simple_vulkan
