/// @ref gtx_type_trait
/// @file glm/gtx/type_trait.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_type_trait GLM_GTX_type_trait
/// @ingroup gtx
///
/// Include <glm/gtx/type_trait.hpp> to use the features of this extension.
///
/// Defines traits for each type.

#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_type_trait is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

// Dependency:
#include "../detail/type_vec2.hpp"
#include "../detail/type_vec3.hpp"
#include "../detail/type_vec4.hpp"
#include "../detail/type_mat2x2.hpp"
#include "../detail/type_mat2x3.hpp"
#include "../detail/type_mat2x4.hpp"
#include "../detail/type_mat3x2.hpp"
#include "../detail/type_mat3x3.hpp"
#include "../detail/type_mat3x4.hpp"
#include "../detail/type_mat4x2.hpp"
#include "../detail/type_mat4x3.hpp"
#include "../detail/type_mat4x4.hpp"
#include "../gtc/quaternion.hpp"
#include "../gtx/dual_quaternion.hpp"

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_type_trait extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_type_trait
	/// @{

	template<typename T>
	struct type
	{
		static bool const is_vec = false;
		static bool const is_mat = false;
		static bool const is_quat = false;
		static length_t const components = 0;
		static length_t const cols = 0;
		static length_t const rows = 0;
	};

	template<length_t L, typename T, qualifier Q>
	struct type<vec<L, T, Q> >
	{
		static bool const is_vec = true;
		static bool const is_mat = false;
		static bool const is_quat = false;
		enum
		{
			components = L
		};
	};

	template<typename T, qualifier Q>
	struct type<mat<2, 2, T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = true;
		static bool const is_quat = false;
		enum
		{
			components = 2,
			cols = 2,
			rows = 2
		};
	};

	template<typename T, qualifier Q>
	struct type<mat<2, 3, T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = true;
		static bool const is_quat = false;
		enum
		{
			components = 2,
			cols = 2,
			rows = 3
		};
	};

	template<typename T, qualifier Q>
	struct type<mat<2, 4, T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = true;
		static bool const is_quat = false;
		enum
		{
			components = 2,
			cols = 2,
			rows = 4
		};
	};

	template<typename T, qualifier Q>
	struct type<mat<3, 2, T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = true;
		static bool const is_quat = false;
		enum
		{
			components = 3,
			cols = 3,
			rows = 2
		};
	};

	template<typename T, qualifier Q>
	struct type<mat<3, 3, T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = true;
		static bool const is_quat = false;
		enum
		{
			components = 3,
			cols = 3,
			rows = 3
		};
	};

	template<typename T, qualifier Q>
	struct type<mat<3, 4, T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = true;
		static bool const is_quat = false;
		enum
		{
			components = 3,
			cols = 3,
			rows = 4
		};
	};

	template<typename T, qualifier Q>
	struct type<mat<4, 2, T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = true;
		static bool const is_quat = false;
		enum
		{
			components = 4,
			cols = 4,
			rows = 2
		};
	};

	template<typename T, qualifier Q>
	struct type<mat<4, 3, T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = true;
		static bool const is_quat = false;
		enum
		{
			components = 4,
			cols = 4,
			rows = 3
		};
	};

	template<typename T, qualifier Q>
	struct type<mat<4, 4, T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = true;
		static bool const is_quat = false;
		enum
		{
			components = 4,
			cols = 4,
			rows = 4
		};
	};

	template<typename T, qualifier Q>
	struct type<tquat<T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = false;
		static bool const is_quat = true;
		enum
		{
			components = 4
		};
	};

	template<typename T, qualifier Q>
	struct type<tdualquat<T, Q> >
	{
		static bool const is_vec = false;
		static bool const is_mat = false;
		static bool const is_quat = true;
		enum
		{
			components = 8
		};
	};

	/// @}
}//namespace glm

#include "type_trait.inl"
