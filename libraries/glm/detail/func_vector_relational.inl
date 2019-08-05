/// @ref core
/// @file glm/detail/func_vector_relational.inl

#include "compute_vector_relational.hpp"

namespace glm
{
	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> lessThan(vec<L, T, Q> const& x, vec<L, T, Q> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, Q> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] < y[i];

		return Result;
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> lessThanEqual(vec<L, T, Q> const& x, vec<L, T, Q> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, Q> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] <= y[i];
		return Result;
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> greaterThan(vec<L, T, Q> const& x, vec<L, T, Q> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, Q> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] > y[i];
		return Result;
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> greaterThanEqual(vec<L, T, Q> const& x, vec<L, T, Q> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, Q> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] >= y[i];
		return Result;
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> equal(vec<L, T, Q> const& x, vec<L, T, Q> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, Q> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = detail::compute_equal<T>::call(x[i], y[i]);
		return Result;
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> notEqual(vec<L, T, Q> const& x, vec<L, T, Q> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, Q> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = !detail::compute_equal<T>::call(x[i], y[i]);
		return Result;
	}

	template<length_t L, qualifier Q>
	GLM_FUNC_QUALIFIER bool any(vec<L, bool, Q> const& v)
	{
		bool Result = false;
		for(length_t i = 0; i < v.length(); ++i)
			Result = Result || v[i];
		return Result;
	}

	template<length_t L, qualifier Q>
	GLM_FUNC_QUALIFIER bool all(vec<L, bool, Q> const& v)
	{
		bool Result = true;
		for(length_t i = 0; i < v.length(); ++i)
			Result = Result && v[i];
		return Result;
	}

	template<length_t L, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> not_(vec<L, bool, Q> const& v)
	{
		vec<L, bool, Q> Result;
		for(length_t i = 0; i < v.length(); ++i)
			Result[i] = !v[i];
		return Result;
	}
}//namespace glm

#if GLM_ARCH != GLM_ARCH_PURE && GLM_HAS_UNRESTRICTED_UNIONS
#	include "func_vector_relational_simd.inl"
#endif
