#pragma once

#include "bhavesh_setup.h"

#if BHAVESH_CXX_VER < 201103L
#error "bhavesh_silence.h needs atleast c++11 support"
#endif

#include <type_traits>

namespace bhavesh {

	namespace detail {
	namespace silence {
		/*	i dont want silence_xxxx of type silence_t to be in my namespace, rather their integral_constant versions; */
			
		// use bhavesh::silence_t::silence_xxxx like you would with `enum class`
		// has 4 members: 
		// silence_less(=0b01) silences when your input has less arguments than necessary 
		// silence_more(=0b10) silences when your input has more arguments than necessary 
		// silence_both(=0b11) silences in both cases -and-
		// silence_none(=0b00) silences in neither case
		enum silence_t : unsigned char {
			silence_none /* = 0b00 */,
			silence_less /* = 0b01 */,
			silence_more /* = 0b10 */,
			silence_both /* = 0b11 */
		};
	}
	}

	using detail::silence::silence_t;

	template<silence_t sil>
	using silence_constant = std::integral_constant<silence_t, sil>;

	namespace detail {
	namespace silence {
		template<typename Silence>
		struct is_silence_type_impl : std::false_type {};
		template<silence_t sil>
		struct is_silence_type_impl<silence_constant<sil>> : std::true_type {};
	}
	}

	template<typename Silence>
	struct is_silence_type : std::bool_constant<detail::silence::is_silence_type_impl<typename std::decay<Silence>::type>::value> {};
#if BHAVESH_CXX_VER >= 201402L
	template<typename Silence>
	constexpr bool is_silence_type_v = is_silence_type<Silence>::value;
#endif
#if BHAVESH_CXX20
	template<typename Silence>
	concept silence_type = is_silence_type_v<Silence>;
#endif

	constexpr const auto silence_none = silence_constant<silence_t::silence_none>{};
	constexpr const auto silence_less = silence_constant<silence_t::silence_less>{};
	constexpr const auto silence_more = silence_constant<silence_t::silence_more>{};
	constexpr const auto silence_both = silence_constant<silence_t::silence_both>{};

}