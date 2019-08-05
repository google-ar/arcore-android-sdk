/// @ref ext_vector_relational
/// @file glm/ext/vector_relational.hpp
/// 
/// @see core (dependence)
///
/// @defgroup ext_vector_relational GLM_EXT_vector_relational
/// @ingroup ext
///
/// Include <glm/ext/vector_relational.hpp> to use the features of this extension.
/// 
/// Comparison functions for a user defined epsilon values.

#pragma once

// Dependencies
#include "../detail/setup.hpp"
#include "../detail/qualifier.hpp"

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_vector_relational extension included")
#endif

namespace glm
{
	/// @addtogroup ext_vector_relational
	/// @{

	/// Returns the component-wise comparison of |x - y| < epsilon.
	/// True if this expression is satisfied.
	///
	/// @tparam L Integer between 1 and 4 included that qualify the dimension of the vector
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_vector_relational
	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_DECL vec<L, bool, Q> equal(vec<L, T, Q> const& x, vec<L, T, Q> const& y, T const& epsilon);

	/// Returns the component-wise comparison of |x - y| < epsilon.
	/// True if this expression is satisfied.
	///
	/// @tparam L Integer between 1 and 4 included that qualify the dimension of the vector
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_vector_relational
	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_DECL vec<L, bool, Q> equal(vec<L, T, Q> const& x, vec<L, T, Q> const& y, vec<L, T, Q> const& epsilon);

	/// Returns the component-wise comparison of |x - y| < epsilon.
	/// True if this expression is satisfied.
	///
	/// @tparam genType Floating-point or integer scalar types
	///
	/// @see ext_vector_relational
	template<typename genType>
	GLM_FUNC_DECL bool equal(genType const& x, genType const& y, genType const& epsilon);

	/// Returns the component-wise comparison of |x - y| < epsilon.
	/// True if this expression is not satisfied.
	///
	/// @tparam L Integer between 1 and 4 included that qualify the dimension of the vector
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_vector_relational
	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_DECL vec<L, bool, Q> notEqual(vec<L, T, Q> const& x, vec<L, T, Q> const& y, T const& epsilon);

	/// Returns the component-wise comparison of |x - y| < epsilon.
	/// True if this expression is not satisfied.
	///
	/// @tparam L Integer between 1 and 4 included that qualify the dimension of the vector
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_vector_relational
	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_DECL vec<L, bool, Q> notEqual(vec<L, T, Q> const& x, vec<L, T, Q> const& y, vec<L, T, Q> const& epsilon);

	/// Returns the component-wise comparison of |x - y| >= epsilon.
	/// True if this expression is not satisfied.
	///
	/// @tparam genType Floating-point or integer scalar types
	///
	/// @see ext_vector_relational
	template<typename genType>
	GLM_FUNC_DECL bool notEqual(genType const& x, genType const& y, genType const& epsilon);

	/// @}
}//namespace glm

#include "vector_relational.inl"
