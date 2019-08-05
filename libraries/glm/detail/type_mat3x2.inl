/// @ref core
/// @file glm/detail/type_mat3x2.inl

namespace glm
{
	// -- Constructors --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, qualifier Q> 
		GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat()
		{}
#	endif

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, qualifier Q>
		GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<3, 2, T, Q> const& m)
		{
			this->value[0] = m.value[0];
			this->value[1] = m.value[1];
			this->value[2] = m.value[2];
		}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, qualifier Q>
	template<qualifier P>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<3, 2, T, P> const& m)
	{
		this->value[0] = m.value[0];
		this->value[1] = m.value[1];
		this->value[2] = m.value[2];
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(T scalar)
	{
		this->value[0] = col_type(scalar, 0);
		this->value[1] = col_type(0, scalar);
		this->value[2] = col_type(0, 0);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat
	(
		T x0, T y0,
		T x1, T y1,
		T x2, T y2
	)
	{
		this->value[0] = col_type(x0, y0);
		this->value[1] = col_type(x1, y1);
		this->value[2] = col_type(x2, y2);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat
	(
		col_type const& v0,
		col_type const& v1,
		col_type const& v2
	)
	{
		this->value[0] = v0;
		this->value[1] = v1;
		this->value[2] = v2;
	}

	// -- Conversion constructors --

	template<typename T, qualifier Q>
	template<
		typename X1, typename Y1,
		typename X2, typename Y2,
		typename X3, typename Y3>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat
	(
		X1 x1, Y1 y1,
		X2 x2, Y2 y2,
		X3 x3, Y3 y3
	)
	{
		this->value[0] = col_type(static_cast<T>(x1), value_type(y1));
		this->value[1] = col_type(static_cast<T>(x2), value_type(y2));
		this->value[2] = col_type(static_cast<T>(x3), value_type(y3));
	}

	template<typename T, qualifier Q>
	template<typename V1, typename V2, typename V3>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat
	(
		vec<2, V1, Q> const& v1,
		vec<2, V2, Q> const& v2,
		vec<2, V3, Q> const& v3
	)
	{
		this->value[0] = col_type(v1);
		this->value[1] = col_type(v2);
		this->value[2] = col_type(v3);
	}

	// -- Matrix conversions --

	template<typename T, qualifier Q>
	template<typename U, qualifier P>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<3, 2, U, P> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
		this->value[2] = col_type(m[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<2, 2, T, Q> const& m)
	{
		this->value[0] = m[0];
		this->value[1] = m[1];
		this->value[2] = col_type(0);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<3, 3, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
		this->value[2] = col_type(m[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<4, 4, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
		this->value[2] = col_type(m[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<2, 3, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
		this->value[2] = col_type(T(0));
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<2, 4, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
		this->value[2] = col_type(T(0));
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<3, 4, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
		this->value[2] = col_type(m[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<4, 2, T, Q> const& m)
	{
		this->value[0] = m[0];
		this->value[1] = m[1];
		this->value[2] = m[2];
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>::mat(mat<4, 3, T, Q> const& m)
	{
		this->value[0] = col_type(m[0]);
		this->value[1] = col_type(m[1]);
		this->value[2] = col_type(m[2]);
	}

	// -- Accesses --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER typename mat<3, 2, T, Q>::col_type & mat<3, 2, T, Q>::operator[](typename mat<3, 2, T, Q>::length_type i)
	{
		assert(i < this->length());
		return this->value[i];
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER typename mat<3, 2, T, Q>::col_type const& mat<3, 2, T, Q>::operator[](typename mat<3, 2, T, Q>::length_type i) const
	{
		assert(i < this->length());
		return this->value[i];
	}

	// -- Unary updatable operators --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, qualifier Q>
		GLM_FUNC_QUALIFIER mat<3, 2, T, Q>& mat<3, 2, T, Q>::operator=(mat<3, 2, T, Q> const& m)
		{
			this->value[0] = m[0];
			this->value[1] = m[1];
			this->value[2] = m[2];
			return *this;
		}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>& mat<3, 2, T, Q>::operator=(mat<3, 2, U, Q> const& m)
	{
		this->value[0] = m[0];
		this->value[1] = m[1];
		this->value[2] = m[2];
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>& mat<3, 2, T, Q>::operator+=(U s)
	{
		this->value[0] += s;
		this->value[1] += s;
		this->value[2] += s;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>& mat<3, 2, T, Q>::operator+=(mat<3, 2, U, Q> const& m)
	{
		this->value[0] += m[0];
		this->value[1] += m[1];
		this->value[2] += m[2];
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>& mat<3, 2, T, Q>::operator-=(U s)
	{
		this->value[0] -= s;
		this->value[1] -= s;
		this->value[2] -= s;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>& mat<3, 2, T, Q>::operator-=(mat<3, 2, U, Q> const& m)
	{
		this->value[0] -= m[0];
		this->value[1] -= m[1];
		this->value[2] -= m[2];
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>& mat<3, 2, T, Q>::operator*=(U s)
	{
		this->value[0] *= s;
		this->value[1] *= s;
		this->value[2] *= s;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> & mat<3, 2, T, Q>::operator/=(U s)
	{
		this->value[0] /= s;
		this->value[1] /= s;
		this->value[2] /= s;
		return *this;
	}

	// -- Increment and decrement operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>& mat<3, 2, T, Q>::operator++()
	{
		++this->value[0];
		++this->value[1];
		++this->value[2];
		return *this;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q>& mat<3, 2, T, Q>::operator--()
	{
		--this->value[0];
		--this->value[1];
		--this->value[2];
		return *this;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> mat<3, 2, T, Q>::operator++(int)
	{
		mat<3, 2, T, Q> Result(*this);
		++*this;
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> mat<3, 2, T, Q>::operator--(int)
	{
		mat<3, 2, T, Q> Result(*this);
		--*this;
		return Result;
	}

	// -- Unary arithmetic operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator+(mat<3, 2, T, Q> const& m)
	{
		return m;
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator-(mat<3, 2, T, Q> const& m)
	{
		return mat<3, 2, T, Q>(
			-m[0],
			-m[1],
			-m[2]);
	}

	// -- Binary arithmetic operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator+(mat<3, 2, T, Q> const& m, T scalar)
	{
		return mat<3, 2, T, Q>(
			m[0] + scalar,
			m[1] + scalar,
			m[2] + scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator+(mat<3, 2, T, Q> const& m1, mat<3, 2, T, Q> const& m2)
	{
		return mat<3, 2, T, Q>(
			m1[0] + m2[0],
			m1[1] + m2[1],
			m1[2] + m2[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator-(mat<3, 2, T, Q> const& m, T scalar)
	{
		return mat<3, 2, T, Q>(
			m[0] - scalar,
			m[1] - scalar,
			m[2] - scalar);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator-(mat<3, 2, T, Q> const& m1, mat<3, 2, T, Q> const& m2)
	{
		return mat<3, 2, T, Q>(
			m1[0] - m2[0],
			m1[1] - m2[1],
			m1[2] - m2[2]);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator*(mat<3, 2, T, Q> const& m, T scalar)
	{
		return mat<3, 2, T, Q>(
			m[0] * scalar,
			m[1] * scalar,
			m[2] * scalar);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator*(T scalar, mat<3, 2, T, Q> const& m)
	{
		return mat<3, 2, T, Q>(
			m[0] * scalar,
			m[1] * scalar,
			m[2] * scalar);
	}
   
	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER typename mat<3, 2, T, Q>::col_type operator*(mat<3, 2, T, Q> const& m, typename mat<3, 2, T, Q>::row_type const& v)
	{
		return typename mat<3, 2, T, Q>::col_type(
			m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z,
			m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER typename mat<3, 2, T, Q>::row_type operator*(typename mat<3, 2, T, Q>::col_type const& v, mat<3, 2, T, Q> const& m)
	{
		return typename mat<3, 2, T, Q>::row_type(
			v.x * m[0][0] + v.y * m[0][1],
			v.x * m[1][0] + v.y * m[1][1],
			v.x * m[2][0] + v.y * m[2][1]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<2, 2, T, Q> operator*(mat<3, 2, T, Q> const& m1, mat<2, 3, T, Q> const& m2)
	{
		const T SrcA00 = m1[0][0];
		const T SrcA01 = m1[0][1];
		const T SrcA10 = m1[1][0];
		const T SrcA11 = m1[1][1];
		const T SrcA20 = m1[2][0];
		const T SrcA21 = m1[2][1];

		const T SrcB00 = m2[0][0];
		const T SrcB01 = m2[0][1];
		const T SrcB02 = m2[0][2];
		const T SrcB10 = m2[1][0];
		const T SrcB11 = m2[1][1];
		const T SrcB12 = m2[1][2];

		mat<2, 2, T, Q> Result;
		Result[0][0] = SrcA00 * SrcB00 + SrcA10 * SrcB01 + SrcA20 * SrcB02;
		Result[0][1] = SrcA01 * SrcB00 + SrcA11 * SrcB01 + SrcA21 * SrcB02;
		Result[1][0] = SrcA00 * SrcB10 + SrcA10 * SrcB11 + SrcA20 * SrcB12;
		Result[1][1] = SrcA01 * SrcB10 + SrcA11 * SrcB11 + SrcA21 * SrcB12;
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator*(mat<3, 2, T, Q> const& m1, mat<3, 3, T, Q> const& m2)
	{
		return mat<3, 2, T, Q>(
			m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
			m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
			m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
			m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
			m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2],
			m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 2, T, Q> operator*(mat<3, 2, T, Q> const& m1, mat<4, 3, T, Q> const& m2)
	{
		return mat<4, 2, T, Q>(
			m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
			m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
			m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
			m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
			m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2],
			m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2],
			m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1] + m1[2][0] * m2[3][2],
			m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1] + m1[2][1] * m2[3][2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator/(mat<3, 2, T, Q> const& m, T scalar)
	{
		return mat<3, 2, T, Q>(
			m[0] / scalar,
			m[1] / scalar,
			m[2] / scalar);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER mat<3, 2, T, Q> operator/(T scalar, mat<3, 2, T, Q> const& m)
	{
		return mat<3, 2, T, Q>(
			scalar / m[0],
			scalar / m[1],
			scalar / m[2]);
	}

	// -- Boolean operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER bool operator==(mat<3, 2, T, Q> const& m1, mat<3, 2, T, Q> const& m2)
	{
		return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]);
	}

	template<typename T, qualifier Q> 
	GLM_FUNC_QUALIFIER bool operator!=(mat<3, 2, T, Q> const& m1, mat<3, 2, T, Q> const& m2)
	{
		return (m1[0] != m2[0]) || (m1[1] != m2[1]) || (m1[2] != m2[2]);
	}
} //namespace glm
