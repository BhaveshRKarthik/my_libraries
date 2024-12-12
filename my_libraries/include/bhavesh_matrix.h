#pragma once

#include "bhavesh_setup.h"

#if BHAVESH_CXX_VER < 201402L
#error "bhavesh_matrix.h needs atleast C++14 support"
#endif

#include "bhavesh_silence.h"
#include <type_traits> // std::is_final, std::true|false_type
#include <memory> // std::allocator_traits, std::allocator
#include <utility> // std::exchange, std::move, std::swap, std::pair
#include <iterator> // std::reverse_iterator
#if BHAVESH_CXX17
#include <memory_resource>
#endif
#if BHAVESH_CXX20
#include <concepts>
#endif

namespace bhavesh {
	namespace detail {
		namespace matrix {

			template<typename ...Tys>
			/* defect in c++11 was only fixed in c++17; so for consistency just use the c++11-compatible version as its always correct */
			struct _void_t_impl { using type = void; };

			template<typename... Tys>
			using void_t = typename _void_t_impl<Tys...>::type;

			template<typename T>
			struct type_identity { using type = T; };

#if BHAVESH_CXX20
			template<typename Iterator, std::sentinel_for<Iterator> Sentinel>
			using is_sized = std::bool_constant<std::sized_sentinel_for<Sentinel, Iterator>>;
#else
			/* no sized-sentinel mechanism, giving option for backdoor using iterator_concept before c++20 so that people can give me more information through this */

			template<typename It, typename=void> struct _it_cat { using category = void; };
			template<typename It> struct _it_cat<It, void_t<typename std::iterator_traits<It>::iterator_category>> { using category = typename std::iterator_traits<It>::iterator_category; };
			template<typename It> using iterator_category = typename _it_cat<It>::category;
			template<typename It, typename=void> struct _it_con { using concept_ = void; };
			template<typename It> struct _it_cat<It, void_t<typename It::iterator_concept>> { using concept_ = typename It::iterator_concept; };
			template<typename It> using iterator_concept = typename _it_con<It>::concept_;

			/* no sized-sentinel mechanism*/
			template<typename Iterator, typename Sentinel>
			using is_sized = std::enable_if_t<!std::is_same<iterator_category<Iterator>, void>::value || !std::is_same<iterator_concept<Iterator>, void>::value, /* if both are void then its not an iterator */
				std::bool_constant<std::is_same<Iterator, Sentinel>::value && (std::is_base_of<std::random_access_iterator_tag, iterator_category<Iterator>>::value || std::is_base_of<std::random_access_iterator_tag, iterator_concept<Iterator>>::value)>>;
#endif

			template<typename Alloc, bool = std::is_final<Alloc>::value>
			struct _compressed_data : private Alloc {
				using allocator_type = Alloc;
				using _al_tr = std::allocator_traits<Alloc>;
				using pointer = typename _al_tr::pointer;
				using size_type = typename _al_tr::size_type;

			public:
				BHAVESH_CXX20_CONSTEXPR _compressed_data(const _compressed_data& oth) : Alloc(_al_tr::select_on_container_copy_construction(oth.get_allocator())),
					_start(_al_tr::allocate(get_allocator(), oth._m* oth._n)), _m(oth._m), _n(oth._n) {
				};

				BHAVESH_CXX20_CONSTEXPR _compressed_data(_compressed_data&& oth) noexcept : Alloc(std::move(oth.get_allocator())),
					_start(std::exchange(oth._start, nullptr)), _m(std::exchange(oth._m, size_type())), _n(std::exchange(oth._n, size_type())) {
				}

				BHAVESH_CXX20_CONSTEXPR _compressed_data(const Alloc& _alloc, size_type _m, size_type _n) : Alloc(_alloc),
					_start(_al_tr::allocate(get_allocator(), _m* _n)), _m(_m), _n(_n) {
				}

				BHAVESH_CXX20_CONSTEXPR _compressed_data(const Alloc& _alloc) : Alloc(_alloc), _start(nullptr), _m(), _n() {}

				BHAVESH_CXX20_CONSTEXPR void _reset() {
					if (_start) _al_tr::deallocate(get_allocator(), _start, _m * _n);
					_start = nullptr; _m = size_type(); _n = size_type();
				}
				BHAVESH_CXX20_CONSTEXPR ~_compressed_data() {
					_reset();
				}

			public:
				BHAVESH_CXX20_CONSTEXPR _compressed_data& operator=(const _compressed_data& oth) {
					if (&oth == this) return *this;
					if (_start) _al_tr::deallocate(get_allocator(), _start, _m * _n);
					_m = oth._m; _n = oth._n;
					copy_allocator(oth, typename _al_tr::propagate_on_container_copy_assignment{});
					_start = _al_tr::allocate(get_allocator(), _m * _n);
					return *this;
				}

				BHAVESH_CXX20_CONSTEXPR _compressed_data& operator=(_compressed_data&& oth) {
					if (&oth == this) return *this;
					if (_start) _al_tr::deallocate(get_allocator(), _start, _m * _n);
					_m = oth._m; _n = oth._n;
					move_allocator(std::move(oth), typename _al_tr::propagate_on_container_move_assignment{});
					_start = std::exchange(oth._start, nullptr);
					return *this;
				}

				friend BHAVESH_CXX20_CONSTEXPR void swap(_compressed_data& first, _compressed_data& second) {
					using std::swap;
					using propogate = typename _compressed_data::_al_tr::propagate_on_container_swap;
					swap(first._start, second._start);
					swap(first._m, second._m);
					swap(first._n, second._n);
					first.swap_allocator(second, propogate{});
				}

				BHAVESH_CXX20_CONSTEXPR void copy_allocator(const _compressed_data& oth, std::true_type) noexcept { this->get_allocator() = oth.get_allocator(); }
				BHAVESH_CXX20_CONSTEXPR void copy_allocator(const _compressed_data& oth, std::false_type) noexcept { /* noop */ }
				BHAVESH_CXX20_CONSTEXPR void move_allocator(_compressed_data&& oth, std::true_type) noexcept { this->get_allocator() = std::move(oth.get_allocator()); }
				BHAVESH_CXX20_CONSTEXPR void move_allocator(_compressed_data&& oth, std::false_type) noexcept { /* noop */ }
				BHAVESH_CXX20_CONSTEXPR void swap_allocator(_compressed_data& oth, std::true_type) noexcept { using std::swap; swap(this->get_allocator(), oth.get_allocator()); }
				BHAVESH_CXX20_CONSTEXPR void swap_allocator(_compressed_data& oth, std::false_type) noexcept { /* noop */ }

			public:
				BHAVESH_CXX20_CONSTEXPR       Alloc& get_allocator() { return *this; }
				BHAVESH_CXX20_CONSTEXPR const Alloc& get_allocator() const { return *this; }

			public:
				pointer _start;
				size_type _m, _n;
			};

			template<typename Alloc>
			struct _compressed_data<Alloc, true> {
				using allocator_type = Alloc;
				using _al_tr = std::allocator_traits<Alloc>;
				using pointer = typename _al_tr::pointer;
				using size_type = typename _al_tr::size_type;

				BHAVESH_CXX20_CONSTEXPR _compressed_data(const _compressed_data& oth) : _alloc(_al_tr::select_on_container_copy_construction(oth.get_allocator())),
					_start(_al_tr::allocate(get_allocator(), oth._m* oth._n)), _m(oth._m), _n(oth._n) {
				};

				BHAVESH_CXX20_CONSTEXPR _compressed_data(_compressed_data&& oth) noexcept : _alloc(std::move(oth.get_allocator())),
					_start(std::exchange(oth._start, nullptr)), _m(std::exchange(oth._m, size_type())), _n(std::exchange(oth._n, size_type())) {
				}

				BHAVESH_CXX20_CONSTEXPR _compressed_data(const Alloc& _alloc, size_type _m, size_type _n) : _alloc(_alloc),
					_start(_al_tr::allocate(get_allocator(), _m* _n)), _m(_m), _n(_n) {
				}

				BHAVESH_CXX20_CONSTEXPR _compressed_data(const Alloc& _alloc) : Alloc(_alloc), _start(nullptr), _m(), _n() {}

				BHAVESH_CXX20_CONSTEXPR void _reset() {
					if (_start) _al_tr::deallocate(get_allocator(), _start, _m * _n);
					_start = nullptr; _m = size_type(); _n = size_type();
				}
				BHAVESH_CXX20_CONSTEXPR ~_compressed_data() {
					_reset();
				}

			public:
				BHAVESH_CXX20_CONSTEXPR _compressed_data& operator=(const _compressed_data& oth) {
					if (&oth == this) return *this;
					if (_start) _al_tr::deallocate(get_allocator(), _start, _m * _n);
					_m = oth._m; _n = oth._n;
					copy_allocator(oth, typename _al_tr::propagate_on_container_copy_assignment{});
					_start = _al_tr::allocate(get_allocator(), _m * _n);
					return *this;
				}

				BHAVESH_CXX20_CONSTEXPR _compressed_data& operator=(_compressed_data&& oth) {
					if (&oth == this) return *this;
					if (_start) _al_tr::deallocate(get_allocator(), _start, _m * _n);
					_m = oth._m; _n = oth._n;
					move_allocator(std::move(oth), typename _al_tr::propagate_on_container_move_assignment{});
					_start = std::exchange(oth._start, nullptr);
					return *this;
				}

				friend BHAVESH_CXX20_CONSTEXPR void swap(_compressed_data& first, _compressed_data& second) {
					using std::swap;
					using propogate = typename _compressed_data::_al_tr::propagate_on_container_swap;
					swap(first._start, second._start);
					swap(first._m, second._m);
					swap(first._n, second._n);
					first.swap_allocator(second, propogate{});
				}

				BHAVESH_CXX20_CONSTEXPR void copy_allocator(const _compressed_data& oth, std::true_type) noexcept { this->get_allocator() = oth.get_allocator(); }
				BHAVESH_CXX20_CONSTEXPR void copy_allocator(const _compressed_data& oth, std::false_type) noexcept { /* noop */ }
				BHAVESH_CXX20_CONSTEXPR void move_allocator(_compressed_data&& oth, std::true_type) noexcept { this->get_allocator() = std::move(oth.get_allocator()); }
				BHAVESH_CXX20_CONSTEXPR void move_allocator(_compressed_data&& oth, std::false_type) noexcept { /* noop */ }
				BHAVESH_CXX20_CONSTEXPR void swap_allocator(_compressed_data& oth, std::true_type) noexcept { using std::swap; swap(this->get_allocator(), oth.get_allocator()); }
				BHAVESH_CXX20_CONSTEXPR void swap_allocator(_compressed_data& oth, std::false_type) noexcept { /* noop */ }

			public:
				BHAVESH_CXX20_CONSTEXPR       Alloc& get_allocator() { return _alloc; }
				BHAVESH_CXX20_CONSTEXPR const Alloc& get_allocator() const { return _alloc; }

			public:
				allocator_type _alloc;
				pointer _start;
				size_type _m, _n;
			};

			struct transpose_t {};
		}
	}

	constexpr detail::matrix::transpose_t transpose{};

	template<typename Ty, typename Alloc = std::allocator<Ty>>
	class matrix {
	private:
		using _al_tr = std::allocator_traits<Alloc>; // shorthand for std::allocator_traits<Alloc>
		using _stored = detail::matrix::_compressed_data<Alloc>; // type of _data which contains all information about a matrix
		using transpose_t = detail::matrix::transpose_t;
		using _size = typename _al_tr::size_type;
		/* calls _reset of object on destruction unless released. */
		struct _ctor_guard {
			matrix* obj;
			_size& _num_constructed;
			void release() { obj = nullptr; }
			~_ctor_guard() {
				if (obj) obj->_reset(_num_constructed);
			}
		};
		/* calls _reset of object on destruction unless released. */
		struct _ctor_guard_transpose {
			matrix* obj;
			_size& _i;
			_size& _j;
			void release() { obj = nullptr; }
			~_ctor_guard_transpose() {
				if (obj) obj->_reset(_i, _j);
			}
		};



		BHAVESH_CXX20_CONSTEXPR void _reset(_size s) { /* regular construction; destructor of _stored should be automatically called, freeing the memory. */
			while (s--) {
				_al_tr::destroy(_data.get_allocator(), _data._start + s);
			}
		}
		BHAVESH_CXX20_CONSTEXPR void _reset(const _size x, const _size y) { /* transpose construction; destructor of _stored should be automatically called, freeing the memory. */
			const _size _m = _data._m;
			for (_size i = _size(); i != x; ++i) {
				for (_size j = _size(); j != y + 1; ++j) {
					_al_tr::destroy(_data.get_allocator(), _data._start + i * _data._n + j);
				}
			}
			for (_size i = x; i != _m; ++i) {
				for (_size j = _size(); j != y + 1; ++j) {
					_al_tr::destroy(_data.get_allocator(), _data._start + i * _data._n + j);
				}
			}
		}

	public:
		using value_type = Ty;
		using allocator_type = Alloc;

		using size_type = typename _al_tr::size_type;
		using difference_type = typename _al_tr::difference_type;

		using reference = Ty&;
		using const_reference = const Ty&;

		using pointer = typename _al_tr::pointer;
		using const_pointer = typename _al_tr::const_pointer;

		using iterator = Ty*;
		using const_iterator = const Ty*;

		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	public:
		BHAVESH_CXX20_CONSTEXPR matrix() noexcept(std::is_nothrow_default_constructible<Alloc>::value && std::is_nothrow_destructible<Alloc>::value) : matrix(Alloc()) {}
		explicit BHAVESH_CXX20_CONSTEXPR matrix(const allocator_type& alloc) : _data(alloc) {}
		
		BHAVESH_CXX20_CONSTEXPR matrix(matrix&& mve) noexcept : _data(std::move(mve._data)) {}
		BHAVESH_CXX20_CONSTEXPR matrix(matrix&& mve, const typename detail::matrix::type_identity<Alloc>::type& alloc) noexcept : _data(alloc, mve._data._m, mve._data._n) { _data._start = std::exchange(mve._data._start, nullptr); }

		BHAVESH_CXX20_CONSTEXPR matrix(const matrix& oth) : _data(oth._data) { _copy_from_matrix(oth.data()); }
		template<typename Uy, typename Ualloc>
		BHAVESH_CXX20_CONSTEXPR matrix(const matrix<Uy, Ualloc>& oth, const allocator_type& alloc = Alloc()) : _data(alloc, oth.rows(), oth.cols()) { _copy_from_matrix(oth.data()); }

		BHAVESH_CXX20_CONSTEXPR matrix(const matrix& oth, transpose_t) : _data(oth._data) {
			using std::swap;
			swap(_data._m, _data._n); /* swappable */
			_copy_from_matrix_transpose(oth.data());
		}
		template<typename Uy, typename Ualloc>
		BHAVESH_CXX20_CONSTEXPR matrix(const matrix<Uy, Ualloc>& oth, transpose_t, const allocator_type& alloc = Alloc()) : _data(alloc, oth.cols(), oth.rows()) { _copy_from_matrix_transpose(oth.data()); }

		BHAVESH_CXX20_CONSTEXPR matrix(size_type M, size_type N, const allocator_type& alloc = Alloc()) : _data(alloc, M, N) { _fill_default(); }
		BHAVESH_CXX20_CONSTEXPR matrix(size_type M, size_type N, reference value, const allocator_type& alloc = Alloc()) : _data(alloc, M, N) { _fill_value(value); }

		template<typename Iterator, typename Sentinel, typename silence, typename=std::enable_if_t<bhavesh::is_silence_type_v<silence>>>
		BHAVESH_CXX20_CONSTEXPR matrix(size_type M, size_type N, Iterator first, Sentinel last, silence, const allocator_type& alloc = Alloc()) : _data(alloc, M, N) { _copy_from<silence::value>(first, last, detail::matrix::is_sized<Iterator, Sentinel>{}); }
		template<typename Iterator, typename Sentinel>
		BHAVESH_CXX20_CONSTEXPR matrix(size_type M, size_type N, Iterator first, Sentinel last, const allocator_type& alloc = Alloc()) : _data(alloc, M, N) { _copy_from<bhavesh::silence_none>(first, last, detail::matrix::is_sized<Iterator, Sentinel>{}); }
		template<typename Iterator, typename Sentinel, typename silence, typename = std::enable_if_t<bhavesh::is_silence_type_v<silence>>>
		BHAVESH_CXX20_CONSTEXPR matrix(size_type M, size_type N, Iterator first, Sentinel last, silence, transpose_t, const allocator_type& alloc = Alloc()) : _data(alloc, M, N) { _copy_from_transpose<silence::value>(first, last, detail::matrix::is_sized<Iterator, Sentinel>{}); }
		template<typename Iterator, typename Sentinel, typename silence, typename = std::enable_if_t<bhavesh::is_silence_type_v<silence>>>
		BHAVESH_CXX20_CONSTEXPR matrix(size_type M, size_type N, Iterator first, Sentinel last, transpose_t, silence, const allocator_type& alloc = Alloc()) : _data(alloc, M, N) { _copy_from_transpose<silence::value>(first, last, detail::matrix::is_sized<Iterator, Sentinel>{}); }
		template<typename Iterator, typename Sentinel>
		BHAVESH_CXX20_CONSTEXPR matrix(size_type M, size_type N, Iterator first, Sentinel last, transpose_t, const allocator_type& alloc = Alloc()) : _data(alloc, M, N) { _copy_from_transpose<bhavesh::silence_none>(first, last, detail::matrix::is_sized<Iterator, Sentinel>{}); }


		template<typename... tArgs> /* forwards to iterator-pair constructors since they do the right thing */
		BHAVESH_CXX20_CONSTEXPR matrix(size_type M, size_type N, std::initializer_list<Ty> il, tArgs&& ...args) : matrix(M, N, il.begin(), il.end(), std::forward<tArgs>(args)...) {}

		template<typename... tArgs> /* when we know shape as a pair; forwards to other constructors */
		BHAVESH_CXX20_CONSTEXPR matrix(std::pair<size_type, size_type> shape, tArgs&&... args) : matrix(shape.first, shape.second, std::forward<tArgs>(args)...) {}

		BHAVESH_CXX20_CONSTEXPR ~matrix() {
			if (_data._start) _reset(size());
		}
	private: /* constructor helpers */
		template<typename Pointer>
		BHAVESH_CXX20_CONSTEXPR void _copy_from_matrix(Pointer data) {
			size_type s = size_type();
			_ctor_guard _guard{ this, s }; // RAII for the win!
			const size_type _max = size();
			for (; s < _max; ++s) {
				_al_tr::construct(_data.get_allocator(), _data._start + s, data[s]);
			}
		}

		template<typename Pointer>
		BHAVESH_CXX20_CONSTEXPR void _copy_from_matrix_transpose(Pointer data) {
			_size i = _size(), j = _size();
			_ctor_guard_transpose _guard{ this, i, j }; // RAII for the win!
			const size_type _m = _data._m, _n = _data._n;
			for (size_type j = size_type(); j < _n; ++j) {
				for (size_type i = size_type(); i < _m; ++i) {
					_al_tr::construct(_data.get_allocator(), _data._start + i * _n + j, data[j * _m + i]);
				}
			}
		}

		BHAVESH_CXX20_CONSTEXPR void _fill_default() {
			_size s = _size();
			_ctor_guard _guard{ this, s };
			const _size _max = size();
			for (; s != _max; ++s) {
				_al_tr::construct(_data.get_allocator(), _data._start + s);
			}
		}

		BHAVESH_CXX20_CONSTEXPR void _fill_value(const Ty& _value) {
			_size s = _size();
			_ctor_guard _guard{ this, s };
			const _size _max = size();
			for (; s != _max; ++s) {
				_al_tr::construct(_data.get_allocator(), _data._start + s, _value);
			}
		}

		template<silence_t sil, typename Iterator, typename Sentinel>
		BHAVESH_CXX20_CONSTEXPR void _copy_from(Iterator first, Sentinel last, std::true_type /* is-sized */) {
			const _size length = static_cast<_size>(last - first);
			const _size _max = size();
			if BHAVESH_CXX17_CONSTEXPR((sil & bhavesh::silence_less) == 0) if (length < _max) throw std::invalid_argument( "Too few elements given to matrix(m, n, ...)");
			if BHAVESH_CXX17_CONSTEXPR((sil & bhavesh::silence_more) == 0) if (length > _max) throw std::invalid_argument("Too many elements given to matrix(m, n, ...)");
			_size i = _size();
			_ctor_guard _guard{ this, i };
			for (; first != last && i != _max; ++i, ++first) {
				_al_tr::construct(_data.get_allocator(), _data._start + i, *first);
			}
			for (; i != _max; ++i) {
				_al_tr::construct(_data.get_allocator(), _data._start + i);
			}
		}

		template<silence_t sil, typename Iterator, typename Sentinel>
		BHAVESH_CXX20_CONSTEXPR void _copy_from(Iterator first, Sentinel last, std::false_type /* !is-sized */) {
			_size i = _size();
			_ctor_guard _guard{ this, i };
			const _size _max = size();
			for (;; ++i, ++first) {
				_al_tr::construct(_data.get_allocator(), _data._start + i, *first);
				if (first == last) {
					if (i == _max) return;
					if BHAVESH_CXX17_CONSTEXPR((sil & bhavesh::silence_less) == 0) throw std::invalid_argument("Too few elements given to matrix(m, n, ...)");
					for (; i != _max; ++i) {
						_al_tr::construct(_data.get_allocator(), _data._start + i);
					}
					return;
				}
				if (i == _max) {
					if BHAVESH_CXX17_CONSTEXPR((sil & bhavesh::silence_more) == 0) throw std::invalid_argument("Too many elements given to matrix(m, n, ...)");
					return;
				}
			}
		}

		template<silence_t sil, typename Iterator, typename Sentinel>
		BHAVESH_CXX20_CONSTEXPR void _copy_from_transpose(Iterator first, Sentinel last, std::true_type /* is-sized */) {
			const _size length = static_cast<_size>(last - first);
			const _size _i = _data._m;
			const _size _j = _data._n;
			if BHAVESH_CXX17_CONSTEXPR((sil & bhavesh::silence_less) == 0) if (length < size()) throw std::invalid_argument( "Too few elements given to matrix(m, n, ...)");
			if BHAVESH_CXX17_CONSTEXPR((sil & bhavesh::silence_more) == 0) if (length > size()) throw std::invalid_argument("Too many elements given to matrix(m, n, ...)");
			_size i = _size();
			_size j = _size();
			_ctor_guard_transpose _guard{ this, i, j };
			for (; j != _j; ++j) {
				for (i = _size(); i != _i; ++i, ++first) {
					if (first == last) {
						for (; i != _i; ++i) { _al_tr::construct(_data.get_allocator(), _data._start + i * _j + j); } // finish that column
						for (++j; j != _j; ++j) { // finish rest of the columns
							for (i = _size(); i != _i; ++i) {
								_al_tr::construct(_data.get_allocator(), _data._start + i * _j + j);
							}
						}
						return;
					}
					_al_tr::construct(_data.get_allocator(), _data._start + i * _j + j, *first);
				}
			}
		}

		template<silence_t sil, typename Iterator, typename Sentinel>
		BHAVESH_CXX20_CONSTEXPR void _copy_from_transpose(Iterator first, Sentinel last, std::false_type /* !is-sized */) {
			_size i = _size();
			_size j = _size();
			_ctor_guard_transpose _guard{ this, i, j };
			const _size _i = _data._m;
			const _size _j = _data._n;
			for (; j != _j; ++j) {
				for (i = _size(); i != _i; ++i) {
					if (first != last) BHAVESH_USE_IF_CXX20([[likely]]) {
						_al_tr::construct(_data.get_allocator(), _data._start + i * _j + j, *first);
						continue;
					}
					if BHAVESH_CXX17_CONSTEXPR((sil & bhavesh::silence_less) == 0) throw std::invalid_argument("Too few elements given to matrix(m, n, ...)");
					for (; i != _i; ++i) { _al_tr::construct(_data.get_allocator(), _data._start + i * _j + j); } // finish that column
					for (; j != j; ++j) {
						for (i = _size(); i != _i; ++i) {
							_al_tr::construct(_data.get_allocator(), _data._start + i * _j + j);
						}
					}
					return;
				}
			}
			if BHAVESH_CXX17_CONSTEXPR((sil & bhavesh::silence_more) == 0) if (first != last/* not exhausted */) throw std::invalid_argument("Too many elements given to matrix(m, n, ...)");
		}

	public:
		BHAVESH_CXX20_CONSTEXPR pointer data() {
			return _data._start;
		}
		BHAVESH_CXX20_CONSTEXPR const_pointer data() const {
			return _data._start;
		}

		BHAVESH_CXX20_CONSTEXPR size_type rows() const {
			return _data._m;
		}
		BHAVESH_CXX20_CONSTEXPR size_type cols() const {
			return _data._n;
		}

		BHAVESH_CXX20_CONSTEXPR size_type size() const {
			return _data._m * _data._n;
		}

		BHAVESH_CXX20_CONSTEXPR std::pair<size_type, size_type> shape() const {
			return { _data._m, _data._n };
		}

		BHAVESH_CXX20_CONSTEXPR bool empty() const {
			return size() == size_type();
		}
	private:
		_stored _data;

	};

#if BHAVESH_CXX17
	namespace pmr {
		template<typename Ty>
		using matrix = bhavesh::matrix<Ty, std::pmr::polymorphic_allocator<Ty>>;
	}
#endif
}