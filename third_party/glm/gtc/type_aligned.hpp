/// @ref gtc_type_aligned
/// @file glm/gtc/type_aligned.hpp
///
/// @see core (dependence)
///
/// @defgroup gtc_type_aligned GLM_GTC_type_aligned
/// @ingroup gtc
///
/// Include <glm/gtc/type_aligned.hpp> to use the features of this extension.
///
/// Aligned types allowing SIMD optimizations of vectors and matrices types

#pragma once

#if !GLM_HAS_ALIGNED_TYPE
#	error "GLM: Aligned types are not supported on this platform"
#endif
#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
# pragma message("GLM: GLM_GTC_type_aligned extension included")
#endif

#include "../vec2.hpp"
#include "../vec3.hpp"
#include "../vec4.hpp"
#include "../gtc/vec1.hpp"

namespace glm
{
	/// @addtogroup gtc_type_aligned
	/// @{

	// -- *vec1 --

	/// 1 component vector aligned in memory of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<1, float, aligned_highp>	aligned_highp_vec1;

	/// 1 component vector aligned in memory of single-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<1, float, aligned_mediump>	aligned_mediump_vec1;

	/// 1 component vector aligned in memory of single-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<1, float, aligned_lowp>		aligned_lowp_vec1;

	/// 1 component vector aligned in memory of double-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<1, double, aligned_highp>	aligned_highp_dvec1;

	/// 1 component vector aligned in memory of double-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<1, double, aligned_mediump>	aligned_mediump_dvec1;

	/// 1 component vector aligned in memory of double-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<1, double, aligned_lowp>	aligned_lowp_dvec1;

	/// 1 component vector aligned in memory of signed integer numbers.
	typedef vec<1, int, aligned_highp>		aligned_highp_ivec1;

	/// 1 component vector aligned in memory of signed integer numbers.
	typedef vec<1, int, aligned_mediump>	aligned_mediump_ivec1;

	/// 1 component vector aligned in memory of signed integer numbers.
	typedef vec<1, int, aligned_lowp>		aligned_lowp_ivec1;

	/// 1 component vector aligned in memory of unsigned integer numbers.
	typedef vec<1, uint, aligned_highp>		aligned_highp_uvec1;

	/// 1 component vector aligned in memory of unsigned integer numbers.
	typedef vec<1, uint, aligned_mediump>	aligned_mediump_uvec1;

	/// 1 component vector aligned in memory of unsigned integer numbers.
	typedef vec<1, uint, aligned_lowp>		aligned_lowp_uvec1;

	/// 1 component vector aligned in memory of bool values.
	typedef vec<1, bool, aligned_highp>		aligned_highp_bvec1;

	/// 1 component vector aligned in memory of bool values.
	typedef vec<1, bool, aligned_mediump>	aligned_mediump_bvec1;

	/// 1 component vector aligned in memory of bool values.
	typedef vec<1, bool, aligned_lowp>		aligned_lowp_bvec1;

	/// 1 component vector tightly packed in memory of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<1, float, packed_highp>		packed_highp_vec1;

	/// 1 component vector tightly packed in memory of single-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<1, float, packed_mediump>	packed_mediump_vec1;

	/// 1 component vector tightly packed in memory of single-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<1, float, packed_lowp>		packed_lowp_vec1;

	/// 1 component vector tightly packed in memory of double-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<1, double, packed_highp>	packed_highp_dvec1;

	/// 1 component vector tightly packed in memory of double-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<1, double, packed_mediump>	packed_mediump_dvec1;

	/// 1 component vector tightly packed in memory of double-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<1, double, packed_lowp>		packed_lowp_dvec1;

	/// 1 component vector tightly packed in memory of signed integer numbers.
	typedef vec<1, int, packed_highp>		packed_highp_ivec1;

	/// 1 component vector tightly packed in memory of signed integer numbers.
	typedef vec<1, int, packed_mediump>		packed_mediump_ivec1;

	/// 1 component vector tightly packed in memory of signed integer numbers.
	typedef vec<1, int, packed_lowp>		packed_lowp_ivec1;

	/// 1 component vector tightly packed in memory of unsigned integer numbers.
	typedef vec<1, uint, packed_highp>		packed_highp_uvec1;

	/// 1 component vector tightly packed in memory of unsigned integer numbers.
	typedef vec<1, uint, packed_mediump>	packed_mediump_uvec1;

	/// 1 component vector tightly packed in memory of unsigned integer numbers.
	typedef vec<1, uint, packed_lowp>		packed_lowp_uvec1;

	/// 1 component vector tightly packed in memory of bool values.
	typedef vec<1, bool, packed_highp>		packed_highp_bvec1;

	/// 1 component vector tightly packed in memory of bool values.
	typedef vec<1, bool, packed_mediump>	packed_mediump_bvec1;

	/// 1 component vector tightly packed in memory of bool values.
	typedef vec<1, bool, packed_lowp>		packed_lowp_bvec1;

	// -- *vec2 --

	/// 2 components vector aligned in memory of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<2, float, aligned_highp>	aligned_highp_vec2;

	/// 2 components vector aligned in memory of single-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<2, float, aligned_mediump>	aligned_mediump_vec2;

	/// 2 components vector aligned in memory of single-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<2, float, aligned_lowp>		aligned_lowp_vec2;

	/// 2 components vector aligned in memory of double-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<2, double, aligned_highp>	aligned_highp_dvec2;

	/// 2 components vector aligned in memory of double-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<2, double, aligned_mediump>	aligned_mediump_dvec2;

	/// 2 components vector aligned in memory of double-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<2, double, aligned_lowp>	aligned_lowp_dvec2;

	/// 2 components vector aligned in memory of signed integer numbers.
	typedef vec<2, int, aligned_highp>		aligned_highp_ivec2;

	/// 2 components vector aligned in memory of signed integer numbers.
	typedef vec<2, int, aligned_mediump>	aligned_mediump_ivec2;

	/// 2 components vector aligned in memory of signed integer numbers.
	typedef vec<2, int, aligned_lowp>		aligned_lowp_ivec2;

	/// 2 components vector aligned in memory of unsigned integer numbers.
	typedef vec<2, uint, aligned_highp>		aligned_highp_uvec2;

	/// 2 components vector aligned in memory of unsigned integer numbers.
	typedef vec<2, uint, aligned_mediump>	aligned_mediump_uvec2;

	/// 2 components vector aligned in memory of unsigned integer numbers.
	typedef vec<2, uint, aligned_lowp>		aligned_lowp_uvec2;

	/// 2 components vector aligned in memory of bool values.
	typedef vec<2, bool, aligned_highp>		aligned_highp_bvec2;

	/// 2 components vector aligned in memory of bool values.
	typedef vec<2, bool, aligned_mediump>	aligned_mediump_bvec2;

	/// 2 components vector aligned in memory of bool values.
	typedef vec<2, bool, aligned_lowp>		aligned_lowp_bvec2;

	/// 2 components vector tightly packed in memory of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<2, float, packed_highp>		packed_highp_vec2;

	/// 2 components vector tightly packed in memory of single-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<2, float, packed_mediump>	packed_mediump_vec2;

	/// 2 components vector tightly packed in memory of single-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<2, float, packed_lowp>		packed_lowp_vec2;

	/// 2 components vector tightly packed in memory of double-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<2, double, packed_highp>	packed_highp_dvec2;

	/// 2 components vector tightly packed in memory of double-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<2, double, packed_mediump>	packed_mediump_dvec2;

	/// 2 components vector tightly packed in memory of double-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<2, double, packed_lowp>		packed_lowp_dvec2;

	/// 2 components vector tightly packed in memory of signed integer numbers.
	typedef vec<2, int, packed_highp>		packed_highp_ivec2;

	/// 2 components vector tightly packed in memory of signed integer numbers.
	typedef vec<2, int, packed_mediump>		packed_mediump_ivec2;

	/// 2 components vector tightly packed in memory of signed integer numbers.
	typedef vec<2, int, packed_lowp>		packed_lowp_ivec2;

	/// 2 components vector tightly packed in memory of unsigned integer numbers.
	typedef vec<2, uint, packed_highp>		packed_highp_uvec2;

	/// 2 components vector tightly packed in memory of unsigned integer numbers.
	typedef vec<2, uint, packed_mediump>	packed_mediump_uvec2;

	/// 2 components vector tightly packed in memory of unsigned integer numbers.
	typedef vec<2, uint, packed_lowp>		packed_lowp_uvec2;

	/// 2 components vector tightly packed in memory of bool values.
	typedef vec<2, bool, packed_highp>		packed_highp_bvec2;

	/// 2 components vector tightly packed in memory of bool values.
	typedef vec<2, bool, packed_mediump>	packed_mediump_bvec2;

	/// 2 components vector tightly packed in memory of bool values.
	typedef vec<2, bool, packed_lowp>		packed_lowp_bvec2;

	// -- *vec3 --

	/// 3 components vector aligned in memory of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<3, float, aligned_highp>	aligned_highp_vec3;

	/// 3 components vector aligned in memory of single-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<3, float, aligned_mediump>	aligned_mediump_vec3;

	/// 3 components vector aligned in memory of single-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<3, float, aligned_lowp>		aligned_lowp_vec3;

	/// 3 components vector aligned in memory of double-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<3, double, aligned_highp>	aligned_highp_dvec3;

	/// 3 components vector aligned in memory of double-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<3, double, aligned_mediump>	aligned_mediump_dvec3;

	/// 3 components vector aligned in memory of double-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<3, double, aligned_lowp>	aligned_lowp_dvec3;

	/// 3 components vector aligned in memory of signed integer numbers.
	typedef vec<3, int, aligned_highp>		aligned_highp_ivec3;

	/// 3 components vector aligned in memory of signed integer numbers.
	typedef vec<3, int, aligned_mediump>	aligned_mediump_ivec3;

	/// 3 components vector aligned in memory of signed integer numbers.
	typedef vec<3, int, aligned_lowp>		aligned_lowp_ivec3;

	/// 3 components vector aligned in memory of unsigned integer numbers.
	typedef vec<3, uint, aligned_highp>		aligned_highp_uvec3;

	/// 3 components vector aligned in memory of unsigned integer numbers.
	typedef vec<3, uint, aligned_mediump>	aligned_mediump_uvec3;

	/// 3 components vector aligned in memory of unsigned integer numbers.
	typedef vec<3, uint, aligned_lowp>		aligned_lowp_uvec3;

	/// 3 components vector aligned in memory of bool values.
	typedef vec<3, bool, aligned_highp>		aligned_highp_bvec3;

	/// 3 components vector aligned in memory of bool values.
	typedef vec<3, bool, aligned_mediump>	aligned_mediump_bvec3;

	/// 3 components vector aligned in memory of bool values.
	typedef vec<3, bool, aligned_lowp>		aligned_lowp_bvec3;

	/// 3 components vector tightly packed in memory of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<3, float, packed_highp>		packed_highp_vec3;

	/// 3 components vector tightly packed in memory of single-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<3, float, packed_mediump>	packed_mediump_vec3;

	/// 3 components vector tightly packed in memory of single-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<3, float, packed_lowp>		packed_lowp_vec3;

	/// 3 components vector tightly packed in memory of double-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<3, double, packed_highp>	packed_highp_dvec3;

	/// 3 components vector tightly packed in memory of double-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<3, double, packed_mediump>	packed_mediump_dvec3;

	/// 3 components vector tightly packed in memory of double-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<3, double, packed_lowp>		packed_lowp_dvec3;

	/// 3 components vector tightly packed in memory of signed integer numbers.
	typedef vec<3, int, packed_highp>		packed_highp_ivec3;

	/// 3 components vector tightly packed in memory of signed integer numbers.
	typedef vec<3, int, packed_mediump>		packed_mediump_ivec3;

	/// 3 components vector tightly packed in memory of signed integer numbers.
	typedef vec<3, int, packed_lowp>		packed_lowp_ivec3;

	/// 3 components vector tightly packed in memory of unsigned integer numbers.
	typedef vec<3, uint, packed_highp>		packed_highp_uvec3;

	/// 3 components vector tightly packed in memory of unsigned integer numbers.
	typedef vec<3, uint, packed_mediump>	packed_mediump_uvec3;

	/// 3 components vector tightly packed in memory of unsigned integer numbers.
	typedef vec<3, uint, packed_lowp>		packed_lowp_uvec3;

	/// 3 components vector tightly packed in memory of bool values.
	typedef vec<3, bool, packed_highp>		packed_highp_bvec3;

	/// 3 components vector tightly packed in memory of bool values.
	typedef vec<3, bool, packed_mediump>	packed_mediump_bvec3;

	/// 3 components vector tightly packed in memory of bool values.
	typedef vec<3, bool, packed_lowp>		packed_lowp_bvec3;

	// -- *vec4 --

	/// 4 components vector aligned in memory of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<4, float, aligned_highp>	aligned_highp_vec4;

	/// 4 components vector aligned in memory of single-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<4, float, aligned_mediump>	aligned_mediump_vec4;

	/// 4 components vector aligned in memory of single-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<4, float, aligned_lowp>		aligned_lowp_vec4;

	/// 4 components vector aligned in memory of double-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<4, double, aligned_highp>	aligned_highp_dvec4;

	/// 4 components vector aligned in memory of double-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<4, double, aligned_mediump>	aligned_mediump_dvec4;

	/// 4 components vector aligned in memory of double-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<4, double, aligned_lowp>	aligned_lowp_dvec4;

	/// 4 components vector aligned in memory of signed integer numbers.
	typedef vec<4, int, aligned_highp>		aligned_highp_ivec4;

	/// 4 components vector aligned in memory of signed integer numbers.
	typedef vec<4, int, aligned_mediump>	aligned_mediump_ivec4;

	/// 4 components vector aligned in memory of signed integer numbers.
	typedef vec<4, int, aligned_lowp>		aligned_lowp_ivec4;

	/// 4 components vector aligned in memory of unsigned integer numbers.
	typedef vec<4, uint, aligned_highp>		aligned_highp_uvec4;

	/// 4 components vector aligned in memory of unsigned integer numbers.
	typedef vec<4, uint, aligned_mediump>	aligned_mediump_uvec4;

	/// 4 components vector aligned in memory of unsigned integer numbers.
	typedef vec<4, uint, aligned_lowp>		aligned_lowp_uvec4;

	/// 4 components vector aligned in memory of bool values.
	typedef vec<4, bool, aligned_highp>		aligned_highp_bvec4;

	/// 4 components vector aligned in memory of bool values.
	typedef vec<4, bool, aligned_mediump>	aligned_mediump_bvec4;

	/// 4 components vector aligned in memory of bool values.
	typedef vec<4, bool, aligned_lowp>		aligned_lowp_bvec4;

	/// 4 components vector tightly packed in memory of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<4, float, packed_highp>		packed_highp_vec4;

	/// 4 components vector tightly packed in memory of single-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<4, float, packed_mediump>	packed_mediump_vec4;

	/// 4 components vector tightly packed in memory of single-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<4, float, packed_lowp>		packed_lowp_vec4;

	/// 4 components vector tightly packed in memory of double-precision floating-point numbers using high precision arithmetic in term of ULPs.
	typedef vec<4, double, packed_highp>	packed_highp_dvec4;

	/// 4 components vector tightly packed in memory of double-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	typedef vec<4, double, packed_mediump>	packed_mediump_dvec4;

	/// 4 components vector tightly packed in memory of double-precision floating-point numbers using low precision arithmetic in term of ULPs.
	typedef vec<4, double, packed_lowp>		packed_lowp_dvec4;

	/// 4 components vector tightly packed in memory of signed integer numbers.
	typedef vec<4, int, packed_highp>		packed_highp_ivec4;

	/// 4 components vector tightly packed in memory of signed integer numbers.
	typedef vec<4, int, packed_mediump>		packed_mediump_ivec4;

	/// 4 components vector tightly packed in memory of signed integer numbers.
	typedef vec<4, int, packed_lowp>		packed_lowp_ivec4;

	/// 4 components vector tightly packed in memory of unsigned integer numbers.
	typedef vec<4, uint, packed_highp>		packed_highp_uvec4;

	/// 4 components vector tightly packed in memory of unsigned integer numbers.
	typedef vec<4, uint, packed_mediump>	packed_mediump_uvec4;

	/// 4 components vector tightly packed in memory of unsigned integer numbers.
	typedef vec<4, uint, packed_lowp>		packed_lowp_uvec4;

	/// 4 components vector tightly packed in memory of bool values.
	typedef vec<4, bool, packed_highp>		packed_highp_bvec4;

	/// 4 components vector tightly packed in memory of bool values.
	typedef vec<4, bool, packed_mediump>	packed_mediump_bvec4;

	/// 4 components vector tightly packed in memory of bool values.
	typedef vec<4, bool, packed_lowp>		packed_lowp_bvec4;

	// -- default --

#if(defined(GLM_PRECISION_LOWP_FLOAT))
	typedef aligned_lowp_vec1			aligned_vec1;
	typedef aligned_lowp_vec2			aligned_vec2;
	typedef aligned_lowp_vec3			aligned_vec3;
	typedef aligned_lowp_vec4			aligned_vec4;
	typedef packed_lowp_vec1			packed_vec1;
	typedef packed_lowp_vec2			packed_vec2;
	typedef packed_lowp_vec3			packed_vec3;
	typedef packed_lowp_vec4			packed_vec4;
#elif(defined(GLM_PRECISION_MEDIUMP_FLOAT))
	typedef aligned_mediump_vec1		aligned_vec1;
	typedef aligned_mediump_vec2		aligned_vec2;
	typedef aligned_mediump_vec3		aligned_vec3;
	typedef aligned_mediump_vec4		aligned_vec4;
	typedef packed_mediump_vec1			packed_vec1;
	typedef packed_mediump_vec2			packed_vec2;
	typedef packed_mediump_vec3			packed_vec3;
	typedef packed_mediump_vec4			packed_vec4;
#else //defined(GLM_PRECISION_HIGHP_FLOAT)
	/// 1 component vector aligned in memory of single-precision floating-point numbers.
	typedef aligned_highp_vec1			aligned_vec1;

	/// 2 components vector aligned in memory of single-precision floating-point numbers.
	typedef aligned_highp_vec2			aligned_vec2;

	/// 3 components vector aligned in memory of single-precision floating-point numbers.
	typedef aligned_highp_vec3			aligned_vec3;

	/// 4 components vector aligned in memory of single-precision floating-point numbers.
	typedef aligned_highp_vec4			aligned_vec4;

	/// 1 component vector tightly packed in memory of single-precision floating-point numbers.
	typedef packed_highp_vec1			packed_vec1;

	/// 2 components vector tightly packed in memory of single-precision floating-point numbers.
	typedef packed_highp_vec2			packed_vec2;

	/// 3 components vector tightly packed in memory of single-precision floating-point numbers.
	typedef packed_highp_vec3			packed_vec3;

	/// 4 components vector tightly packed in memory of single-precision floating-point numbers.
	typedef packed_highp_vec4			packed_vec4;
#endif//GLM_PRECISION

#if(defined(GLM_PRECISION_LOWP_DOUBLE))
	typedef aligned_lowp_dvec1			aligned_dvec1;
	typedef aligned_lowp_dvec2			aligned_dvec2;
	typedef aligned_lowp_dvec3			aligned_dvec3;
	typedef aligned_lowp_dvec4			aligned_dvec4;
	typedef packed_lowp_dvec1			packed_dvec1;
	typedef packed_lowp_dvec2			packed_dvec2;
	typedef packed_lowp_dvec3			packed_dvec3;
	typedef packed_lowp_dvec4			packed_dvec4;
#elif(defined(GLM_PRECISION_MEDIUMP_DOUBLE))
	typedef aligned_mediump_dvec1		aligned_dvec1;
	typedef aligned_mediump_dvec2		aligned_dvec2;
	typedef aligned_mediump_dvec3		aligned_dvec3;
	typedef aligned_mediump_dvec4		aligned_dvec4;
	typedef packed_mediump_dvec1		packed_dvec1;
	typedef packed_mediump_dvec2		packed_dvec2;
	typedef packed_mediump_dvec3		packed_dvec3;
	typedef packed_mediump_dvec4		packed_dvec4;
#else //defined(GLM_PRECISION_HIGHP_DOUBLE)
	/// 1 component vector aligned in memory of double-precision floating-point numbers.
	typedef aligned_highp_dvec1			aligned_dvec1;

	/// 2 components vector aligned in memory of double-precision floating-point numbers.
	typedef aligned_highp_dvec2			aligned_dvec2;

	/// 3 components vector aligned in memory of double-precision floating-point numbers.
	typedef aligned_highp_dvec3			aligned_dvec3;

	/// 4 components vector aligned in memory of double-precision floating-point numbers.
	typedef aligned_highp_dvec4			aligned_dvec4;

	/// 1 component vector tightly packed in memory of double-precision floating-point numbers.
	typedef packed_highp_dvec1			packed_dvec1;

	/// 2 components vector tightly packed in memory of double-precision floating-point numbers.
	typedef packed_highp_dvec2			packed_dvec2;

	/// 3 components vector tightly packed in memory of double-precision floating-point numbers.
	typedef packed_highp_dvec3			packed_dvec3;

	/// 4 components vector tightly packed in memory of double-precision floating-point numbers.
	typedef packed_highp_dvec4			packed_dvec4;
#endif//GLM_PRECISION

#if(defined(GLM_PRECISION_LOWP_INT))
	typedef aligned_lowp_ivec1			aligned_ivec1;
	typedef aligned_lowp_ivec2			aligned_ivec2;
	typedef aligned_lowp_ivec3			aligned_ivec3;
	typedef aligned_lowp_ivec4			aligned_ivec4;
#elif(defined(GLM_PRECISION_MEDIUMP_INT))
	typedef aligned_mediump_ivec1		aligned_ivec1;
	typedef aligned_mediump_ivec2		aligned_ivec2;
	typedef aligned_mediump_ivec3		aligned_ivec3;
	typedef aligned_mediump_ivec4		aligned_ivec4;
#else //defined(GLM_PRECISION_HIGHP_INT)
	/// 1 component vector aligned in memory of signed integer numbers.
	typedef aligned_highp_ivec1			aligned_ivec1;

	/// 2 components vector aligned in memory of signed integer numbers.
	typedef aligned_highp_ivec2			aligned_ivec2;

	/// 3 components vector aligned in memory of signed integer numbers.
	typedef aligned_highp_ivec3			aligned_ivec3;

	/// 4 components vector aligned in memory of signed integer numbers.
	typedef aligned_highp_ivec4			aligned_ivec4;

	/// 1 component vector tightly packed in memory of signed integer numbers.
	typedef packed_highp_ivec1			packed_ivec1;

	/// 2 components vector tightly packed in memory of signed integer numbers.
	typedef packed_highp_ivec2			packed_ivec2;

	/// 3 components vector tightly packed in memory of signed integer numbers.
	typedef packed_highp_ivec3			packed_ivec3;

	/// 4 components vector tightly packed in memory of signed integer numbers.
	typedef packed_highp_ivec4			packed_ivec4;

#endif//GLM_PRECISION

	// -- Unsigned integer definition --

#if(defined(GLM_PRECISION_LOWP_UINT))
	typedef aligned_lowp_uvec1			aligned_uvec1;
	typedef aligned_lowp_uvec2			aligned_uvec2;
	typedef aligned_lowp_uvec3			aligned_uvec3;
	typedef aligned_lowp_uvec4			aligned_uvec4;
#elif(defined(GLM_PRECISION_MEDIUMP_UINT))
	typedef aligned_mediump_uvec1		aligned_uvec1;
	typedef aligned_mediump_uvec2		aligned_uvec2;
	typedef aligned_mediump_uvec3		aligned_uvec3;
	typedef aligned_mediump_uvec4		aligned_uvec4;
#else //defined(GLM_PRECISION_HIGHP_UINT)
	/// 1 component vector aligned in memory of unsigned integer numbers.
	typedef aligned_highp_uvec1			aligned_uvec1;

	/// 2 components vector aligned in memory of unsigned integer numbers.
	typedef aligned_highp_uvec2			aligned_uvec2;

	/// 3 components vector aligned in memory of unsigned integer numbers.
	typedef aligned_highp_uvec3			aligned_uvec3;

	/// 4 components vector aligned in memory of unsigned integer numbers.
	typedef aligned_highp_uvec4			aligned_uvec4;

	/// 1 component vector tightly packed in memory of unsigned integer numbers.
	typedef packed_highp_uvec1			packed_uvec1;

	/// 2 components vector tightly packed in memory of unsigned integer numbers.
	typedef packed_highp_uvec2			packed_uvec2;

	/// 3 components vector tightly packed in memory of unsigned integer numbers.
	typedef packed_highp_uvec3			packed_uvec3;

	/// 4 components vector tightly packed in memory of unsigned integer numbers.
	typedef packed_highp_uvec4			packed_uvec4;
#endif//GLM_PRECISION

#if(defined(GLM_PRECISION_LOWP_BOOL))
	typedef aligned_lowp_bvec1			aligned_bvec1;
	typedef aligned_lowp_bvec2			aligned_bvec2;
	typedef aligned_lowp_bvec3			aligned_bvec3;
	typedef aligned_lowp_bvec4			aligned_bvec4;
#elif(defined(GLM_PRECISION_MEDIUMP_BOOL))
	typedef aligned_mediump_bvec1		aligned_bvec1;
	typedef aligned_mediump_bvec2		aligned_bvec2;
	typedef aligned_mediump_bvec3		aligned_bvec3;
	typedef aligned_mediump_bvec4		aligned_bvec4;
#else //defined(GLM_PRECISION_HIGHP_BOOL)
	/// 1 component vector aligned in memory of bool values.
	typedef aligned_highp_bvec1			aligned_bvec1;

	/// 2 components vector aligned in memory of bool values.
	typedef aligned_highp_bvec2			aligned_bvec2;

	/// 3 components vector aligned in memory of bool values.
	typedef aligned_highp_bvec3			aligned_bvec3;

	/// 4 components vector aligned in memory of bool values.
	typedef aligned_highp_bvec4			aligned_bvec4;

	/// 1 components vector tightly packed in memory of bool values.
	typedef packed_highp_bvec1			packed_bvec1;

	/// 2 components vector tightly packed in memory of bool values.
	typedef packed_highp_bvec2			packed_bvec2;

	/// 3 components vector tightly packed in memory of bool values.
	typedef packed_highp_bvec3			packed_bvec3;

	/// 4 components vector tightly packed in memory of bool values.
	typedef packed_highp_bvec4			packed_bvec4;
#endif//GLM_PRECISION

	/// @}
}//namespace glm
