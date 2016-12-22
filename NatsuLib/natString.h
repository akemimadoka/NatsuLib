#pragma once
#include "natConfig.h"
#include "natType.h"
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <cassert>
#include <algorithm>
#include <iterator>

#ifdef _MSC_VER
#pragma push_macro("max")
#undef max
#endif

namespace NatsuLib
{
	enum class StringType
	{
		Utf8,
		Utf16,
		Utf32,

#ifdef _WIN32
		Ansi,
		Wide,
#endif
	};

	template <StringType>
	struct StringEncodingTrait;

	template <>
	struct StringEncodingTrait<StringType::Utf8>
	{
		typedef char CharType;
		enum
		{
			MaxCharSize = 4,	///< @brief	对于此编码，一个有效字符可能占用的最大CharType数
		};

		static size_t GetCharCount(CharType Char) noexcept
		{
			const auto unsignedChar = static_cast<std::make_unsigned_t<CharType>>(Char);
			if (unsignedChar < 0x80)
			{
				return 1;
			}
			if (unsignedChar - 0x80 < 0x40)
			{
				// 错误的编码
				return size_t(-1);
			}
			if (unsignedChar - 0xC0 < 0x20)
			{
				return 2;
			}
			if (unsignedChar - 0xE0 < 0x10)
			{
				return 3;
			}
			if (unsignedChar - 0xF0 < 0x08)
			{
				return 4;
			}
			return size_t(-1);
		}
	};

	template <>
	struct StringEncodingTrait<StringType::Utf16>
	{
		typedef char16_t CharType;
		enum
		{
			MaxCharSize = 1,
		};

		static size_t GetCharCount(CharType /*Char*/) noexcept
		{
			return 1;
		}
	};

	template <>
	struct StringEncodingTrait<StringType::Utf32>
	{
		typedef char32_t CharType;
		enum
		{
			MaxCharSize = 1,
		};

		static size_t GetCharCount(CharType /*Char*/) noexcept
		{
			return 1;
		}
	};

#ifdef _WIN32
	template <>
	struct StringEncodingTrait<StringType::Ansi>
	{
		typedef char CharType;
		enum
		{
			MaxCharSize = 4,
		};

		static size_t GetCharCount(CharType Char) noexcept
		{
			return StringEncodingTrait<StringType::Utf8>::GetCharCount(Char);
		}
	};

	template <>
	struct StringEncodingTrait<StringType::Wide>
	{
		typedef wchar_t CharType;
		enum
		{
			MaxCharSize = 1,
		};

		static size_t GetCharCount(CharType /*Char*/) noexcept
		{
			return 1;
		}
	};
#endif

	namespace detail_
	{
		enum : size_t
		{
			npos = size_t(-1),
			MaxAllocaSize = 0x10000,
		};

		template <typename CharType>
		const CharType* GetEndOfString(const CharType* begin) noexcept
		{
			assert(begin && "begin should not be a nullptr.");

			auto end = begin;
			while (*end != CharType{})
			{
				++end;
			}

			return end;
		}

		template <typename Iter, typename CharType>
		size_t SearchCharRepeat(Iter srcBegin, Iter srcEnd, CharType searchChar, size_t repeatCount) noexcept
		{
			assert(repeatCount != 0);
			assert(static_cast<size_t>(srcEnd - srcBegin) > repeatCount);

			auto current = srcBegin;

			while (true)
			{
				while (true)
				{
					if (srcEnd - current < static_cast<std::ptrdiff_t>(repeatCount))
					{
						return npos;
					}
					if (*current == searchChar)
					{
						break;
					}
					++current;
				}

				std::ptrdiff_t matchLen = 1;
				while (true)
				{
					if (static_cast<size_t>(matchLen) >= repeatCount)
					{
						return static_cast<size_t>(std::distance(srcBegin, current));
					}
					if (current[matchLen] != searchChar)
					{
						break;
					}
					++matchLen;
				}

				current += matchLen;
				++current;
			}
		}

		template <typename Iter1, typename Iter2>
		size_t MatchString(Iter1 srcBegin, Iter1 srcEnd, Iter2 patternBegin, Iter2 patternEnd) noexcept;

		template <StringType stringType>
		std::enable_if_t<StringEncodingTrait<stringType>::MaxCharSize == 1, size_t> GetCharCount(const typename StringEncodingTrait<stringType>::CharType* /*str*/, size_t length)
		{
			return length;
		}

		template <StringType stringType>
		std::enable_if_t<StringEncodingTrait<stringType>::MaxCharSize != 1, size_t> GetCharCount(const typename StringEncodingTrait<stringType>::CharType* str, size_t length)
		{
			size_t count{};
			size_t currentSize;
			for (size_t i = 0; i < length; )
			{
				currentSize = StringEncodingTrait<stringType>::GetCharCount(str[i]);
				i += currentSize;
				count += currentSize;
			}

			return count;
		}

		[[noreturn]] void IndexOutOfRange();
		[[noreturn]] void SizeOutOfRange();
	}

	enum class EncodingResult
	{
		Accept,
		Reject,
		Incomplete,
	};

	std::tuple<EncodingResult, char32_t, const char*> DecodeUtf8(const char* strBegin, const char* strEnd) noexcept;
	std::tuple<EncodingResult, char32_t, const char16_t*> DecodeUtf16(const char16_t* strBegin, const char16_t* strEnd) noexcept;
	std::tuple<EncodingResult, char32_t, const char32_t*> DecodeUtf32(const char32_t* strBegin, const char32_t* strEnd) noexcept;
	std::pair<EncodingResult, char*> EncodeUtf8(char* strBegin, const char* strEnd, char32_t input) noexcept;
	std::pair<EncodingResult, char16_t*> EncodeUtf16(char16_t* strBegin, const char16_t* strEnd, char32_t input) noexcept;
	std::pair<EncodingResult, char32_t*> EncodeUtf32(char32_t* strBegin, const char32_t* strEnd, char32_t input) noexcept;

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	字符串视图
	///	@tparam	stringType	字符串的编码
	///	@note	表示字符串的原始表示
	////////////////////////////////////////////////////////////////////////////////
	template <StringType stringType>
	class StringView
	{
	public:
		typedef typename StringEncodingTrait<stringType>::CharType CharType;
		typedef CharType Element;
		typedef const CharType* CharIterator;
		typedef const CharType* const_iterator;
		typedef const_iterator iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef const_reverse_iterator reverse_iterator;

		enum : size_t
		{
			npos = detail_::npos
		};

		constexpr StringView() noexcept
			: m_StrBegin(nullptr), m_StrEnd(nullptr)
		{
		}

		constexpr StringView(CharIterator begin, CharIterator end) noexcept
			: m_StrBegin(begin), m_StrEnd(end)
		{
		}

		StringView(nullptr_t, nullptr_t = nullptr) noexcept
			: StringView()
		{
		}

		StringView(CharIterator begin, size_t length) noexcept
			: StringView(begin, begin + length)
		{
		}

		StringView(std::initializer_list<CharType> il) noexcept
			: StringView(il.begin(), il.end())
		{
		}

		StringView(CharIterator begin) noexcept
			: StringView(begin, detail_::GetEndOfString(begin))
		{
		}

		template <size_t N>
		StringView(const CharType (&array)[N])
			: StringView(array, array + N)
		{
		}

		~StringView()
		{
		}

		bool IsEmpty() const noexcept
		{
			return m_StrBegin == m_StrEnd;
		}

		void Clear() noexcept
		{
			m_StrBegin = m_StrEnd;
		}

		const_iterator begin() const noexcept
		{
			return m_StrBegin;
		}

		const_iterator end() const noexcept
		{
			return m_StrEnd;
		}

		const_iterator cbegin() const noexcept
		{
			return m_StrBegin;
		}

		const_iterator cend() const noexcept
		{
			return m_StrEnd;
		}

		const_reverse_iterator rbegin() const noexcept
		{
			return const_reverse_iterator{ std::prev(m_StrEnd) };
		}

		const_reverse_iterator rend() const noexcept
		{
			return const_reverse_iterator{ std::prev(m_StrBegin) };
		}

		const_reverse_iterator crbegin() const noexcept
		{
			return const_reverse_iterator{ std::prev(m_StrEnd) };
		}

		const_reverse_iterator crend() const noexcept
		{
			return const_reverse_iterator{ std::prev(m_StrBegin) };
		}

		size_t size() const noexcept
		{
			return GetSize();
		}

		bool empty() const noexcept
		{
			return m_StrBegin == m_StrEnd;
		}

		CharIterator data() const noexcept
		{
			return m_StrBegin;
		}

		const CharType& operator[](size_t pos) const noexcept
		{
			return UncheckGet(pos);
		}

		bool operator==(StringView const& other) const noexcept
		{
			if (GetSize() != other.GetSize())
			{
				return false;
			}

			return Compare(other) == 0;
		}

		bool operator!=(StringView const& other) const noexcept
		{
			if (GetSize() != other.GetSize())
			{
				return true;
			}

			return Compare(other) != 0;
		}

		bool operator<(StringView const& other) const noexcept
		{
			return Compare(other) < 0;
		}

		bool operator>(StringView const& other) const noexcept
		{
			return Compare(other) > 0;
		}

		bool operator<=(StringView const& other) const noexcept
		{
			return Compare(other) <= 0;
		}

		bool operator>=(StringView const& other) const noexcept
		{
			return Compare(other) >= 0;
		}

		size_t GetSize() const noexcept
		{
			return m_StrEnd - m_StrBegin;
		}

		size_t GetCharCount() const noexcept
		{
			return detail_::GetCharCount<stringType>(m_StrBegin, m_StrEnd - m_StrBegin);
		}

		void Swap(StringView& other) noexcept
		{
			using std::swap;
			swap(m_StrBegin, other.m_StrBegin);
			swap(m_StrEnd, other.m_StrEnd);
		}

		const CharType& Get(size_t index) const
		{
			if (index >= GetSize())
			{
				detail_::IndexOutOfRange();
			}

			return UncheckGet(index);
		}

		const CharType& UncheckGet(size_t pos) const noexcept
		{
			assert(pos < GetSize());

			return begin()[pos];
		}

		nInt Compare(StringView const& other) const noexcept
		{
			typedef std::make_unsigned_t<CharType> UChar;

			auto read = begin();
			const auto readEnd = end();
			auto otherRead = other.begin();
			const auto otherEnd = other.end();

			while (read != readEnd && otherRead != otherEnd)
			{
				const auto elem = static_cast<UChar>(*read);
				const auto otherElem = static_cast<UChar>(*otherRead);
				if (elem != otherElem)
				{
					return elem > otherElem ? 1 : -1;
				}
				++read; ++otherRead;
			}

			return 0;
		}

		StringView Slice(std::ptrdiff_t begin, std::ptrdiff_t end) const noexcept
		{
			const auto size = GetSize();
			return { m_StrBegin + ApplyOffset(begin, size), m_StrBegin + ApplyOffset(end, size) };
		}

		///	@brief	字符串分割函数
		///	@param[in]	pattern		分割字符
		///	@param[in]	callableObject	可被调用的对象，接受参数为StringView
		template <typename CallableObject>
		void Split(StringView const& pattern, CallableObject callableObject) const noexcept
		{
			size_t pos{};
			const auto strLen = size();

			for (size_t i = 0; i < strLen; ++i)
			{
				const auto currentchar = m_StrBegin[i];
				for (auto&& item : pattern)
				{
					if (currentchar == item)
					{
						callableObject(StringView{ m_StrBegin + pos, i - pos });

						pos = i + 1;
						break;
					}
				}
			}

			if (pos != strLen)
			{
				callableObject(StringView{ m_StrBegin + pos, strLen - pos });
			}
		}

		void Assign(CharIterator begin, CharIterator end) noexcept
		{
			m_StrBegin = begin;
			m_StrEnd = end;
		}

		void Assign(std::nullptr_t, std::nullptr_t = nullptr) noexcept
		{
			m_StrBegin = nullptr;
			m_StrEnd = nullptr;
		}

		void Assign(CharIterator begin, size_t length) noexcept
		{
			Assign(begin, begin + length);
		}

		void Assign(std::initializer_list<CharType> il) noexcept
		{
			Assign(il.begin(), il.end());
		}

		void Assign(CharIterator begin) noexcept
		{
			Assign(begin, detail_::GetEndOfString(begin));
		}

		size_t Find(StringView const& pattern, std::ptrdiff_t nBegin = 0) const noexcept
		{
			const auto length = GetSize();
			const auto realBegin = ApplyOffset(nBegin, length);
			const auto patternLength = pattern.GetSize();
			if (patternLength == 0)
			{
				return realBegin;
			}
			if (length < patternLength || realBegin + patternLength > length)
			{
				return npos;
			}
			const auto pos = detail_::MatchString(cbegin() + realBegin, cend(), pattern.cbegin(), pattern.cend());
			if (pos == npos)
			{
				return npos;
			}
			return pos + realBegin;
		}

		size_t FindBackward(StringView const& pattern, std::ptrdiff_t nEnd = -1) const noexcept
		{
			const auto length = GetSize();
			const auto realEnd = ApplyOffset(nEnd, length);
			const auto patternLength = pattern.GetSize();
			if (patternLength == 0)
			{
				return realEnd;
			}
			if (length < patternLength || realEnd < length)
			{
				return npos;
			}
			const auto pos = detail_::MatchString(const_reverse_iterator{ cbegin() + realEnd }, crend(), pattern.cbegin(), pattern.cend());
			if (pos == npos)
			{
				return npos;
			}
			return realEnd - pos - patternLength;
		}

		size_t FindCharRepeat(CharType findChar, size_t repeatCount, std::ptrdiff_t nBegin = 0) const noexcept
		{
			const auto length = GetSize();
			const auto realBegin = ApplyOffset(nBegin, length);
			if (repeatCount == 0)
			{
				return realBegin;
			}
			if (length < repeatCount || realBegin + repeatCount > length)
			{
				return npos;
			}
			const auto pos = detail_::SearchCharRepeat(cbegin() + realBegin, cend(), findChar, repeatCount);
			if (pos == npos)
			{
				return npos;
			}
			return pos + realBegin;
		}

		size_t FindCharRepeatBackward(CharType findChar, size_t repeatCount, std::ptrdiff_t nEnd = 0) const noexcept
		{
			const auto length = GetSize();
			const auto realEnd = ApplyOffset(nEnd, length);
			if (repeatCount == 0)
			{
				return realEnd;
			}
			if (length < repeatCount || realEnd < repeatCount)
			{
				return npos;
			}
			const auto pos = detail_::SearchCharRepeat(const_reverse_iterator{ cbegin() + realEnd }, crend(), findChar, repeatCount);
			if (pos == npos)
			{
				return npos;
			}
			return realEnd - pos - repeatCount;
		}

		size_t Find(CharType findChar, std::ptrdiff_t nBegin = 0) const noexcept
		{
			return FindCharRepeat(findChar, 1, nBegin);
		}

		size_t FindBackward(CharType findChar, std::ptrdiff_t nEnd = -1) const noexcept
		{
			return FindCharRepeatBackward(findChar, 1, nEnd);
		}

		bool DoesOverlapWith(StringView const& other) const noexcept
		{
			return m_StrBegin < other.m_StrEnd && m_StrEnd > other.m_StrBegin;
		}

	private:
		CharIterator m_StrBegin;
		CharIterator m_StrEnd;

		static size_t ApplyOffset(std::ptrdiff_t offset, size_t size) noexcept
		{
			auto ret = static_cast<size_t>(offset);
			if (offset < 0)
			{
				ret += size + 1;
			}

			assert(ret <= size);
			return ret;
		}
	};

	extern template class StringView<StringType::Utf8>;
	extern template class StringView<StringType::Utf16>;
	extern template class StringView<StringType::Utf32>;

#ifdef _WIN32
	extern template class StringView<StringType::Ansi>;
	extern template class StringView<StringType::Wide>;
#endif

	namespace detail_
	{
		template <StringType SrcType>
		struct TransCoder;

		constexpr size_t Grow(size_t size) noexcept
		{
			return std::max(size + 1, (size + 1 + ((size + 1) >> 1) + 0x0F) & static_cast<size_t>(-0x10));
		}

		template <typename CharType, size_t ArrayMaxSize>
		struct StringStorage
		{
			constexpr StringStorage() noexcept
				: Size{}, Capacity{ ArrayMaxSize }, Pointer{}
			{
			}

			explicit StringStorage(size_t capacity)
				: StringStorage{}
			{
				Reserve(capacity);
			}
			
			StringStorage(const CharType* str, size_t length)
				: StringStorage{}
			{
				Assign(str, length);
			}

			StringStorage(StringStorage const& other)
				: StringStorage{}
			{
				Assign(other);
			}

			StringStorage(StringStorage && other) noexcept
				: StringStorage{}
			{
				Assign(std::move(other));
			}

			template <size_t OtherArrayMaxSize>
			StringStorage(StringStorage<CharType, OtherArrayMaxSize> const& other)
				: StringStorage{}
			{
				Assign(other);
			}

			template <size_t OtherArrayMaxSize>
			StringStorage(StringStorage<CharType, OtherArrayMaxSize>&& other) noexcept
				: StringStorage{}
			{
				Assign(std::move(other));
			}

			~StringStorage()
			{
				if (Capacity > ArrayMaxSize)
				{
					delete[] Pointer;
				}
			}

			template <size_t OtherArrayMaxSize>
			StringStorage& operator=(StringStorage<CharType, OtherArrayMaxSize> const& other)
			{
				Assign(other);
				return *this;
			}

			template <size_t OtherArrayMaxSize>
			StringStorage& operator=(StringStorage<CharType, OtherArrayMaxSize>&& other) noexcept
			{
				Assign(std::move(other));
				return *this;
			}

			void Assign(const CharType* str, size_t length)
			{
				Resize(length);
				std::memmove(GetData(), str, length * sizeof(CharType));
			}

			template <size_t OtherArrayMaxSize>
			void Assign(StringStorage<CharType, OtherArrayMaxSize> const& other)
			{
				Assign(other.GetData(), other.Size);
			}

			template <size_t OtherArrayMaxSize>
			void Assign(StringStorage<CharType, OtherArrayMaxSize>&& other) noexcept
			{
				using std::swap;

				if (other.Capacity > OtherArrayMaxSize && other.Size > ArrayMaxSize)
				{
					swap(Pointer, other.Pointer);
					swap(Capacity, other.Capacity);
					swap(Size, other.Size);
				}
				else
				{
					Assign(other.GetData(), other.Size);
				}
			}

			const CharType* GetData() const noexcept
			{
				if (Capacity > ArrayMaxSize)
				{
					return Pointer;
				}

				return Array;
			}

			CharType* GetData() noexcept
			{
				return const_cast<CharType*>(static_cast<const StringStorage*>(this)->GetData());
			}

			void Reserve(size_t newCapacity);

			void Resize(size_t newSize)
			{
				if (newSize >= std::max(ArrayMaxSize, Capacity))
				{
					Reserve(Grow(newSize));
				}

				for (auto i = Size; i < newSize; ++i)
				{
					GetData()[i] = CharType{};
				}
				Size = newSize;
				GetData()[Size] = CharType{};

				assert(Capacity > Size);
			}

			size_t Size;
			size_t Capacity;
			union
			{
				CharType Array[ArrayMaxSize];
				CharType* Pointer;
			};
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	字符串
	///	@tparam	stringType	字符串编码
	///	@note	保存字符串的原始表示
	////////////////////////////////////////////////////////////////////////////////
	template <StringType stringType>
	class String
	{
	public:
		typedef StringView<stringType> View;
		typedef typename View::CharType CharType;
		typedef CharType Element;
		typedef CharType* iterator;
		typedef const CharType* const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		
		enum : size_t
		{
			npos = detail_::npos,
			MaxShortStringSize = 31,
		};

		static void TransAppendTo(String<StringType::Utf16>& dst, View const& src);
		static void TransAppendFrom(String& dst, StringView<StringType::Utf16> const& src);

		static void TransAppendTo(String<StringType::Utf32>& dst, View const& src);
		static void TransAppendFrom(String& dst, StringView<StringType::Utf32> const& src);

		String()
			: m_Storage{}
		{
		}

		explicit String(CharType Char, size_t count = 1)
			: String{}
		{
			Assign(Char, count);
		}

		String(View const& view)
			: String{}
		{
			Assign(view);
		}

		String& operator=(View const& view)
		{
			Assign(view);
			return *this;
		}

		String(String const& other)
			: String{}
		{
			Assign(other);
		}

		String(String&& other) noexcept
			: String{}
		{
			Assign(std::move(other));
		}

		String& operator=(String const& other)
		{
			Assign(other);
			return *this;
		}

		String& operator=(String&& other) noexcept
		{
			Assign(std::move(other));
			return *this;
		}

		template <StringType srcType>
		String(StringView<srcType> const& other)
			: String{}
		{
			Assign(other);
		}

		template <StringType srcType>
		String& operator=(StringView<srcType> const& other)
		{
			Assign(other);
			return *this;
		}

		template <StringType srcType>
		String(String<srcType> const& other)
			: String{ other.GetView() }
		{
		}

		template <StringType srcType>
		String& operator=(String<srcType> const& other)
		{
			return *this = other.GetView();
		}

		String(const CharType* str)
			: String{}
		{
			Assign(View{ str });
		}

		~String()
		{
		}

		void Reserve(size_t newCapacity)
		{
			m_Storage.Reserve(newCapacity);
		}

		void Resize(size_t newSize)
		{
			m_Storage.Resize(newSize);
		}

		iterator ResizeMore(size_t moreSize)
		{
			const auto oldSize = m_Storage.Size;
			Resize(oldSize + moreSize);
			return m_Storage.GetData() + oldSize;
		}

		void Assign(CharType Char, size_t count = 1)
		{
			Resize(count);
			std::fill_n(m_Storage.GetData(), count, Char);
		}

		void Assign(View const& view)
		{
			Resize(view.size());

#ifdef _MSC_VER
			auto data = stdext::checked_array_iterator<CharType*>{ m_Storage.GetData(), m_Storage.Size, 0 };
#else
			auto data = m_Storage.GetData();
#endif

			std::copy(view.cbegin(), view.cend(), data);
		}

		template <StringType srcType>
		void Assign(StringView<srcType> const& src)
		{
			const auto oldSize = m_Storage.Size;
			Append(src);
			if (oldSize > 0)
			{
#ifdef _MSC_VER
				auto pData = stdext::checked_array_iterator<CharType*>{ m_Storage.GetData(), m_Storage.Size, 0 };
#else
				auto pData = m_Storage.GetData();
#endif

				const auto newSize = m_Storage.Size;
				std::copy(pData + oldSize, pData + newSize, pData);
				pop_back(oldSize);
			}
		}

		template <StringType srcType>
		void Assign(String<srcType> const& src)
		{
			Assign(src.GetView());
		}

		void Append(CharType Char, size_t count = 1)
		{
			std::fill_n(ResizeMore(count), count, Char);
		}

		void Append(View const& view)
		{
#ifdef _MSC_VER
			auto data = stdext::checked_array_iterator<CharType*>{ ResizeMore(view.size()), m_Storage.Size + view.size(), 0 };
#else
			auto data = ResizeMore(view.size());
#endif
			std::copy(view.cbegin(), view.cend(), data);
		}

		template <StringType srcType>
		void Append(StringView<srcType> const& src)
		{
			detail_::TransCoder<srcType>{}(*this, src);
		}

		template <StringType srcType>
		void Append(String<srcType> const& src)
		{
			Append(src.GetView());
		}

		void Clear() noexcept
		{
			Resize(0);
		}

		iterator begin() noexcept
		{
			return m_Storage.GetData();
		}

		iterator end() noexcept
		{
			return m_Storage.GetData() + m_Storage.Size;
		}

		const_iterator cbegin() const noexcept
		{
			return m_Storage.GetData();
		}

		const_iterator cend() const noexcept
		{
			return m_Storage.GetData() + m_Storage.Size;
		}

		size_t size() const noexcept
		{
			return m_Storage.Size;
		}

		bool empty() const noexcept
		{
			return m_Storage.Size == 0;
		}

		void pop_front(size_t count = 1) noexcept
		{
			assert(count <= m_Storage.Size);

#ifdef _MSC_VER
			auto data = stdext::checked_array_iterator<CharType*>{ m_Storage.GetData(), m_Storage.Size, 0 };
#else
			auto data = m_Storage.GetData();
#endif

			std::copy_n(m_Storage.GetData() + count, m_Storage.Size - count, data);
			Resize(m_Storage.Size - count);
		}

		void pop_back(size_t count = 1) noexcept
		{
			assert(count <= m_Storage.Size);
			Resize(m_Storage.Size - count);
		}

		CharType const& UncheckGet(size_t index) const noexcept
		{
			assert(index < m_Storage.Size && "index is out of range.");

			return m_Storage.GetData()[index];
		}

		CharType const& Get(size_t index) const
		{
			if (index >= m_Storage.Size)
			{
				detail_::IndexOutOfRange();
			}

			return UncheckGet(index);
		}

		CharType& UncheckGet(size_t index) noexcept
		{
			return const_cast<CharType&>(static_cast<const String*>(this)->UncheckGet(index));
		}

		CharType& Get(size_t index)
		{
			if (index >= m_Storage.Size)
			{
				detail_::IndexOutOfRange();
			}

			return UncheckGet(index);
		}

		CharType& operator[](size_t index) noexcept
		{
			return UncheckGet(index);
		}

		CharType const& operator[](size_t index) const noexcept
		{
			return UncheckGet(index);
		}

		CharType* data() noexcept
		{
			return m_Storage.GetData();
		}

		const CharType* data() const noexcept
		{
			return m_Storage.GetData();
		}

		bool operator==(View const& other) const noexcept
		{
			return GetView() == other;
		}

		bool operator!=(View const& other) const noexcept
		{
			return GetView() != other;
		}

		bool operator<(View const& other) const noexcept
		{
			return GetView() < other;
		}

		bool operator>(View const& other) const noexcept
		{
			return GetView() > other;
		}

		bool operator<=(View const& other) const noexcept
		{
			return GetView() <= other;
		}

		bool operator>=(View const& other) const noexcept
		{
			return GetView() >= other;
		}

		View GetView() const noexcept
		{
			return { m_Storage.GetData(), m_Storage.Size };
		}

		operator View() const noexcept
		{
			return GetView();
		}

#ifdef _WIN32
		operator std::string() const
		{
			String<StringType::Ansi> ansiStr;
			detail_::TransCoder<stringType>{}(ansiStr, *this);
			return { ansiStr.data(), ansiStr.size() };
		}

		operator std::wstring() const
		{
			String<StringType::Wide> wideStr;
			detail_::TransCoder<stringType>{}(wideStr, *this);
			return { wideStr.data(), wideStr.size() };
		}
#endif

	private:
		detail_::StringStorage<CharType, MaxShortStringSize> m_Storage;
	};

	extern template class String<StringType::Utf8>;
	extern template class String<StringType::Utf16>;
	extern template class String<StringType::Utf32>;

#ifdef _WIN32
	extern template class String<StringType::Ansi>;
	extern template class String<StringType::Wide>;
#endif

	typedef StringView<StringType::Utf8> U8StringView;
	typedef StringView<StringType::Utf16> U16StringView;
	typedef StringView<StringType::Utf32> U32StringView;

#ifdef _WIN32
	typedef StringView<StringType::Ansi> AnsiStringView;
	typedef StringView<StringType::Wide> WideStringView;
#endif

	typedef String<StringType::Utf8> U8String;
	typedef String<StringType::Utf16> U16String;
	typedef String<StringType::Utf32> U32String;

#ifdef _WIN32
	typedef String<StringType::Ansi> AnsiString;
	typedef String<StringType::Wide> WideString;
#endif

	namespace detail_
	{
#ifdef _WIN32
		static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t does not have the same size with char16_t.");
		static_assert(alignof(wchar_t) == alignof(char16_t), "wchar_t does not have the same alignment with char16_t.");
#endif

		template <StringType encoding>
		struct EncodingCodePoint;

		template <>
		struct EncodingCodePoint<StringType::Utf8>
		{
			static EncodingResult Encode(String<StringType::Utf8>& output, nuInt codePoint);
			static std::pair<EncodingResult, size_t> Decode(StringView<StringType::Utf8> const& input, nuInt& codePoint);
		};

		template <>
		struct EncodingCodePoint<StringType::Utf16>
		{
			static EncodingResult Encode(String<StringType::Utf16>& output, nuInt codePoint);
			static std::pair<EncodingResult, size_t> Decode(StringView<StringType::Utf16> const& input, nuInt& codePoint);
		};

		template <>
		struct EncodingCodePoint<StringType::Utf32>
		{
			static EncodingResult Encode(String<StringType::Utf32>& output, nuInt codePoint);
			static std::pair<EncodingResult, size_t> Decode(StringView<StringType::Utf32> const& input, nuInt& codePoint);
		};

#ifdef _WIN32
		template <>
		struct EncodingCodePoint<StringType::Ansi>
		{
			static EncodingResult Encode(String<StringType::Ansi>& output, nuInt codePoint);
			static std::pair<EncodingResult, size_t> Decode(StringView<StringType::Ansi> const& input, nuInt& codePoint);
		};

		template <>
		struct EncodingCodePoint<StringType::Wide>
		{
			static EncodingResult Encode(String<StringType::Wide>& output, nuInt codePoint);
			static std::pair<EncodingResult, size_t> Decode(StringView<StringType::Wide> const& input, nuInt& codePoint);
		};
#endif

		template <StringType SrcType>
		struct TransCoder
		{
			template <StringType DstType>
			void operator()(String<DstType>& dst, StringView<SrcType> const& src) const
			{
				typedef std::conditional_t<DstType == StringType::Utf32 || SrcType == StringType::Utf32, U32String, U16String> UnifiedString;

				UnifiedString tmpStr;
				String<SrcType>::TransAppendTo(tmpStr, src);
				String<DstType>::TransAppendFrom(dst, tmpStr);
			}

#ifdef _WIN32
			void operator()(WideString& dst, StringView<SrcType> const& src) const
			{
				String<SrcType>::TransAppendTo(reinterpret_cast<U16String&>(dst), src);
			}
#endif

			void operator()(U16String& dst, StringView<SrcType> const& src) const
			{
				String<SrcType>::TransAppendTo(dst, src);
			}

			void operator()(U32String& dst, StringView<SrcType> const& src) const
			{
				String<SrcType>::TransAppendTo(dst, src);
			}
		};

#ifdef _WIN32
		template <>
		struct TransCoder<StringType::Wide>
		{
			template <StringType DstType>
			void operator()(String<DstType>& dst, WideStringView const& src) const
			{
				String<DstType>::TransAppendFrom(dst, reinterpret_cast<U16StringView const&>(src));
			}
		};
#endif

		template <>
		struct TransCoder<StringType::Utf16>
		{
			template <StringType DstType>
			void operator()(String<DstType>& dst, U16StringView const& src) const
			{
				String<DstType>::TransAppendFrom(dst, src);
			}
		};

		template <>
		struct TransCoder<StringType::Utf32>
		{
			template <StringType DstType>
			void operator()(String<DstType>& dst, U32StringView const& src) const
			{
				String<DstType>::TransAppendFrom(dst, src);
			}
		};
	}
}

#ifdef _WIN32
template <typename CharType, NatsuLib::StringType stringType>
std::basic_ostream<CharType>& operator<<(std::basic_ostream<CharType>& os, NatsuLib::StringView<stringType> const& str)
{
	os << static_cast<std::basic_string<CharType>>(static_cast<std::conditional_t<std::is_same<CharType, char>::value, NatsuLib::AnsiString, NatsuLib::WideString>>(str));
	return os;
}

template <typename CharType, NatsuLib::StringType stringType>
std::basic_ostream<CharType>& operator<<(std::basic_ostream<CharType>& os, NatsuLib::String<stringType> const& str)
{
	return os << static_cast<std::basic_string<CharType>>(str);
}

template <typename CharType, NatsuLib::StringType stringType>
std::basic_istream<CharType>& operator>>(std::basic_istream<CharType>& is, NatsuLib::String<stringType>& str)
{
	std::basic_string<CharType> tmpStr;
	is >> tmpStr;
	str.Assign(static_cast<typename NatsuLib::String<stringType>::View>(tmpStr.c_str()));
	return is;
}
#else
template <typename CharType, NatsuLib::StringType stringType>
std::basic_ostream<CharType>& operator<<(std::basic_ostream<CharType>& os, NatsuLib::StringView<stringType> const& str)
{
	os << str.data();
	return os;
}

template <typename CharType, NatsuLib::StringType stringType>
std::basic_ostream<CharType>& operator<<(std::basic_ostream<CharType>& os, NatsuLib::String<stringType> const& str)
{
	return os << str.GetView();
}

template <typename CharType, NatsuLib::StringType stringType>
std::basic_istream<CharType>& operator >> (std::basic_istream<CharType>& is, NatsuLib::String<stringType>& str)
{
	std::basic_string<CharType> tmpStr;
	is >> tmpStr;
	str.Assign(tmpStr.c_str());
	return is;
}
#endif

#ifdef _MSC_VER
#pragma pop_macro("max")
#endif

NATINLINE NatsuLib::U8StringView operator""_u8v(const NatsuLib::U8StringView::CharType* str, size_t length) noexcept
{
	return { str,length };
}

NATINLINE NatsuLib::U8String operator""_u8s(NatsuLib::U8StringView::CharType Char) noexcept
{
	return NatsuLib::U8StringView{ Char };
}

NATINLINE NatsuLib::U8String operator""_u8s(const NatsuLib::U8String::CharType* str, size_t length) noexcept
{
	return NatsuLib::U8StringView{ str,length };
}

NATINLINE NatsuLib::U16StringView operator""_u16v(const NatsuLib::U16StringView::CharType* str, size_t length) noexcept
{
	return { str,length };
}

NATINLINE NatsuLib::U16String operator""_u16s(NatsuLib::U16StringView::CharType Char) noexcept
{
	return NatsuLib::U16StringView{ Char };
}

NATINLINE NatsuLib::U16String operator""_u16s(const NatsuLib::U16String::CharType* str, size_t length) noexcept
{
	return NatsuLib::U16StringView{ str,length };
}

NATINLINE NatsuLib::U32StringView operator""_u32v(const NatsuLib::U32StringView::CharType* str, size_t length) noexcept
{
	return { str,length };
}

NATINLINE NatsuLib::U32String operator""_u32s(NatsuLib::U32StringView::CharType Char) noexcept
{
	return NatsuLib::U32StringView{ Char };
}

NATINLINE NatsuLib::U32String operator""_u32s(const NatsuLib::U32String::CharType* str, size_t length) noexcept
{
	return NatsuLib::U32StringView{ str,length };
}

#ifdef _WIN32
NATINLINE NatsuLib::AnsiStringView operator""_av(const NatsuLib::AnsiStringView::CharType* str, size_t length) noexcept
{
	return { str,length };
}

NATINLINE NatsuLib::AnsiString operator""_as(NatsuLib::AnsiStringView::CharType Char) noexcept
{
	return NatsuLib::AnsiStringView{ Char };
}

NATINLINE NatsuLib::AnsiString operator""_as(const NatsuLib::AnsiString::CharType* str, size_t length) noexcept
{
	return NatsuLib::AnsiStringView{ str,length };
}

NATINLINE NatsuLib::WideStringView operator""_wv(const NatsuLib::WideStringView::CharType* str, size_t length) noexcept
{
	return{ str,length };
}

NATINLINE NatsuLib::WideString operator""_ws(NatsuLib::WideStringView::CharType Char) noexcept
{
	return NatsuLib::WideStringView{ Char };
}

NATINLINE NatsuLib::WideString operator""_ws(const NatsuLib::WideString::CharType* str, size_t length) noexcept
{
	return NatsuLib::WideStringView{ str,length };
}
#endif

typedef NatsuLib::StringEncodingTrait<NatsuLib::StringType::Utf8>::CharType nU8Char;
typedef NatsuLib::U8StringView nStrView;
typedef NatsuLib::U8String nString;
typedef nString::CharType nTChar;
typedef nTChar* nTStr;

NATINLINE nStrView operator""_nv(const nU8Char* str, size_t length) noexcept
{
	return { str, length };
}

NATINLINE nString operator""_ns(nU8Char Char) noexcept
{
	return nStrView{ Char };
}

NATINLINE nString operator""_ns(const nU8Char* str, size_t length)
{
	return nStrView{ str, length };
}

template <NatsuLib::StringType stringTypel, NatsuLib::StringType stringTyper>
NatsuLib::String<stringTypel> operator+(NatsuLib::StringView<stringTypel> const& left, NatsuLib::StringView<stringTyper> const& right)
{
	NatsuLib::String<stringTypel> ret{ left };
	ret.Append(right);
	return ret;
}

template <NatsuLib::StringType stringTypel, NatsuLib::StringType stringTyper>
NatsuLib::String<stringTypel> operator+(NatsuLib::String<stringTypel> const& left, NatsuLib::StringView<stringTyper> const& right)
{
	return left.GetView() + right;
}

template <NatsuLib::StringType stringTypel, NatsuLib::StringType stringTyper>
NatsuLib::String<stringTypel> operator+(NatsuLib::StringView<stringTypel> const& left, NatsuLib::String<stringTyper> const& right)
{
	return left + right.GetView();
}

template <NatsuLib::StringType stringTypel, NatsuLib::StringType stringTyper>
NatsuLib::String<stringTypel> operator+(NatsuLib::String<stringTypel> const& left, NatsuLib::String<stringTyper> const& right)
{
	return left.GetView() + right.GetView();
}

template <NatsuLib::StringType stringTypel, NatsuLib::StringType stringTyper>
NatsuLib::String<stringTypel>& operator+=(NatsuLib::String<stringTypel>& left, NatsuLib::StringView<stringTyper> const& right)
{
	left.Append(right);
	return left;
}

template <NatsuLib::StringType stringTypel, NatsuLib::StringType stringTyper>
NatsuLib::String<stringTypel>& operator+=(NatsuLib::String<stringTypel>& left, NatsuLib::String<stringTyper> const& right)
{
	left.Append(right);
	return left;
}

namespace NatsuLib
{
	namespace detail_
	{
		// 来自 https://www.byvoid.com/blog/string-hash-compare
		template <typename CharType>
		size_t BKDRHash(const CharType* str)
		{
			size_t seed = 131; // 31 131 1313 13131 131313 etc..
			size_t hash = 0;

			while (*str)
			{
				hash = hash * seed + (*str++);
			}

			return (hash & 0x7FFFFFFF);
		}
	}
}

namespace std
{
	template <NatsuLib::StringType stringType>
	struct hash<NatsuLib::StringView<stringType>>
	{
		size_t operator()(NatsuLib::StringView<stringType> const& view) const
		{
			return NatsuLib::detail_::BKDRHash(view.data());
		}
	};

	template <NatsuLib::StringType stringType>
	struct hash<NatsuLib::String<stringType>>
	{
		size_t operator()(NatsuLib::String<stringType> const& str) const
		{
			return hash<NatsuLib::StringView<stringType>>{}(str.GetView());
		}
	};
}

#include "natMisc.h"

namespace NatsuLib
{
	namespace detail_
	{
		template <typename Iter1, typename Iter2>
		size_t MatchString(Iter1 srcBegin, Iter1 srcEnd, Iter2 patternBegin, Iter2 patternEnd) noexcept
		{
			assert(srcBegin != srcEnd);
			assert(static_cast<size_t>(srcEnd - srcBegin) >= static_cast<size_t>(patternEnd - patternBegin));

			const auto patternSize = static_cast<size_t>(std::distance(patternBegin, patternEnd));

			std::ptrdiff_t* table{};
			auto shouldDelete = false;
			const auto tableSize = patternSize - 1;
			auto scope = make_scope([&table, &shouldDelete]
			{
				if (shouldDelete)
				{
					delete[] table;
				}
			});

			if (tableSize >= MaxAllocaSize / sizeof(std::ptrdiff_t))
			{
				table = new(std::nothrow) std::ptrdiff_t[tableSize];
				shouldDelete = true;
			}
			else if (tableSize > 0)
			{
				table = static_cast<std::ptrdiff_t*>(alloca(tableSize * sizeof(std::ptrdiff_t)));
				shouldDelete = false;
			}

			if (table)
			{
				table[0] = 0;

				if (patternSize > 2)
				{
					std::ptrdiff_t pos = 1, cand = 0;

					do
					{
						if (patternBegin[pos] == patternBegin[cand])
						{
							++cand;
							table[pos] = patternBegin[pos + 1] == patternBegin[cand] ? table[cand - 1] : cand;
						}
						else if (cand == 0)
						{
							table[pos] = 0;
						}
						else
						{
							cand = table[cand - 1];
							continue;
						}

						++pos;
					} while (static_cast<size_t>(pos) < patternSize - 1);
				}
			}

			auto current = srcBegin;
			while (true)
			{
				for (;; ++current)
				{
					if (std::distance(current, srcEnd) < static_cast<std::ptrdiff_t>(patternSize))
					{
						return npos;
					}
					if (*current == *patternBegin)
					{
						break;
					}
				}

				std::ptrdiff_t matchLen = 1;

				while (true)
				{
					if (static_cast<size_t>(matchLen) >= patternSize)
					{
						return static_cast<size_t>(current - srcBegin);
					}

				Fallback:
					if (current[matchLen] != patternBegin[matchLen])
					{
						break;
					}

					++matchLen;
				}

				if (!table)
				{
					++current;
					continue;
				}

				current += matchLen;
				const auto fallback = table[matchLen - 1];
				if (fallback != 0)
				{
					current -= fallback;
					matchLen = fallback;
					goto Fallback;
				}
			}
		}

		template <typename CharType, size_t ArrayMaxSize>
		void StringStorage<CharType, ArrayMaxSize>::Reserve(size_t newCapacity)
		{
			using std::swap;

			if (newCapacity <= Capacity)
			{
				return;
			}

			if (newCapacity > ArrayMaxSize)
			{
				auto newBuffer = new CharType[newCapacity];
				auto scope = make_scope([&newBuffer]
				{
					delete[] newBuffer;
				});

				if (Size > 0)
				{
					std::memmove(newBuffer, GetData(), Size * sizeof(CharType));
					newBuffer[Size] = 0;
				}

				if (Capacity <= ArrayMaxSize)
				{
					Pointer = nullptr;
				}

				swap(Pointer, newBuffer);
				Capacity = newCapacity;
			}
		}
	}
}
