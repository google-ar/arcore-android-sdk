/// @ref core
/// @file glm/detail/_vectorize.hpp

#pragma once

#include "type_vec1.hpp"
#include "type_vec2.hpp"
#include "type_vec3.hpp"
#include "type_vec4.hpp"

namespace glm{
namespace detail
{
	template<length_t L, typename R, typename T, qualifier Q>
	struct functor1{};

	template<typename R, typename T, qualifier Q>
	struct functor1<1, R, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<1, R, Q> call(R (*Func) (T x), vec<1, T, Q> const& v)
		{
			return vec<1, R, Q>(Func(v.x));
		}
	};

	template<typename R, typename T, qualifier Q>
	struct functor1<2, R, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<2, R, Q> call(R (*Func) (T x), vec<2, T, Q> const& v)
		{
			return vec<2, R, Q>(Func(v.x), Func(v.y));
		}
	};

	template<typename R, typename T, qualifier Q>
	struct functor1<3, R, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<3, R, Q> call(R (*Func) (T x), vec<3, T, Q> const& v)
		{
			return vec<3, R, Q>(Func(v.x), Func(v.y), Func(v.z));
		}
	};

	template<typename R, typename T, qualifier Q>
	struct functor1<4, R, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<4, R, Q> call(R (*Func) (T x), vec<4, T, Q> const& v)
		{
			return vec<4, R, Q>(Func(v.x), Func(v.y), Func(v.z), Func(v.w));
		}
	};

	template<length_t L, typename T, qualifier Q>
	struct functor2{};

	template<typename T, qualifier Q>
	struct functor2<1, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<1, T, Q> call(T (*Func) (T x, T y), vec<1, T, Q> const& a, vec<1, T, Q> const& b)
		{
			return vec<1, T, Q>(Func(a.x, b.x));
		}
	};

	template<typename T, qualifier Q>
	struct functor2<2, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<2, T, Q> call(T (*Func) (T x, T y), vec<2, T, Q> const& a, vec<2, T, Q> const& b)
		{
			return vec<2, T, Q>(Func(a.x, b.x), Func(a.y, b.y));
		}
	};

	template<typename T, qualifier Q>
	struct functor2<3, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<3, T, Q> call(T (*Func) (T x, T y), vec<3, T, Q> const& a, vec<3, T, Q> const& b)
		{
			return vec<3, T, Q>(Func(a.x, b.x), Func(a.y, b.y), Func(a.z, b.z));
		}
	};

	template<typename T, qualifier Q>
	struct functor2<4, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<4, T, Q> call(T (*Func) (T x, T y), vec<4, T, Q> const& a, vec<4, T, Q> const& b)
		{
			return vec<4, T, Q>(Func(a.x, b.x), Func(a.y, b.y), Func(a.z, b.z), Func(a.w, b.w));
		}
	};

	template<length_t L, typename T, qualifier Q>
	struct functor2_vec_sca{};

	template<typename T, qualifier Q>
	struct functor2_vec_sca<1, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<1, T, Q> call(T (*Func) (T x, T y), vec<1, T, Q> const& a, T b)
		{
			return vec<1, T, Q>(Func(a.x, b));
		}
	};

	template<typename T, qualifier Q>
	struct functor2_vec_sca<2, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<2, T, Q> call(T (*Func) (T x, T y), vec<2, T, Q> const& a, T b)
		{
			return vec<2, T, Q>(Func(a.x, b), Func(a.y, b));
		}
	};

	template<typename T, qualifier Q>
	struct functor2_vec_sca<3, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<3, T, Q> call(T (*Func) (T x, T y), vec<3, T, Q> const& a, T b)
		{
			return vec<3, T, Q>(Func(a.x, b), Func(a.y, b), Func(a.z, b));
		}
	};

	template<typename T, qualifier Q>
	struct functor2_vec_sca<4, T, Q>
	{
		GLM_FUNC_QUALIFIER static vec<4, T, Q> call(T (*Func) (T x, T y), vec<4, T, Q> const& a, T b)
		{
			return vec<4, T, Q>(Func(a.x, b), Func(a.y, b), Func(a.z, b), Func(a.w, b));
		}
	};
}//namespace detail
}//namespace glm
