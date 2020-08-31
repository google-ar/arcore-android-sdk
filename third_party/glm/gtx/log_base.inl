/// @ref gtx_log_base
/// @file glm/gtx/log_base.inl

namespace glm
{
	template<typename genType> 
	GLM_FUNC_QUALIFIER genType log(genType const& x, genType const& base)
	{
		assert(!detail::compute_equal<genType>::call(x, static_cast<genType>(0)));
		return glm::log(x) / glm::log(base);
	}

	template<length_t L, typename T, qualifier Q>
	GLM_FUNC_QUALIFIER vec<L, T, Q> log(vec<L, T, Q> const& x, vec<L, T, Q> const& base)
	{
		return glm::log(x) / glm::log(base);
	}
}//namespace glm
