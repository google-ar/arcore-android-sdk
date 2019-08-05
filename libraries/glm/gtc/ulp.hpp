/// @ref gtc_ulp
/// @file glm/gtc/ulp.hpp
///
/// @see core (dependence)
///
/// @defgroup gtc_ulp GLM_GTC_ulp
/// @ingroup gtc
///
/// Include <glm/gtc/ulp.hpp> to use the features of this extension.
///
/// Allow the measurement of the accuracy of a function against a reference 
/// implementation. This extension works on floating-point data and provide results 
/// in ULP.

#pragma once

// Dependencies
#include "../detail/setup.hpp"
#include "../detail/qualifier.hpp"
#include "../detail/type_int.hpp"
#include "../detail/compute_vector_relational.hpp"

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTC_ulp extension included")
#endif

namespace glm
{
	/// @addtogroup gtc_ulp
	/// @{

	/// Return the next ULP value(s) after the input value(s).
	/// @see gtc_ulp
	template<typename genType>
	GLM_FUNC_DECL genType next_float(genType const& x);

	/// Return the previous ULP value(s) before the input value(s).
	/// @see gtc_ulp
	template<typename genType>
	GLM_FUNC_DECL genType prev_float(genType const& x);

	/// Return the value(s) ULP distance after the input value(s).
	/// @see gtc_ulp
	template<typename genType>
	GLM_FUNC_DECL genType next_float(genType const& x, uint const& Distance);

	/// Return the value(s) ULP distance before the input value(s).
	/// @see gtc_ulp
	template<typename genType>
	GLM_FUNC_DECL genType prev_float(genType const& x, uint const& Distance);
	
	/// Return the distance in the number of ULP between 2 scalars.
	/// @see gtc_ulp
	template<typename T>
	GLM_FUNC_DECL uint float_distance(T const& x, T const& y);

	/// Return the distance in the number of ULP between 2 vectors.
	/// @see gtc_ulp
	template<typename T, qualifier Q>
	GLM_FUNC_DECL vec<2, uint, Q> float_distance(vec<2, T, Q> const& x, vec<2, T, Q> const& y);
	
	/// @}
}// namespace glm

#include "ulp.inl"
