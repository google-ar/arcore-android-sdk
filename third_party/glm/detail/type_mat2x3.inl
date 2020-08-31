/// @ref core
/// @file glm/detail/type_mat2x3.inl

namespace glm
{
	// -- Constructors --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, qualifier Q>
		GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat()
		{}
#	endif

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, qualifier Q>
		GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<2, 3, T, Q> const& m)
		{
			this->value[0] = m.value[0];
			this->value[1] = m.value[1];
		}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, qualifier Q>
	template<qualifier P>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<2, 3, T, P> const& m)
	{
		this->value[0] = m.value[0];
		this->value[1] = m.value[1];
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(T scalar)
	{
		this->value[0] = col_type(scalar, 0, 0);
		this->value[1] = col_type(0, scalar, 0);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat 
	(
		T x0, T y0, T z0,
		T x1, T y1, T z1
	)
	{
		this->value[0] = col_type(x0, y0, z0);
		this->value[1] = col_type(x1, y1, z1);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(col_type const& v0, col_type const& v1)
	{
		this->value[0] = v0;
		this->value[1] = v1;
	}

	// -- Conversion constructors --

	template<typename T, qualifier Q>
	template<
		typename X1, typename Y1, typename Z1,
		typename X2, typename Y2, typename Z2>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat 
	(
		X1 x1, Y1 y1, Z1 z1,
		X2 x2, Y2 y2, Z2 z2
	)
	{
		this->value[0] = col_type(static_cast<T>(x1), value_type(y1), value_type(z1));
		this->value[1] = col_type(static_cast<T>(x2), value_type(y2), value_type(z2));
	}
	
	template<typename T, qualifier Q>
	template<typename V1, typename V2>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(vec<3, V1, Q> const& v1, vec<3, V2, Q> const& v2)
	{
		this->value[0] = col_type(v1);
		this->value[1] = col_type(v2);
	}

	// -- Matrix conversions --

	template<typename T, qualifier Q>
	template<typename U, qualifier P>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<2, 3, U, P> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<2, 2, T, Q> const& m)
	{
		this->value[0] = col_type(m[0], 0);
		this->value[1] = col_type(m[1], 0);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER  mat<2, 3, T, Q>::mat(mat<3, 3, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<4, 4, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<2, 4, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<3, 2, T, Q> const& m)
	{
		this->value[0] = col_type(m[0], 0);
		this->value[1] = col_type(m[1], 0);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<3, 4, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<4, 2, T, Q> const& m)
	{
		this->value[0] = col_type(m[0], 0);
		this->value[1] = col_type(m[1], 0);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>::mat(mat<4, 3, T, Q> const& m)
	{
		this->value[0] = m[0];
		this->value[1] = m[1];
	}

	// -- Accesses --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER typename mat<2, 3, T, Q>::col_type & mat<2, 3, T, Q>::operator[](typename mat<2, 3, T, Q>::length_type i)
	{
		assert(i < this->length());
		return this->value[i];
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER typename mat<2, 3, T, Q>::col_type const& mat<2, 3, T, Q>::operator[](typename mat<2, 3, T, Q>::length_type i) const
	{
		assert(i < this->length());
		return this->value[i];
	}

	// -- Unary updatable operators --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, qualifier Q>
		GLM_FUNC_QUALIFIER mat<2, 3, T, Q>& mat<2, 3, T, Q>::operator=(mat<2, 3, T, Q> const& m)
		{
			this->value[0] = m[0];
			this->value[1] = m[1];
			return *this;
		}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>& mat<2, 3, T, Q>::operator=(mat<2, 3, U, Q> const& m)
	{
		this->value[0] = m[0];
		this->value[1] = m[1];
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> & mat<2, 3, T, Q>::operator+=(U s)
	{
		this->value[0] += s;
		this->value[1] += s;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>& mat<2, 3, T, Q>::operator+=(mat<2, 3, U, Q> const& m)
	{
		this->value[0] += m[0];
		this->value[1] += m[1];
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>& mat<2, 3, T, Q>::operator-=(U s)
	{
		this->value[0] -= s;
		this->value[1] -= s;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>& mat<2, 3, T, Q>::operator-=(mat<2, 3, U, Q> const& m)
	{
		this->value[0] -= m[0];
		this->value[1] -= m[1];
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q>& mat<2, 3, T, Q>::operator*=(U s)
	{
		this->value[0] *= s;
		this->value[1] *= s;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> & mat<2, 3, T, Q>::operator/=(U s)
	{
		this->value[0] /= s;
		this->value[1] /= s;
		return *this;
	}

	// -- Increment and decrement operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> & mat<2, 3, T, Q>::operator++()
	{
		++this->value[0];
		++this->value[1];
		return *this;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> & mat<2, 3, T, Q>::operator--()
	{
		--this->value[0];
		--this->value[1];
		return *this;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> mat<2, 3, T, Q>::operator++(int)
	{
		mat<2, 3, T, Q> Result(*this);
		++*this;
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> mat<2, 3, T, Q>::operator--(int)
	{
		mat<2, 3, T, Q> Result(*this);
		--*this;
		return Result;
	}

	// -- Unary arithmetic operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator+(mat<2, 3, T, Q> const& m)
	{
		return m;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator-(mat<2, 3, T, Q> const& m)
	{
		return mat<2, 3, T, Q>(
			-m[0],
			-m[1]);
	}

	// -- Binary arithmetic operators --

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator+(mat<2, 3, T, Q> const& m, T scalar)
	{
		return mat<2, 3, T, Q>(
			m[0] + scalar,
			m[1] + scalar);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator+(mat<2, 3, T, Q> const& m1, mat<2, 3, T, Q> const& m2)
	{
		return mat<2, 3, T, Q>(
			m1[0] + m2[0],
			m1[1] + m2[1]);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator-(mat<2, 3, T, Q> const& m, T scalar)
	{
		return mat<2, 3, T, Q>(
			m[0] - scalar,
			m[1] - scalar);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator-(mat<2, 3, T, Q> const& m1, mat<2, 3, T, Q> const& m2)
	{
		return mat<2, 3, T, Q>(
			m1[0] - m2[0],
			m1[1] - m2[1]);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator*(mat<2, 3, T, Q> const& m, T scalar)
	{
		return mat<2, 3, T, Q>(
			m[0] * scalar,
			m[1] * scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator*(T scalar, mat<2, 3, T, Q> const& m)
	{
		return mat<2, 3, T, Q>(
			m[0] * scalar,
			m[1] * scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER typename mat<2, 3, T, Q>::col_type operator*
	(
		mat<2, 3, T, Q> const& m,
		typename mat<2, 3, T, Q>::row_type const& v)
	{
		return typename mat<2, 3, T, Q>::col_type(
			m[0][0] * v.x + m[1][0] * v.y,
			m[0][1] * v.x + m[1][1] * v.y,
			m[0][2] * v.x + m[1][2] * v.y);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER typename mat<2, 3, T, Q>::row_type operator*
	(
		typename mat<2, 3, T, Q>::col_type const& v,
		mat<2, 3, T, Q> const& m)
	{
		return typename mat<2, 3, T, Q>::row_type(
			v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2],
			v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator*(mat<2, 3, T, Q> const& m1, mat<2, 2, T, Q> const& m2)
	{
		return mat<2, 3, T, Q>(
			m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
			m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
			m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1],
			m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
			m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
			m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 3, T, Q> operator*(mat<2, 3, T, Q> const& m1, mat<3, 2, T, Q> const& m2)
	{
		T SrcA00 = m1[0][0];
		T SrcA01 = m1[0][1];
		T SrcA02 = m1[0][2];
		T SrcA10 = m1[1][0];
		T SrcA11 = m1[1][1];
		T SrcA12 = m1[1][2];

		T SrcB00 = m2[0][0];
		T SrcB01 = m2[0][1];
		T SrcB10 = m2[1][0];
		T SrcB11 = m2[1][1];
		T SrcB20 = m2[2][0];
		T SrcB21 = m2[2][1];

		mat<3, 3, T, Q> Result;
		Result[0][0] = SrcA00 * SrcB00 + SrcA10 * SrcB01;
		Result[0][1] = SrcA01 * SrcB00 + SrcA11 * SrcB01;
		Result[0][2] = SrcA02 * SrcB00 + SrcA12 * SrcB01;
		Result[1][0] = SrcA00 * SrcB10 + SrcA10 * SrcB11;
		Result[1][1] = SrcA01 * SrcB10 + SrcA11 * SrcB11;
		Result[1][2] = SrcA02 * SrcB10 + SrcA12 * SrcB11;
		Result[2][0] = SrcA00 * SrcB20 + SrcA10 * SrcB21;
		Result[2][1] = SrcA01 * SrcB20 + SrcA11 * SrcB21;
		Result[2][2] = SrcA02 * SrcB20 + SrcA12 * SrcB21;
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 3, T, Q> operator*(mat<2, 3, T, Q> const& m1, mat<4, 2, T, Q> const& m2)
	{
		return mat<4, 3, T, Q>(
			m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
			m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
			m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1],
			m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
			m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
			m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1],
			m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1],
			m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1],
			m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1],
			m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1],
			m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1],
			m1[0][2] * m2[3][0] + m1[1][2] * m2[3][1]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator/(mat<2, 3, T, Q> const& m, T scalar)
	{
		return mat<2, 3, T, Q>(
			m[0] / scalar,
			m[1] / scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 3, T, Q> operator/(T scalar, mat<2, 3, T, Q> const& m)
	{
		return mat<2, 3, T, Q>(
			scalar / m[0],
			scalar / m[1]);
	}

	// -- Boolean operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER bool operator==(mat<2, 3, T, Q> const& m1, mat<2, 3, T, Q> const& m2)
	{
		return (m1[0] == m2[0]) && (m1[1] == m2[1]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER bool operator!=(mat<2, 3, T, Q> const& m1, mat<2, 3, T, Q> const& m2)
	{
		return (m1[0] != m2[0]) || (m1[1] != m2[1]);
	}
} //namespace glm
