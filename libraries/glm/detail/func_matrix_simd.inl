/// @ref core
/// @file glm/detail/func_matrix_simd.inl

#if GLM_ARCH & GLM_ARCH_SSE2_BIT

#include "type_mat4x4.hpp"
#include "../geometric.hpp"
#include "../simd/matrix.h"
#include <cstring>

namespace glm{
namespace detail
{
	template<qualifier Q>
	struct compute_matrixCompMult<4, 4, float, Q, true>
	{
		GLM_STATIC_ASSERT(detail::is_aligned<Q>::value, "Specialization requires aligned");

		GLM_FUNC_QUALIFIER static mat<4, 4, float, Q> call(mat<4, 4, float, Q> const& x, mat<4, 4, float, Q> const& y)
		{
			mat<4, 4, float, Q> Result;
			glm_mat4_matrixCompMult(
				*static_cast<glm_vec4 const (*)[4]>(&x[0].data),
				*static_cast<glm_vec4 const (*)[4]>(&y[0].data),
				*static_cast<glm_vec4(*)[4]>(&Result[0].data));
			return Result;
		}
	};

	template<qualifier Q>
	struct compute_transpose<4, 4, float, Q, true>
	{
		GLM_FUNC_QUALIFIER static mat<4, 4, float, Q> call(mat<4, 4, float, Q> const& m)
		{
			mat<4, 4, float, Q> Result;
			glm_mat4_transpose(
				*static_cast<glm_vec4 const (*)[4]>(&m[0].data),
				*static_cast<glm_vec4(*)[4]>(&Result[0].data));
			return Result;
		}
	};

	template<qualifier Q>
	struct compute_determinant<4, 4, float, Q, true>
	{
		GLM_FUNC_QUALIFIER static float call(mat<4, 4, float, Q> const& m)
		{
			return _mm_cvtss_f32(glm_mat4_determinant(*reinterpret_cast<__m128 const(*)[4]>(&m[0].data)));
		}
	};

	template<qualifier Q>
	struct compute_inverse<4, 4, float, Q, true>
	{
		GLM_FUNC_QUALIFIER static mat<4, 4, float, Q> call(mat<4, 4, float, Q> const& m)
		{
			mat<4, 4, float, Q> Result;
			glm_mat4_inverse(*reinterpret_cast<__m128 const(*)[4]>(&m[0].data), *reinterpret_cast<__m128(*)[4]>(&Result[0].data));
			return Result;
		}
	};
}//namespace detail

	template<>
	GLM_FUNC_QUALIFIER mat<4, 4, float, aligned_lowp> outerProduct<4, 4, float, aligned_lowp>(vec<4, float, aligned_lowp> const& c, vec<4, float, aligned_lowp> const& r)
	{
		__m128 NativeResult[4];
		glm_mat4_outerProduct(c.data, r.data, NativeResult);
		mat<4, 4, float, aligned_lowp> Result;
		std::memcpy(&Result[0], &NativeResult[0], sizeof(Result));
		return Result;
	}

	template<>
	GLM_FUNC_QUALIFIER mat<4, 4, float, aligned_mediump> outerProduct<4, 4, float, aligned_mediump>(vec<4, float, aligned_mediump> const& c, vec<4, float, aligned_mediump> const& r)
	{
		__m128 NativeResult[4];
		glm_mat4_outerProduct(c.data, r.data, NativeResult);
		mat<4, 4, float, aligned_mediump> Result;
		std::memcpy(&Result[0], &NativeResult[0], sizeof(Result));
		return Result;
	}

	template<>
	GLM_FUNC_QUALIFIER mat<4, 4, float, aligned_highp> outerProduct<4, 4, float, aligned_highp>(vec<4, float, aligned_highp> const& c, vec<4, float, aligned_highp> const& r)
	{
		__m128 NativeResult[4];
		glm_mat4_outerProduct(c.data, r.data, NativeResult);
		mat<4, 4, float, aligned_highp> Result;
		std::memcpy(&Result[0], &NativeResult[0], sizeof(Result));
		return Result;
	}
}//namespace glm

#endif
