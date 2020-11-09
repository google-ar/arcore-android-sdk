# Description:
#   The GLM (OpenGL Mathematics) library.

load("//third_party/glm:defs.bzl", "COMMON_COPTS")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

# Headers that can be passed to cc_library.hdrs.
filegroup(
    name = "glm_hdrs",
    srcs = glob(
        [
            "glm/**/*.h",
            "glm/**/*.hpp",
        ],
        exclude = [
            "glm/detail/*.hpp",
            "glm/simd/*.h",
        ],
    ),
    compatible_with = ["//buildenv/target:gce"],
)

# All headers, including inlined files, that can be passed to
# cc_library.textual_hdrs.
filegroup(
    name = "glm_all_hdrs",
    srcs = glob([
        "glm/**/*.h",
        "glm/**/*.hpp",
        "glm/**/*.inl",
    ]),
    compatible_with = ["//buildenv/target:gce"],
)

cc_library(
    name = "glm_impl",
    srcs = ["glm/detail/glm.cpp"],
    compatible_with = ["//buildenv/target:gce"],
    copts = COMMON_COPTS + [
        # GLM includes things in a few ways, and by doing this as a copt we
        # avoid polluting the global header paths.
        "-I.",
        "-Ithird_party/glm/latest",
    ],
    defines = ["GLM_ENABLE_EXPERIMENTAL"],
    features = ["-use_header_modules"],  # Incompatible with -std=c++11.
    textual_hdrs = [":glm_all_hdrs"],
    visibility = ["//third_party/glm:__pkg__"],
)

cc_library(
    name = "glm_impl_force_pure",
    srcs = ["glm/detail/glm.cpp"],
    compatible_with = ["//buildenv/target:gce"],
    copts = COMMON_COPTS + [
        # GLM includes things in a few ways, and by doing this as a copt we
        # avoid polluting the global header paths.
        "-I.",
        "-Ithird_party/glm/latest",
    ],
    defines = [
        "GLM_ENABLE_EXPERIMENTAL",
        "GLM_FORCE_PURE",
    ],
    features = ["-use_header_modules"],  # Incompatible with -std=c++11.
    textual_hdrs = [":glm_all_hdrs"],
    visibility = ["//third_party/glm:__pkg__"],
)

# This is not actually a test we care about but can be used to validate that
# GLM compiles successfully.
cc_test(
    name = "core_func_matrix_test",
    srcs = ["test/core/core_func_matrix.cpp"],
    copts = ["-Wno-unused-variable"],
    deps = ["//third_party/glm:glm_system"],
)
