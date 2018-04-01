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

		static std::size_t GetCharCount(CharType Char) noexcept
		{
			const auto unsignedChar = static_cast<std::make_unsigned_t<CharType>>(Char);
			if (unsignedChar < 0x80)
			{
				return 1;
			}
			if (unsignedChar - 0x80 < 0x40)
			{
				// 错误的编码
				return std::size_t(-1);
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
			return std::size_t(-1);
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

		static std::size_t GetCharCount(CharType /*Char*/) noexcept
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

		static std::size_t GetCharCount(CharType /*Char*/) noexcept
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

		static std::size_t GetCharCount(CharType Char) noexcept
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

		static std::size_t GetCharCount(CharType /*Char*/) noexcept
		{
			return 1;
		}
	};
#endif

	namespace detail_
	{
		enum : std::size_t
		{
			npos = std::size_t(-1),
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
		std::size_t SearchCharRepeat(Iter srcBegin, Iter srcEnd, CharType searchChar, std::size_t repeatCount) noexcept
		{
			assert(repeatCount != 0);
			assert(static_cast<std::size_t>(srcEnd - srcBegin) > repeatCount);

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
					if (static_cast<std::size_t>(matchLen) >= repeatCount)
					{
						return static_cast<std::size_t>(std::distance(srcBegin, current));
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
		std::size_t MatchString(Iter1 srcBegin, Iter1 srcEnd, Iter2 patternBegin, Iter2 patternEnd) noexcept;

		template <StringType stringType>
		std::enable_if_t<StringEncodingTrait<stringType>::MaxCharSize == 1, std::size_t> GetCharCount(const typename StringEncodingTrait<stringType>::CharType* /*str*/, std::size_t length)
		{
			return length;
		}

		template <StringType stringType>
		std::enable_if_t<(StringEncodingTrait<stringType>::MaxCharSize > 1), std::size_t> GetCharCount(const typename StringEncodingTrait<stringType>::CharType* str, std::size_t length)
		{
			std::size_t count{};
			for (std::size_t i = 0; i < length; )
			{
				const auto currentSize = StringEncodingTrait<stringType>::GetCharCount(str[i]);
				i += currentSize;
				count += currentSize;
			}

			return count;
		}

		[[noreturn]] void IndexOutOfRange();
		[[noreturn]] void SizeOutOfRange();
	}

	////////////////////////////////////////////////////////////////////////////////
	///	@brief	编码结果
	////////////////////////////////////////////////////////////////////////////////
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
		static constexpr StringType UsingStringType = stringType;

		typedef typename StringEncodingTrait<stringType>::CharType CharType;
		typedef CharType Element;
		typedef const CharType* CharIterator;
		typedef const CharType* const_iterator;
		typedef const_iterator iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef const_reverse_iterator reverse_iterator;

		enum : std::size_t
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

		constexpr StringView(std::nullptr_t, std::nullptr_t = nullptr) noexcept
			: StringView()
		{
		}

		constexpr StringView(CharIterator begin, std::size_t length) noexcept
			: StringView(begin, begin + length)
		{
		}

		constexpr StringView(std::initializer_list<CharType> il) noexcept
			: StringView(il.begin(), il.end())
		{
		}

		StringView(CharIterator begin) noexcept
			: StringView(begin, detail_::GetEndOfString(begin))
		{
		}

		template <std::size_t N>
		constexpr StringView(const CharType(&array)[N])
			: StringView(array, array + N)
		{
		}

		///	@brief	字符串视图是否为空
		constexpr nBool IsEmpty() const noexcept
		{
			return m_StrBegin == m_StrEnd;
		}

		void Clear() noexcept
		{
			m_StrBegin = m_StrEnd;
		}

		constexpr const_iterator begin() const noexcept
		{
			return m_StrBegin;
		}

		constexpr const_iterator end() const noexcept
		{
			return m_StrEnd;
		}

		constexpr const_iterator cbegin() const noexcept
		{
			return m_StrBegin;
		}

		constexpr const_iterator cend() const noexcept
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

		constexpr std::size_t size() const noexcept
		{
			return GetSize();
		}

		constexpr nBool empty() const noexcept
		{
			return m_StrBegin == m_StrEnd;
		}

		constexpr CharIterator data() const noexcept
		{
			return m_StrBegin;
		}

		constexpr const CharType& operator[](std::size_t pos) const noexcept
		{
			return UncheckGet(pos);
		}

		nBool operator==(StringView const& other) const noexcept
		{
			if (GetSize() != other.GetSize())
			{
				return false;
			}

			return Compare(other) == 0;
		}

		nBool operator!=(StringView const& other) const noexcept
		{
			if (GetSize() != other.GetSize())
			{
				return true;
			}

			return Compare(other) != 0;
		}

		nBool operator<(StringView const& other) const noexcept
		{
			return Compare(other) < 0;
		}

		nBool operator>(StringView const& other) const noexcept
		{
			return Compare(other) > 0;
		}

		nBool operator<=(StringView const& other) const noexcept
		{
			return Compare(other) <= 0;
		}

		nBool operator>=(StringView const& other) const noexcept
		{
			return Compare(other) >= 0;
		}

		constexpr std::size_t GetSize() const noexcept
		{
			return m_StrEnd - m_StrBegin;
		}

		std::size_t GetCharCount() const noexcept
		{
			return detail_::GetCharCount<stringType>(m_StrBegin, m_StrEnd - m_StrBegin);
		}

		void Swap(StringView& other) noexcept
		{
			using std::swap;
			swap(m_StrBegin, other.m_StrBegin);
			swap(m_StrEnd, other.m_StrEnd);
		}

		const CharType& Get(std::size_t index) const
		{
			if (index >= GetSize())
			{
				detail_::IndexOutOfRange();
			}

			return UncheckGet(index);
		}

		constexpr const CharType& UncheckGet(std::size_t pos) const noexcept
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

		constexpr StringView Slice(std::ptrdiff_t begin, std::ptrdiff_t end) const noexcept
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
			std::size_t pos{};
			const auto strLen = size();

			for (std::size_t i = 0; i < strLen; ++i)
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

		void Assign(CharIterator begin, std::size_t length) noexcept
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

		std::size_t Find(StringView const& pattern, std::ptrdiff_t nBegin = 0) const noexcept
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

		std::size_t FindBackward(StringView const& pattern, std::ptrdiff_t nEnd = -1) const noexcept
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

		std::size_t FindCharRepeat(CharType findChar, std::size_t repeatCount, std::ptrdiff_t nBegin = 0) const noexcept
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

		std::size_t FindCharRepeatBackward(CharType findChar, std::size_t repeatCount, std::ptrdiff_t nEnd = 0) const noexcept
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

		std::size_t Find(CharType findChar, std::ptrdiff_t nBegin = 0) const noexcept
		{
			return FindCharRepeat(findChar, 1, nBegin);
		}

		std::size_t FindBackward(CharType findChar, std::ptrdiff_t nEnd = -1) const noexcept
		{
			return FindCharRepeatBackward(findChar, 1, nEnd);
		}

		nBool StartWith(CharType findChar, std::size_t repeatCount = 1) const noexcept
		{
			if (!repeatCount || repeatCount > size())
			{
				return false;
			}

			for (std::size_t i = 0; i < repeatCount; ++i)
			{
				if (m_StrBegin[i] != findChar)
				{
					return false;
				}
			}

			return true;
		}

		nBool StartWith(StringView const& other) const noexcept
		{
			return size() >= other.size() && Slice(0, other.size()) == other;
		}

		nBool EndWith(CharType findChar, std::size_t repeatCount = 1) const noexcept
		{
			if (!repeatCount || repeatCount > size())
			{
				return false;
			}

			const auto startIndex = size() - repeatCount;
			for (std::size_t i = 0; i < repeatCount; ++i)
			{
				if (m_StrBegin[startIndex + i] != findChar)
				{
					return false;
				}
			}

			return true;
		}

		nBool EndWith(StringView const& other) const noexcept
		{
			return size() >= other.size() && Slice(size() - other.size(), other.size()) == other;
		}

		nBool DoesOverlapWith(StringView const& other) const noexcept
		{
			// FIXME: 可能是UB，参见n4640 5.9 (3)
			return m_StrBegin < other.m_StrEnd && m_StrEnd > other.m_StrBegin;
		}

	private:
		CharIterator m_StrBegin;
		CharIterator m_StrEnd;

		static std::size_t ApplyOffset(std::ptrdiff_t offset, std::size_t size) noexcept
		{
			auto ret = static_cast<std::size_t>(offset);
			if (offset < 0)
			{
				ret += size + 1;
			}

			assert(ret <= size);
			return ret;
		}
	};

	namespace detail_
	{
		template <StringType SrcType>
		struct TransCoder;

		constexpr std::size_t Grow(std::size_t size) noexcept
		{
			return std::max(size + 1, (size + 1 + ((size + 1) >> 1) + 0x0F) & static_cast<std::size_t>(-0x10));
		}

		template <typename CharType, std::size_t ArrayMaxSize>
		struct StringStorage
		{
			constexpr StringStorage() noexcept
				: Size{}, Capacity{ ArrayMaxSize }, Pointer{}
			{
			}

			explicit StringStorage(std::size_t capacity)
				: StringStorage{}
			{
				Reserve(capacity);
			}

			StringStorage(const CharType* str, std::size_t length)
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

			template <std::size_t OtherArrayMaxSize>
			StringStorage(StringStorage<CharType, OtherArrayMaxSize> const& other)
				: StringStorage{}
			{
				Assign(other);
			}

			template <std::size_t OtherArrayMaxSize>
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

			template <std::size_t OtherArrayMaxSize>
			StringStorage& operator=(StringStorage<CharType, OtherArrayMaxSize> const& other)
			{
				Assign(other);
				return *this;
			}

			template <std::size_t OtherArrayMaxSize>
			StringStorage& operator=(StringStorage<CharType, OtherArrayMaxSize>&& other) noexcept
			{
				Assign(std::move(other));
				return *this;
			}

			void Assign(const CharType* str, std::size_t length)
			{
				Resize(length);
				std::memmove(GetData(), str, length * sizeof(CharType));
			}

			template <std::size_t OtherArrayMaxSize>
			void Assign(StringStorage<CharType, OtherArrayMaxSize> const& other)
			{
				Assign(other.GetData(), other.Size);
			}

			template <std::size_t OtherArrayMaxSize>
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

			void Reserve(std::size_t newCapacity);

			void Resize(std::size_t newSize)
			{
				if (newSize >= std::max(ArrayMaxSize, Capacity))
				{
					Reserve(Grow(newSize));
				}

				const auto data = GetData();

				for (auto i = Size; i < newSize; ++i)
				{
					data[i] = CharType{};
				}
				Size = newSize;
				data[Size] = CharType{};

				assert(Capacity > Size);
			}

			std::size_t Size;
			std::size_t Capacity;
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
		static constexpr StringType UsingStringType = stringType;

		typedef StringView<stringType> View;
		typedef typename View::CharType CharType;
		typedef CharType Element;
		typedef CharType* iterator;
		typedef const CharType* const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		enum : std::size_t
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

		explicit String(CharType Char, std::size_t count = 1)
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

		void Reserve(std::size_t newCapacity)
		{
			m_Storage.Reserve(newCapacity);
		}

		void Resize(std::size_t newSize)
		{
			m_Storage.Resize(newSize);
		}

		iterator ResizeMore(std::size_t moreSize)
		{
			const auto oldSize = m_Storage.Size;
			Resize(oldSize + moreSize);
			return m_Storage.GetData() + oldSize;
		}

		void Assign(CharType Char, std::size_t count = 1)
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

		void Assign(String && src)
		{
			m_Storage.Assign(std::move(src.m_Storage));
		}

		void Append(CharType Char, std::size_t count = 1)
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

		nBool IsEmpty() const noexcept
		{
			return empty();
		}

		iterator begin() noexcept
		{
			return m_Storage.GetData();
		}

		iterator end() noexcept
		{
			return m_Storage.GetData() + m_Storage.Size;
		}

		const_iterator begin() const noexcept
		{
			return m_Storage.GetData();
		}

		const_iterator end() const noexcept
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

		std::size_t size() const noexcept
		{
			return m_Storage.Size;
		}

		nBool empty() const noexcept
		{
			return m_Storage.Size == 0;
		}

		void pop_front(std::size_t count = 1) noexcept
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

		void pop_back(std::size_t count = 1) noexcept
		{
			assert(count <= m_Storage.Size);
			Resize(m_Storage.Size - count);
		}

		CharType const& UncheckGet(std::size_t index) const noexcept
		{
			assert(index < m_Storage.Size && "index is out of range.");

			return m_Storage.GetData()[index];
		}

		CharType const& Get(std::size_t index) const
		{
			if (index >= m_Storage.Size)
			{
				detail_::IndexOutOfRange();
			}

			return UncheckGet(index);
		}

		CharType& UncheckGet(std::size_t index) noexcept
		{
			return const_cast<CharType&>(static_cast<const String*>(this)->UncheckGet(index));
		}

		CharType& Get(std::size_t index)
		{
			if (index >= m_Storage.Size)
			{
				detail_::IndexOutOfRange();
			}

			return UncheckGet(index);
		}

		CharType& operator[](std::size_t index) noexcept
		{
			return UncheckGet(index);
		}

		CharType const& operator[](std::size_t index) const noexcept
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

	template <>
	void String<StringType::Utf8>::TransAppendTo(String<StringType::Utf16>& dst, View const& src);
	template <>
	void String<StringType::Utf8>::TransAppendFrom(String& dst, StringView<StringType::Utf16> const& src);
	template <>
	void String<StringType::Utf8>::TransAppendTo(String<StringType::Utf32>& dst, View const& src);
	template <>
	void String<StringType::Utf8>::TransAppendFrom(String& dst, StringView<StringType::Utf32> const& src);

	template <>
	void String<StringType::Utf16>::TransAppendTo(String<StringType::Utf16>& dst, View const& src);
	template <>
	void String<StringType::Utf16>::TransAppendFrom(String& dst, StringView<StringType::Utf16> const& src);
	template <>
	void String<StringType::Utf16>::TransAppendTo(String<StringType::Utf32>& dst, View const& src);
	template <>
	void String<StringType::Utf16>::TransAppendFrom(String& dst, StringView<StringType::Utf32> const& src);

	template <>
	void String<StringType::Utf32>::TransAppendTo(String<StringType::Utf16>& dst, View const& src);
	template <>
	void String<StringType::Utf32>::TransAppendFrom(String& dst, StringView<StringType::Utf16> const& src);
	template <>
	void String<StringType::Utf32>::TransAppendTo(String<StringType::Utf32>& dst, View const& src);
	template <>
	void String<StringType::Utf32>::TransAppendFrom(String& dst, StringView<StringType::Utf32> const& src);

#ifdef _WIN32
	template <>
	void String<StringType::Ansi>::TransAppendTo(String<StringType::Utf16>& dst, View const& src);
	template <>
	void String<StringType::Ansi>::TransAppendFrom(String& dst, StringView<StringType::Utf16> const& src);
	template <>
	void String<StringType::Ansi>::TransAppendTo(String<StringType::Utf32>& dst, View const& src);
	template <>
	void String<StringType::Ansi>::TransAppendFrom(String& dst, StringView<StringType::Utf32> const& src);

	template <>
	void String<StringType::Wide>::TransAppendTo(String<StringType::Utf16>& dst, View const& src);
	template <>
	void String<StringType::Wide>::TransAppendFrom(String& dst, StringView<StringType::Utf16> const& src);
	template <>
	void String<StringType::Wide>::TransAppendTo(String<StringType::Utf32>& dst, View const& src);
	template <>
	void String<StringType::Wide>::TransAppendFrom(String& dst, StringView<StringType::Utf32> const& src);
#endif

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

	template <typename>
	struct GetUsingStringType;

	template <StringType UsingStringType>
	struct GetUsingStringType<StringView<UsingStringType>>
		: std::integral_constant<StringType, UsingStringType>
	{
	};

	template <StringType UsingStringType>
	struct GetUsingStringType<String<UsingStringType>>
		: std::integral_constant<StringType, UsingStringType>
	{
	};

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
			static std::pair<EncodingResult, std::size_t> Decode(StringView<StringType::Utf8> const& input, nuInt& codePoint);
		};

		template <>
		struct EncodingCodePoint<StringType::Utf16>
		{
			static EncodingResult Encode(String<StringType::Utf16>& output, nuInt codePoint);
			static std::pair<EncodingResult, std::size_t> Decode(StringView<StringType::Utf16> const& input, nuInt& codePoint);
		};

		template <>
		struct EncodingCodePoint<StringType::Utf32>
		{
			static EncodingResult Encode(String<StringType::Utf32>& output, nuInt codePoint);
			static std::pair<EncodingResult, std::size_t> Decode(StringView<StringType::Utf32> const& input, nuInt& codePoint);
		};

#ifdef _WIN32
		template <>
		struct EncodingCodePoint<StringType::Ansi>
		{
			static EncodingResult Encode(String<StringType::Ansi>& output, nuInt codePoint);
			static std::pair<EncodingResult, std::size_t> Decode(StringView<StringType::Ansi> const& input, nuInt& codePoint);
		};

		template <>
		struct EncodingCodePoint<StringType::Wide>
		{
			static EncodingResult Encode(String<StringType::Wide>& output, nuInt codePoint);
			static std::pair<EncodingResult, std::size_t> Decode(StringView<StringType::Wide> const& input, nuInt& codePoint);
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

	typedef StringEncodingTrait<StringType::Utf8>::CharType nU8Char;
	typedef nU8Char nTChar;
	typedef U8StringView nStrView;
	typedef U8String nString;
}

using NatsuLib::nU8Char;
using NatsuLib::nTChar;
using NatsuLib::nStrView;
using NatsuLib::nString;

namespace NatsuLib
{

#ifdef _WIN32
	namespace detail_
	{
		template <typename CharType>
		struct SelectStringType;

		template <>
		struct SelectStringType<char>
		{
#ifndef NATSULIB_UTF8_SOURCE
			static constexpr StringType SelectedStringType = StringType::Ansi;
#else
			static constexpr StringType SelectedStringType = StringType::Utf8;
#endif
		};

		template <>
		struct SelectStringType<char16_t>
		{
			static constexpr StringType SelectedStringType = StringType::Utf16;
		};

		template <>
		struct SelectStringType<char32_t>
		{
			static constexpr StringType SelectedStringType = StringType::Utf32;
		};

		template <>
		struct SelectStringType<wchar_t>
		{
			static constexpr StringType SelectedStringType = StringType::Wide;
		};
	}

#else
	namespace detail_
	{
		template <typename CharType>
		struct SelectStringType;

		template <>
		struct SelectStringType<char>
		{
			static constexpr StringType SelectedStringType = StringType::Utf8;
		};

		template <>
		struct SelectStringType<char16_t>
		{
			static constexpr StringType SelectedStringType = StringType::Utf16;
		};

		template <>
		struct SelectStringType<char32_t>
		{
			static constexpr StringType SelectedStringType = StringType::Utf32;
		};

		// Bug?
		template <>
		struct SelectStringType<wchar_t>
		{
			static constexpr StringType SelectedStringType = StringType::Utf32;
		};
	}
#endif

	template <typename CharType>
	std::basic_ostream<CharType>& operator<<(std::basic_ostream<CharType>& os, StringView<detail_::SelectStringType<CharType>::SelectedStringType> const& str)
	{
		return os.write(str.data(), str.size());
	}

	template <typename CharType, StringType stringType, std::enable_if_t<detail_::SelectStringType<CharType>::SelectedStringType != stringType, int> = 0>
	std::basic_ostream<CharType>& operator<<(std::basic_ostream<CharType>& os, StringView<stringType> const& str)
	{
		return os << NatsuLib::String<detail_::SelectStringType<CharType>::SelectedStringType>{ str }.GetView();
	}

	template <typename CharType, StringType stringType>
	std::basic_ostream<CharType>& operator<<(std::basic_ostream<CharType>& os, String<stringType> const& str)
	{
		return os << str.GetView();
	}

	template <typename CharType, StringType stringType>
	std::basic_istream<CharType>& operator>>(std::basic_istream<CharType>& is, String<stringType>& str)
	{
		std::basic_string<CharType> tmpStr;
		is >> tmpStr;
		str.Assign(NatsuLib::StringView<detail_::SelectStringType<CharType>::SelectedStringType>{ tmpStr.c_str() });
		return is;
	}

#ifdef _MSC_VER
#pragma pop_macro("max")
#endif

	template <StringType stringTypel, StringType stringTyper>
	String<stringTypel> operator+(StringView<stringTypel> const& left, StringView<stringTyper> const& right)
	{
		String<stringTypel> ret{ left };
		ret.Append(right);
		return ret;
	}

	template <StringType stringTypel, StringType stringTyper>
	String<stringTypel> operator+(String<stringTypel> const& left, StringView<stringTyper> const& right)
	{
		return left.GetView() + right;
	}

	template <StringType stringTypel, StringType stringTyper>
	String<stringTypel> operator+(StringView<stringTypel> const& left, String<stringTyper> const& right)
	{
		return left + right.GetView();
	}

	template <StringType stringTypel, StringType stringTyper>
	String<stringTypel> operator+(String<stringTypel> const& left, String<stringTyper> const& right)
	{
		return left.GetView() + right.GetView();
	}

	template <StringType stringTypel, StringType stringTyper>
	String<stringTypel>& operator+=(String<stringTypel>& left, StringView<stringTyper> const& right)
	{
		left.Append(right);
		return left;
	}

	template <StringType stringTypel, StringType stringTyper>
	String<stringTypel>& operator+=(String<stringTypel>& left, String<stringTyper> const& right)
	{
		left.Append(right);
		return left;
	}

	namespace detail_
	{
		// 修改自 https://www.byvoid.com/blog/string-hash-compare
		template <typename CharType>
		std::size_t BKDRHash(const CharType* strBegin, const CharType* strEnd)
		{
			constexpr std::size_t seed = 131; // 31 131 1313 13131 131313 etc..
			std::size_t hash = 0;

			while (strBegin != strEnd)
			{
				hash = hash * seed + (*strBegin++);
			}

			return (hash & 0x7FFFFFFF);
		}
	}

	inline namespace StringLiterals
	{
		constexpr U8StringView operator""_u8v(const U8StringView::CharType* str, std::size_t length) noexcept
		{
			return { str,length };
		}

		NATINLINE U8String operator""_u8s(U8StringView::CharType Char) noexcept
		{
			return U8StringView{ Char };
		}

		NATINLINE U8String operator""_u8s(const U8String::CharType* str, std::size_t length) noexcept
		{
			return U8StringView{ str,length };
		}

		constexpr U16StringView operator""_u16v(const U16StringView::CharType* str, std::size_t length) noexcept
		{
			return { str,length };
		}

		NATINLINE U16String operator""_u16s(U16StringView::CharType Char) noexcept
		{
			return U16StringView{ Char };
		}

		NATINLINE U16String operator""_u16s(const U16String::CharType* str, std::size_t length) noexcept
		{
			return U16StringView{ str,length };
		}

		constexpr U32StringView operator""_u32v(const U32StringView::CharType* str, std::size_t length) noexcept
		{
			return { str,length };
		}

		NATINLINE U32String operator""_u32s(U32StringView::CharType Char) noexcept
		{
			return U32StringView{ Char };
		}

		NATINLINE U32String operator""_u32s(const U32String::CharType* str, std::size_t length) noexcept
		{
			return U32StringView{ str,length };
		}

#ifdef _WIN32
		constexpr AnsiStringView operator""_av(const AnsiStringView::CharType* str, std::size_t length) noexcept
		{
			return { str,length };
		}

		NATINLINE AnsiString operator""_as(AnsiStringView::CharType Char) noexcept
		{
			return AnsiStringView{ Char };
		}

		NATINLINE AnsiString operator""_as(const AnsiString::CharType* str, std::size_t length) noexcept
		{
			return AnsiStringView{ str,length };
		}

		constexpr WideStringView operator""_wv(const WideStringView::CharType* str, std::size_t length) noexcept
		{
			return{ str,length };
		}

		NATINLINE WideString operator""_ws(WideStringView::CharType Char) noexcept
		{
			return WideStringView{ Char };
		}

		NATINLINE WideString operator""_ws(const WideString::CharType* str, std::size_t length) noexcept
		{
			return WideStringView{ str,length };
		}
#endif

		constexpr nStrView operator""_nv(const nU8Char* str, std::size_t length) noexcept
		{
			return { str, length };
		}

		NATINLINE nString operator""_ns(nU8Char Char) noexcept
		{
			return nStrView{ Char };
		}

		NATINLINE nString operator""_ns(const nU8Char* str, std::size_t length)
		{
			return nStrView{ str, length };
		}
	}
}

namespace std
{
	template <NatsuLib::StringType stringType>
	struct hash<NatsuLib::StringView<stringType>>
	{
		std::size_t operator()(NatsuLib::StringView<stringType> const& view) const
		{
			return NatsuLib::detail_::BKDRHash(view.cbegin(), view.cend());
		}
	};

	template <NatsuLib::StringType stringType>
	struct hash<NatsuLib::String<stringType>>
	{
		std::size_t operator()(NatsuLib::String<stringType> const& str) const
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
		std::size_t MatchString(Iter1 srcBegin, Iter1 srcEnd, Iter2 patternBegin, Iter2 patternEnd) noexcept
		{
			assert(srcBegin != srcEnd);
			assert(static_cast<std::size_t>(srcEnd - srcBegin) >= static_cast<std::size_t>(patternEnd - patternBegin));

			const auto patternSize = static_cast<std::size_t>(std::distance(patternBegin, patternEnd));

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
					} while (static_cast<std::size_t>(pos) < patternSize - 1);
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
					if (static_cast<std::size_t>(matchLen) >= patternSize)
					{
						return static_cast<std::size_t>(current - srcBegin);
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

		template <typename CharType, std::size_t ArrayMaxSize>
		void StringStorage<CharType, ArrayMaxSize>::Reserve(std::size_t newCapacity)
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
