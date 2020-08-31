/// @ref gtc_vec1
/// @file glm/gtc/vec1.hpp
///
/// @see core (dependence)
///
/// @defgroup gtc_vec1 GLM_GTC_vec1
/// @ingroup gtc
///
/// Include <glm/gtc/vec1.hpp> to use the features of this extension.
/// 
/// Add vec1, ivec1, uvec1 and bvec1 types.

#pragma once

// Dependency:
#include "../ext/vec1.hpp"

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTC_vec1 extension included")
#endif

namespace glm
{
	//////////////////////////
	// vec1 definition

#if(defined(GLM_PRECISION_HIGHP_BOOL))
	typedef highp_bvec1				bvec1;
#elif(defined(GLM_PRECISION_MEDIUMP_BOOL))
	typedef mediump_bvec1			bvec1;
#elif(defined(GLM_PRECISION_LOWP_BOOL))
	typedef lowp_bvec1				bvec1;
#else
	/// 1 component vector of boolean.
	/// @see gtc_vec1 extension.
	typedef highp_bvec1				bvec1;
#endif//GLM_PRECISION

#if(defined(GLM_PRECISION_HIGHP_FLOAT))
	typedef highp_vec1				vec1;
#elif(defined(GLM_PRECISION_MEDIUMP_FLOAT))
	typedef mediump_vec1			vec1;
#elif(defined(GLM_PRECISION_LOWP_FLOAT))
	typedef lowp_vec1				vec1;
#else
	/// 1 component vector of floating-point numbers.
	/// @see gtc_vec1 extension.
	typedef highp_vec1				vec1;
#endif//GLM_PRECISION

#if(defined(GLM_PRECISION_HIGHP_DOUBLE))
	typedef highp_dvec1				dvec1;
#elif(defined(GLM_PRECISION_MEDIUMP_DOUBLE))
	typedef mediump_dvec1			dvec1;
#elif(defined(GLM_PRECISION_LOWP_DOUBLE))
	typedef lowp_dvec1				dvec1;
#else
	/// 1 component vector of floating-point numbers.
	/// @see gtc_vec1 extension.
	typedef highp_dvec1				dvec1;
#endif//GLM_PRECISION

#if(defined(GLM_PRECISION_HIGHP_INT))
	typedef highp_ivec1			ivec1;
#elif(defined(GLM_PRECISION_MEDIUMP_INT))
	typedef mediump_ivec1		ivec1;
#elif(defined(GLM_PRECISION_LOWP_INT))
	typedef lowp_ivec1			ivec1;
#else
	/// 1 component vector of signed integer numbers. 
	/// @see gtc_vec1 extension.
	typedef highp_ivec1			ivec1;
#endif//GLM_PRECISION

#if(defined(GLM_PRECISION_HIGHP_UINT))
	typedef highp_uvec1			uvec1;
#elif(defined(GLM_PRECISION_MEDIUMP_UINT))
	typedef mediump_uvec1		uvec1;
#elif(defined(GLM_PRECISION_LOWP_UINT))
	typedef lowp_uvec1			uvec1;
#else
	/// 1 component vector of unsigned integer numbers. 
	/// @see gtc_vec1 extension.
	typedef highp_uvec1			uvec1;
#endif//GLM_PRECISION

}// namespace glm

#include "vec1.inl"
