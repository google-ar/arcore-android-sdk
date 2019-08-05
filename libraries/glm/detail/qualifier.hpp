/// @ref core
/// @file glm/detail/qualifier.hpp

#pragma once

#include "setup.hpp"

namespace glm
{
	/// Qualify GLM types in term of alignment (packed, aligned) and precision in term of ULPs (lowp, mediump, highp)
	enum qualifier
	{
		packed_highp, ///< Typed data is tightly packed in memory and operations are executed with high precision in term of ULPs
		packed_mediump, ///< Typed data is tightly packed in memory  and operations are executed with medium precision in term of ULPs for higher performance
		packed_lowp, ///< Typed data is tightly packed in memory  and operations are executed with low precision in term of ULPs to maximize performance

#		if GLM_HAS_ALIGNED_TYPE
			aligned_highp, ///< Typed data is aligned in memory allowing SIMD optimizations and operations are executed with high precision in term of ULPs
			aligned_mediump, ///< Typed data is aligned in memory allowing SIMD optimizations and operations are executed with high precision in term of ULPs for higher performance
			aligned_lowp, // ///< Typed data is aligned in memory allowing SIMD optimizations and operations are executed with high precision in term of ULPs to maximize performance
			aligned = aligned_highp, ///< By default aligned qualifier is also high precision
#		endif

		highp = packed_highp, ///< By default highp qualifier is also packed
		mediump = packed_mediump, ///< By default mediump qualifier is also packed
		lowp = packed_lowp, ///< By default lowp qualifier is also packed
		packed = packed_highp, ///< By default packed qualifier is also high precision

#		if GLM_HAS_ALIGNED_TYPE && defined(GLM_FORCE_ALIGNED)
			defaultp = aligned_highp
#		else
			defaultp = highp
#		endif
	};

	template<length_t L, typename T, qualifier Q = defaultp> struct vec;
	template<length_t C, length_t R, typename T, qualifier Q = defaultp> struct mat;

namespace detail
{
	template<glm::qualifier P>
	struct is_aligned
	{
		static const bool value = false;
	};

#	if GLM_HAS_ALIGNED_TYPE
		template<>
		struct is_aligned<glm::aligned_lowp>
		{
			static const bool value = true;
		};

		template<>
		struct is_aligned<glm::aligned_mediump>
		{
			static const bool value = true;
		};

		template<>
		struct is_aligned<glm::aligned_highp>
		{
			static const bool value = true;
		};
#	endif
}//namespace detail
}//namespace glm
