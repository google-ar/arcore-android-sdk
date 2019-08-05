/// @ref gtx_matrix_operation
/// @file glm/gtx/matrix_operation.inl

namespace glm
{
	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 2, T, Q> diagonal2x2
	(
		vec<2, T, Q> const& v
	)
	{
		mat<2, 2, T, Q> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> diagonal2x3
	(
		vec<2, T, Q> const& v
	)
	{
		mat<2, 3, T, Q> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 4, T, Q> diagonal2x4
	(
		vec<2, T, Q> const& v
	)
	{
		mat<2, 4, T, Q> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> diagonal3x2
	(
		vec<2, T, Q> const& v
	)
	{
		mat<3, 2, T, Q> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 3, T, Q> diagonal3x3
	(
		vec<3, T, Q> const& v
	)
	{
		mat<3, 3, T, Q> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		Result[2][2] = v[2];
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 4, T, Q> diagonal3x4
	(
		vec<3, T, Q> const& v
	)
	{
		mat<3, 4, T, Q> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		Result[2][2] = v[2];
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> diagonal4x4
	(
		vec<4, T, Q> const& v
	)
	{
		mat<4, 4, T, Q> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		Result[2][2] = v[2];
		Result[3][3] = v[3];
		return Result;		
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 3, T, Q> diagonal4x3
	(
		vec<3, T, Q> const& v
	)
	{
		mat<4, 3, T, Q> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		Result[2][2] = v[2];
		return Result;		
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 2, T, Q> diagonal4x2
	(
		vec<2, T, Q> const& v
	)
	{
		mat<4, 2, T, Q> Result(static_cast<T>(1));
		Result[0][0] = v[0];
		Result[1][1] = v[1];
		return Result;		
	}
}//namespace glm
