#ifndef THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOAR_CPP_TEXTURE_H_
#define THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOAR_CPP_TEXTURE_H_

#include "arcore_c_api.h"

namespace hello_ar {

/**
 * Handle the creation and update of a GPU texture.
 **/
class Texture {
 public:
  Texture() = default;
  ~Texture() = default;

  void CreateOnGlThread();
  void UpdateWithDepthImageOnGlThread(const ArSession& session,
                                      const ArFrame& frame);
  unsigned int GetTextureId() { return texture_id_; }

  unsigned int GetWidth() { return width_; }

  unsigned int GetHeight() { return height_; }

 private:
  unsigned int texture_id_ = 0;
  unsigned int width_ = 1;
  unsigned int height_ = 1;
};
}  // namespace hello_ar

#endif  // THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOAR_CPP_TEXTURE_H_
