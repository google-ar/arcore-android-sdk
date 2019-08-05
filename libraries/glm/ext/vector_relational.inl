/// @ref ext_vector_relational
/// @file glm/ext/vector_relational.inl

// Dependency:
#include "../vector_relational.hpp"
#include "../common.hpp"
#include "../detail/type_vec.hpp"

namespace glm
{
	template<typename genType>
	GLM_FUNC_QUALIFIER bool equal(genType const& x, genType const& y, genType const& epsilon)
	{
		return abs(x - y) < epsilon;
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> equal(vec<L, T, Q> const& x, vec<L, T, Q> const& y, T const& epsilon)
	{
		return lessThan(abs(x - y), vec<L, T, Q>(epsilon));
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> equal(vec<L, T, Q> const& x, vec<L, T, Q> const& y, vec<L, T, Q> const& epsilon)
	{
		return lessThan(abs(x - y), epsilon);
	}

	template<typename genType>
	GLM_FUNC_QUALIFIER bool notEqual(genType const& x, genType const& y, genType const& epsilon)
	{
		return abs(x - y) >= epsilon;
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> notEqual(vec<L, T, Q> const& x, vec<L, T, Q> const& y, T const& epsilon)
	{
		return greaterThanEqual(abs(x - y), vec<L, T, Q>(epsilon));
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, bool, Q> notEqual(vec<L, T, Q> const& x, vec<L, T, Q> const& y, vec<L, T, Q> const& epsilon)
	{
		return greaterThanEqual(abs(x - y), epsilon);
	}
}//namespace glm
