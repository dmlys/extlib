#pragma once
#include <bitset>
#include <type_traits>

namespace ext
{
	template <class Enum, std::size_t Size>
	class enum_bitset;

	template <class Enum, Enum Last>
	using enum_bitset_for = enum_bitset<Enum, static_cast<std::size_t>(Last) + 1>;

	/// typed bitset, actually wraps std::bitset,
	/// provides operations taking enum values and std::size_t
	template <class Enum, std::size_t Size>
	class enum_bitset
	{
		typedef enum_bitset self_type;

	public:
		typedef Enum     value_type;
		typedef Enum     enum_type;
		typedef std::underlying_type_t<enum_type> index_type;

		typedef std::bitset<Size> bitset_type;
		typedef typename bitset_type::reference   reference;

	private:
		bitset_type m_bitset;

	public:
		      bitset_type & underlaying()       noexcept { return m_bitset; }
		const bitset_type & underlaying() const noexcept { return m_bitset; }

	public:
		constexpr std::size_t size() const noexcept { return m_bitset.size(); }
		bool test(index_type pos) const { return m_bitset.test(static_cast<std::size_t>(pos)); }
		bool test(enum_type pos)  const { return test(static_cast<index_type>(pos)); }

		constexpr bool operator[](index_type pos) const noexcept { return m_bitset[static_cast<std::size_t>(pos)]; }
		     reference operator[](index_type pos)       noexcept { return m_bitset[static_cast<std::size_t>(pos)]; }

		constexpr bool operator[](enum_type pos) const noexcept { return operator [](static_cast<index_type>(pos)); }
		     reference operator[](enum_type pos)       noexcept { return operator [](static_cast<index_type>(pos)); }

	public:
		self_type & set(index_type pos, bool val = true)  { m_bitset.set(static_cast<std::size_t>(pos), val); return *this; }
		self_type & reset(index_type pos)                 { m_bitset.reset(static_cast<std::size_t>(pos)); return *this; }
		self_type & flip(index_type pos)                  { m_bitset.flip(static_cast<std::size_t>(pos)); return *this; }
		
		self_type & set()   noexcept { m_bitset.set(); return *this; }
		self_type & reset() noexcept { m_bitset.reset(); return *this; }
		self_type & flip()  noexcept { m_bitset.flip(); return *this; }

	public:
		bool all()  const noexcept { return m_bitset.all(); }
		bool any()  const noexcept { return m_bitset.any(); }
		bool none() const noexcept { return m_bitset.none(); }
		
		std::size_t count() const noexcept { return m_bitset.count(); }

	public:
		self_type   operator ~ () noexcept { ~m_bitset return *this; }
		self_type & operator &= (const self_type & other) noexcept { m_bitset &= other.m_bitset; return *this; }
		self_type & operator |= (const self_type & other) noexcept { m_bitset |= other.m_bitset; return *this; }
		self_type & operator ^= (const self_type & other) noexcept { m_bitset ^= other.m_bitset; return *this; }
		self_type & operator <<= (const self_type & other) noexcept { m_bitset <<= other.m_bitset; return *this; }
		self_type & operator >>= (const self_type & other) noexcept { m_bitset >>= other.m_bitset; return *this; }

	public:
		template <class Char = char, class CharTraits = std::char_traits<Char>, class Alloc = std::allocator<Char>>
		std::basic_string<Char, CharTraits, Alloc> to_string() const
		{ return m_bitset.to_string<Char, CharTraits, Alloc>>(); }

		template <class Char = char, class CharTraits = std::char_traits<Char>, class Alloc = std::allocator<Char>>
		std::basic_string<Char, CharTraits, Alloc> to_string(Char zero = Char('0'), Char one = Char('1')) const
		{ return m_bitset.to_string<Char, CharTraits, Alloc >>(zero, one); }

		unsigned long to_ulong()  const { return m_bitset.to_ulong(); }
		unsigned long to_ullong() const { return m_bitset.to_ullong(); }

	public:
		constexpr enum_bitset() = default;
		constexpr enum_bitset(unsigned long long val) : m_bitset(val) {}


		template <class Char, class Traits, class Alloc>
		explicit enum_bitset( 
			const std::basic_string<Char, Traits, Alloc> & str,
            typename std::basic_string<Char, Traits, Alloc>::size_type pos = 0,
            typename std::basic_string<Char, Traits, Alloc>::size_type n = std::basic_string<Char, Traits, Alloc>::npos,
			Char zero = Char('0'),
            Char one = Char('1')
		) : m_bitset(str, pos, n, zero, one) {}

		template <class Char>
		explicit enum_bitset(
			const Char * str,
            typename std::basic_string<Char>::size_type n = std::basic_string<Char>::npos,
            Char zero = Char('0'),
            Char one = Char('1')
		) : m_bitset(str, npos, zero, one) {}
	};

	template <class Enum, std::size_t Size>
	inline auto operator &(const enum_bitset<Enum, Size> & lhs, const enum_bitset<Enum, Size> & rhs)
	{
		return enum_bitset<Enum, Size>(lhs) &= rhs;
	}

	template <class Enum, std::size_t Size>
	inline auto operator |(const enum_bitset<Enum, Size> & lhs, const enum_bitset<Enum, Size> & rhs)
	{
		return enum_bitset<Enum, Size>(lhs) |= rhs;
	}

	template <class Enum, std::size_t Size>
	inline auto operator ^(const enum_bitset<Enum, Size> & lhs, const enum_bitset<Enum, Size> & rhs)
	{
		return enum_bitset<Enum, Size>(lhs) ^= rhs;
	}

	template <class Enum, std::size_t Size>
	inline auto operator <<(const enum_bitset<Enum, Size> & lhs, const enum_bitset<Enum, Size> & rhs)
	{
		return enum_bitset<Enum, Size>(lhs) <<= rhs;
	}

	template <class Enum, std::size_t Size>
	inline auto operator >>(const enum_bitset<Enum, Size> & lhs, const enum_bitset<Enum, Size> & rhs)
	{
		return enum_bitset<Enum, Size>(lhs) >>= rhs;
	}

	template <class Enum, std::size_t Size>
	inline bool operator == (const enum_bitset<Enum, Size> & lhs, const enum_bitset<Enum, Size> & rhs)
	{
		return lhs.underlaying() == rhs.underlaying();
	}

	template <class Enum, std::size_t Size>
	inline bool operator != (const enum_bitset<Enum, Size> & lhs, const enum_bitset<Enum, Size> & rhs)
	{
		return lhs.underlaying() != rhs.underlaying();
	}

	
	template <class Char, class Traits, class Enum, std::size_t Size>
	inline std::basic_ostream<Char, Traits> & operator <<(std::basic_ostream<Char, Traits> & os, const enum_bitset<Enum, Size> & bs)
	{
		return os << bs.underlaying();
	}

	template <class Char, class Traits, class Enum, std::size_t Size>
	inline std::basic_istream<Char, Traits> & operator <<(std::basic_istream<Char, Traits> & is, const enum_bitset<Enum, Size> & bs)
	{
		return is >> bs.underlaying();
	}
}

namespace std
{
	template <class Enum, std::size_t Size>
	struct hash<ext::enum_bitset<Enum, Size>>
	{
		constexpr std::size_t operator()(const ext::enum_bitset<Enum, Size> & bs) const noexcept
		{
			typedef ext::enum_bitset<Enum, Size> typed_bitset;
			return std::hash<typename typed_bitset::bitset_type>{}(bs.underlaying());
		}
	};
}
