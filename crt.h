#pragma once
#include <q-tee/common/common.h>

// used: [stl] type traits
#include <type_traits>
// used: [stl] bit_cast
#include <bit>

#if defined(Q_COMPILER_CLANG) || defined(Q_COMPILER_GCC)
#include <x86intrin.h>
#elif defined(Q_COMPILER_MSC)
#include <intrin.h>
#endif

#define Q_CRT

// tells the compiler to inline a metafunction that acts as a named cast from the parameter type to the return type
#ifdef Q_COMPILER_CLANG
#define Q_CRT_INTRINSIC Q_INLINE
#elif defined(Q_COMPILER_MSC)
#define Q_CRT_INTRINSIC [[msvc::intrinsic]]
#endif

// add support of argument validation and other diagnostic messages as for builtins
#ifdef Q_COMPILER_CLANG
#define Q_CRT_DIAGNOSE_AS_BUILTIN(...) [[clang::diagnose_as_builtin(__VA_ARGS__)]]
#elif defined(Q_COMPILER_GCC)
#define Q_CRT_NO_BUILTIN __attribute__((diagnose_as_builtin(__VA_ARGS__)))
#else
#define Q_CRT_DIAGNOSE_AS_BUILTIN(...)
#endif

// tells the compiler to don't replace intrinsics calls with builtins
#ifdef Q_COMPILER_CLANG
#define Q_CRT_NO_BUILTIN [[clang::no_builtin]]
#elif defined(Q_COMPILER_GCC)
#define Q_CRT_NO_BUILTIN __attribute__((no_builtin))
#else
#define Q_CRT_NO_BUILTIN
#endif

// add support of syntax highlight to our string formatting methods
#if defined(Q_COMPILER_CLANG) || defined(Q_COMPILER_GCC)
#define Q_CRT_STRING_FORMAT_ATTRIBUTE(METHOD, STRING_INDEX, FIRST_INDEX) [[gnu::format(METHOD, STRING_INDEX, FIRST_INDEX)]]
#elif defined(__RESHARPER__)
#define Q_CRT_STRING_FORMAT_ATTRIBUTE(METHOD, STRING_INDEX, FIRST_INDEX) [[rscpp::format(METHOD, STRING_INDEX, FIRST_INDEX)]]
#else
#define Q_CRT_STRING_FORMAT_ATTRIBUTE(METHOD, STRING_INDEX, FIRST_INDEX)
#endif

#if defined(Q_COMPILER_CLANG) && Q_HAS_FEATURE(address_sanitizer)
#if Q_HAS_ATTRIBUTE(__no_sanitize__)
#define Q_CRT_NO_SANITIZE __attribute__((__no_sanitize__("address")))
#elif Q_HAS_ATTRIBUTE(__no_sanitize_address__)
#define Q_CRT_NO_SANITIZE __attribute__((__no_sanitize_address__))
#elif Q_HAS_ATTRIBUTE(__no_address_safety_analysis__)
#define Q_CRT_NO_SANITIZE __attribute__((__no_address_safety_analysis__))
#endif
#elif defined(Q_COMPILER_GCC) && __SANITIZE_ADDRESS__
#define Q_CRT_NO_SANITIZE __attribute__((__no_sanitize_address__))
#else
#define Q_CRT_NO_SANITIZE
#endif

/*
 * C-RUNTIME
 * - rebuild of C standard library and partially STL
 */
namespace CRT
{
	/*
	 * @section: algorithm
	 * - functions for a variety of purposes (e.g. searching, sorting, counting, manipulating) that operate on ranges of elements
	 */
	#pragma region stl_algorithm
	/// alternative of 'std::min()'
	/// @returns: minimal value of the given comparable values
	template <typename T>
	[[nodiscard]] Q_INLINE constexpr const T& Min(const T& left, const T& right) noexcept
	{
		return (right < left) ? right : left;
	}

	/// alternative of 'std::max()'
	/// @returns: maximal value of the given comparable values
	template <typename T>
	[[nodiscard]] Q_INLINE constexpr const T& Max(const T& left, const T& right) noexcept
	{
		return (right > left) ? right : left;
	}

	/// alternative of 'std::clamp()'
	/// @returns: value clamped in range [@a`minimal` .. @a`maximal`]
	template <typename T>
	[[nodiscard]] Q_INLINE constexpr const T& Clamp(const T& value, const T& minimal, const T& maximal) noexcept
	{
		return (value < minimal) ? minimal : (value > maximal) ? maximal : value;
	}
	#pragma endregion

	/* @section: memory */
	#pragma region crt_memory
	/// compare bytes in two buffers, alternative of 'memcmp()'
	/// @remarks: compares the first @a`nCount` bytes of @a`pFirstBuffer` and @a`pRightBuffer` and return a value that indicates their relationship, performs unsigned character comparison
	/// @returns: <0 - if @a`pFirstBuffer` less than @a`pRightBuffer`, 0 - if @a`pFirstBuffer` identical to @a`pRightBuffer`, >0 - if @a`pFirstBuffer` greater than @a`pRightBuffer`
	Q_INLINE Q_CRT_DIAGNOSE_AS_BUILTIN(__builtin_memcmp, 1, 2, 3) inline int MemoryCompare(const void* pLeftBuffer, const void* pRightBuffer, std::size_t nCount)
	{
		auto pLeftByte = static_cast<const std::uint8_t*>(pLeftBuffer);
		auto pRightByte = static_cast<const std::uint8_t*>(pRightBuffer);

		while (nCount-- != 0U)
		{
			if (const std::uint8_t uLeft = *pLeftByte++, uRight = *pRightByte++; uLeft != uRight)
				return uLeft - uRight;
		}

		return 0;
	}

	/// compare bytes in two buffers, alternative of 'wmemcmp()'
	/// @remarks: compares the first @a`nCount` characters of @a`pwLeftBuffer` and @a`pwRightBuffer` and return a value that indicates their relationship, performs signed character comparison
	/// @returns: <0 - if @a`pwLeftBuffer` less than @a`pwRightBuffer`, 0 - if @a`pwLeftBuffer` identical to @a`pwRightBuffer`, >0 - if @a`pwLeftBuffer` greater than @a`pwRightBuffer`
	Q_INLINE inline int MemoryCompareW(const wchar_t* pwLeftBuffer, const wchar_t* pwRightBuffer, std::size_t nCount)
	{
		while (nCount-- != 0U)
		{
			if (const wchar_t wchLeft = *pwLeftBuffer++, wchRight = *pwRightBuffer++; wchLeft != wchRight)
				return wchLeft - wchRight;
		}

		return 0;
	}

	/// find character in a buffer, alternative of 'memchr()'
	/// @remarks: looks for the first occurrence of @a`uSearch` byte in the first @a`nCount` bytes of @a`pBuffer`, performs unsigned comparison for elements
	/// @returns: pointer to the first occurence of @a`uSearch` byte in @a`pBuffer` on success, null otherwise
	Q_INLINE inline void* MemoryChar(const void* pBuffer, const std::uint8_t uSearch, std::size_t nCount)
	{
		auto pByte = static_cast<const std::uint8_t*>(pBuffer);

		while (nCount-- != 0U)
		{
			if (*pByte == uSearch)
				return const_cast<std::uint8_t*>(pByte);

			++pByte;
		}

		return nullptr;
	}

	/// find wide character in a buffer, alternative of 'wmemchr()'
	/// @remarks: looks for the first occurrence of @a`wSearch` character in the first @a`nCount` characters of @a`pwBuffer`, performs signed comparison for elements
	/// @returns: pointer to the first occurence of @a`wSearch` character in @a`pwBuffer` on success, null otherwise
	Q_INLINE inline wchar_t* MemoryCharW(wchar_t* pwBuffer, const wchar_t wSearch, std::size_t nCount)
	{
		while (nCount-- != 0U)
		{
			if (*pwBuffer == wSearch)
				return pwBuffer;

			++pwBuffer;
		}

		return nullptr;
	}

	/// search for one buffer inside another, alternative of 'memmem()'
	/// @remarks: looks for the first occurrence of @a`pSearchBuffer` that @a`nSearchLength` bytes long in the first @a`nSourceLength` bytes of @a`pSourceBuffer`, performs unsigned comparison for elements
	/// @returns: pointer to the first occurrence of @a`pSearchBuffer` in @a`pSourceBuffer` on success, null otherwise
	Q_INLINE inline void* MemoryMemory(const void* pSourceBuffer, const std::size_t nSourceLength, const void* pSearchBuffer, const std::size_t nSearchLength)
	{
		if (nSearchLength == 0U || nSourceLength < nSearchLength)
			return nullptr;

		const auto pCurrentSource = static_cast<const std::uint8_t*>(pSourceBuffer);
		for (std::size_t i = 0U; i <= nSourceLength - nSearchLength; i++)
		{
			if (MemoryCompare(pCurrentSource + i, pSearchBuffer, nSearchLength) == 0)
				return const_cast<std::uint8_t*>(pCurrentSource + i);
		}

		return nullptr;
	}

	/// set a buffer to a specified byte, alternative of 'mempset()'
	/// @remarks: sets the first @a`nCount` bytes of @a`pDestination` to the @a`uByte` value
	/// @returns: pointer to the @a`pDestination` advanced by @a'nCount'
	Q_INLINE /*Q_CRT_DIAGNOSE_AS_BUILTIN(__builtin_memset, 1, 2, 3) @todo: do diagnose only when declare intrinsics? */ Q_CRT_NO_BUILTIN inline void* MemorySet(void* pDestination, const std::uint8_t uByte, std::size_t nCount)
	{
		auto pCurrentDestination = static_cast<std::uint8_t*>(pDestination);

	#ifdef Q_ISA_SSE2
		// copy the max of owords
		const __m128i arrByte = ::_mm_set1_epi8(uByte);
		for (std::size_t i = 0U; i < (nCount >> 4U); i += 16U)
		{
			::_mm_storeu_si128(reinterpret_cast<__m128i*>(pCurrentDestination), arrByte);
			pCurrentDestination += sizeof(__m128i);
		}
		nCount &= 15U;
	#endif
	#ifdef Q_COMPILER_MSC
	#ifdef Q_ARCH_X86_64
		// copy the max of qwords
		::__stosq(reinterpret_cast<std::uint64_t*>(pCurrentDestination), static_cast<std::uint64_t>(uByte) * 0x0101010101010101ULL, nCount >> 3U);
		pCurrentDestination += nCount & ~7U;
		nCount &= 7U;
	#endif
		// copy the rest of dwords
		::__stosd(reinterpret_cast<unsigned long*>(pCurrentDestination), static_cast<unsigned long>(uByte) * 0x01010101UL, nCount >> 2U);
		pCurrentDestination += nCount & ~3U;
		nCount &= 3U;
		// copy the rest of words
		::__stosw(reinterpret_cast<std::uint16_t*>(pCurrentDestination), static_cast<std::uint16_t>(uByte | (uByte << 8U)), nCount >> 1U);
		pCurrentDestination += nCount & ~1U;
		nCount &= 1U;
	#endif
		// copy the rest of bytes
	#ifdef Q_COMPILER_CLANG
		/*
		 * @note: llvm implements '__stosb' intrinsic as volatile 'memset' that may lead to infinity recursion
		 * @source: https://lists.llvm.org/pipermail/cfe-commits/Week-of-Mon-20161003/172885.html
		 */
		while (nCount-- != 0U)
			*pCurrentDestination++ = uByte;
	#else
		::__stosb(pCurrentDestination, uByte, nCount);
		pCurrentDestination += nCount;
	#endif

		return pCurrentDestination;
	}

	/// set a buffer to a specified wide character, alternative of 'wmempset()'
	/// @remarks: sets the first @a`nCount` wide characters of @a`pwDestination` to the @a`wChar` value
	/// @returns: pointer to the @a`pwDestination` advanced by @a'nCount'
	Q_INLINE Q_CRT_DIAGNOSE_AS_BUILTIN(__builtin_wmemset, 1, 2, 3) Q_CRT_NO_BUILTIN inline wchar_t* MemorySetW(wchar_t* pwDestination, const wchar_t wChar, std::size_t nCount)
	{
		while (nCount-- != 0U)
			*pwDestination++ = wChar;

		return pwDestination;
	}

	/// copy one buffer to another, alternative of 'mempcpy()'
	/// @remarks: copies the initial @a`nCount` bytes from @a`pSource` to @a`pDestination`. if the source and destination regions overlap, the behavior is undefined
	/// @returns: pointer to the @a`pDestination` advanced by @a'nCount'
	Q_INLINE Q_CRT_DIAGNOSE_AS_BUILTIN(__builtin_memcpy, 1, 2, 3) Q_CRT_NO_BUILTIN inline void* MemoryCopy(void* pDestination, const void* pSource, std::size_t nCount)
	{
		auto pCurrentDestination = static_cast<std::uint8_t*>(pDestination);
		auto pCurrentSource = static_cast<const std::uint8_t*>(pSource);

	#ifdef Q_ISA_SSE2
		// copy the max of owords
		for (std::size_t i = 0U; i < (nCount >> 4U); i += 16U)
		{
			::_mm_storeu_si128(reinterpret_cast<__m128i*>(pCurrentDestination), ::_mm_loadu_si128(reinterpret_cast<const __m128i*>(pCurrentSource)));
			pCurrentDestination += sizeof(__m128i);
			pCurrentSource += sizeof(__m128i);
		}
		nCount &= 15U;
	#endif
	#ifdef Q_COMPILER_MSC
		std::size_t nCopiedCount;
	#ifdef Q_ARCH_X86_64
		// copy the max of qwords
		::__movsq(reinterpret_cast<std::uint64_t*>(pCurrentDestination), reinterpret_cast<const std::uint64_t*>(pCurrentSource), nCount >> 3U);
		nCopiedCount = nCount & ~7U;
		pCurrentDestination += nCopiedCount;
		pCurrentSource += nCopiedCount;
		nCount &= 7U;
	#endif
		// copy the rest of dwords
		::__movsd(reinterpret_cast<unsigned long*>(pCurrentDestination), reinterpret_cast<const unsigned long*>(pCurrentSource), nCount >> 2U);
		nCopiedCount = nCount & ~3U;
		pCurrentDestination += nCopiedCount;
		pCurrentSource += nCopiedCount;
		nCount &= 3U;
		// copy the rest of words
		::__movsw(reinterpret_cast<std::uint16_t*>(pCurrentDestination), reinterpret_cast<const std::uint16_t*>(pCurrentSource), nCount >> 1U);
		nCopiedCount = nCount & ~1U;
		pCurrentDestination += nCopiedCount;
		pCurrentSource += nCopiedCount;
		nCount &= 1U;
	#endif
		// copy the rest of bytes
	#ifdef Q_COMPILER_CLANG
		// @test: not sure if its same as memset getting optimized into memcpy call
		while (nCount-- != 0U)
			*pCurrentDestination++ = *pCurrentSource++;
	#else
		::__movsb(pCurrentDestination, pCurrentSource, nCount);
		pCurrentDestination += nCount;
	#endif

		return pCurrentDestination;
	}

	/// copy one wide buffer to another, alternative of 'wmempcpy()'
	/// @remarks: copies the initial @a`nCount` wide characters from @a`pSource` to @a`pwDestination`. if the source and destination regions overlap, the behavior is undefined
	/// @returns: pointer to the @a`pwDestination` advanced by @a'nCount'
	Q_INLINE Q_CRT_DIAGNOSE_AS_BUILTIN(__builtin_memcpy, 1, 2, 3) Q_CRT_NO_BUILTIN inline wchar_t* MemoryCopyW(wchar_t* pwDestination, const wchar_t* pwSource, std::size_t nCount)
	{
		while (nCount-- != 0U)
			*pwDestination++ = *pwSource++;

		return pwDestination;
	}

	/// move one buffer to another, alternative of 'memmove()'
	/// @remarks: copies the initial @a`nCount` bytes from @a`pSource` to @a`pDestination`. if some portions of the source and the destination regions overlap, both functions ensure that the original source bytes in the overlapping region are copied before being overwritten
	/// @returns: pointer to the @a`pDestination`
	Q_INLINE Q_CRT_DIAGNOSE_AS_BUILTIN(__builtin_memmove, 1, 2, 3) Q_CRT_NO_BUILTIN inline void* MemoryMove(void* pDestination, const void* pSource, std::size_t nCount)
	{
		auto pCurrentDestination = static_cast<std::uint8_t*>(pDestination);
		auto pCurrentSource = static_cast<const std::uint8_t*>(pSource);

		// check if buffers don't overlap, copy from lower to higher addresses
		if (pCurrentDestination <= pCurrentSource || pCurrentDestination >= pCurrentSource + nCount)
		{
		#ifdef Q_COMPILER_MSC
			std::size_t nCopiedCount;
		#ifdef Q_ARCH_X86_64
			// copy the max of qwords
			::__movsq(reinterpret_cast<std::uint64_t*>(pCurrentDestination), reinterpret_cast<const std::uint64_t*>(pCurrentSource), nCount >> 3U);
			nCopiedCount = nCount & ~7U;
			pCurrentDestination += nCopiedCount;
			pCurrentSource += nCopiedCount;
			nCount &= 7U;
		#endif
			// copy the rest of dwords
			::__movsd(reinterpret_cast<unsigned long*>(pCurrentDestination), reinterpret_cast<const unsigned long*>(pCurrentSource), nCount >> 2U);
			nCopiedCount = nCount & ~3U;
			pCurrentDestination += nCopiedCount;
			pCurrentSource += nCopiedCount;
			nCount &= 3U;
			// copy the rest of words
			::__movsw(reinterpret_cast<std::uint16_t*>(pCurrentDestination), reinterpret_cast<const std::uint16_t*>(pCurrentSource), nCount >> 1U);
			nCopiedCount = nCount & ~1U;
			pCurrentDestination += nCopiedCount;
			pCurrentSource += nCopiedCount;
			nCount &= 1U;
		#endif
			// copy the rest of bytes
			while (nCount-- != 0U)
				*pCurrentDestination++ = *pCurrentSource++;
		}
		// otherwise buffers overlapping, copy from higher to lower addresses
		else
		{
			std::uint8_t* pDestinationEnd = pCurrentDestination + (nCount - 1U);
			const std::uint8_t* pSourceEnd = pCurrentSource + (nCount - 1U);

			while (nCount-- != 0U)
				*pDestinationEnd-- = *pSourceEnd--;
		}

		return pDestination;
	}

	/// move one wide buffer to another, alternative of 'wmemmove()'
	/// @remarks: copies the initial @a`nCount` wide characters from @a`pwSource` to @a`pwDestination`. if some portions of the source and the destination regions overlap, both functions ensure that the original source characters in the overlapping region are copied before being overwritten
	/// @returns: pointer to the @a`pwDestination`
	Q_INLINE Q_CRT_DIAGNOSE_AS_BUILTIN(__builtin_wmemmove, 1, 2, 3) Q_CRT_NO_BUILTIN inline wchar_t* MemoryMoveW(wchar_t* pwDestination, const wchar_t* pwSource, std::size_t nCount)
	{
		auto pwCurrentDestination = pwDestination;

		// check if buffers don't overlap, copy from lower to higher addresses
		if (pwCurrentDestination <= pwSource || pwCurrentDestination >= pwSource + nCount)
		{
			while (nCount-- != 0U)
				*pwCurrentDestination++ = *pwSource++;
		}
		// otherwise buffers overlapping, copy from higher to lower addresses
		else
		{
			pwCurrentDestination += nCount - 1U;
			pwSource += nCount - 1U;

			while (nCount-- != 0U)
				*pwCurrentDestination-- = *pwSource--;
		}

		return pwDestination;
	}

	#ifdef Q_CRT_MEMORY_CRYPTO
	namespace CRYPTO
	{
		#include "crypto/memory.inl"
	}
	#endif
	#pragma endregion

	/*
	 * @section: character
	 * - valid only for default C locale, unless 'Q_CRT_STRING_WIDE_TYPE' is defined
	 */
	#pragma region crt_characters
	#ifdef Q_CRT_STRING_WIDE_TYPE
	#include "string/wctype.inl"
	#endif

	/// alternative of 'iscntrl()'
	/// @returns: true if given character is a control character, false otherwise
	[[nodiscard]] constexpr bool IsControl(const int iChar)
	{
		return ((iChar >= 0 && iChar <= 0x1F) || iChar == 0x7F);
	}

	/// alternative of 'iswcntrl()'
	/// @returns: true if given wide character is a control character, false otherwise
	[[nodiscard]] constexpr bool IsControl(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & TYPE_CONTROL) != 0U;
	#else
		return (wChar <= 0x1F || wChar == 0x7F);
	#endif
	}

	/// alternative of 'isdigit()'
	/// @returns: true if given character is decimal digit, false otherwise
	[[nodiscard]] constexpr bool IsDigit(const int iChar)
	{
		return (iChar >= '0' && iChar <= '9');
	}

	/// alternative of 'iswdigit()'
	/// @returns: true if given wide character is decimal digit, false otherwise
	[[nodiscard]] constexpr bool IsDigit(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & TYPE_DIGIT) != 0U;
	#else
		return (wChar >= L'0' && wChar <= L'9');
	#endif
	}

	/// alternative of 'isxdigit()'
	/// @returns: true if given character is hexadecimal digit, false otherwise
	[[nodiscard]] constexpr bool IsHexDigit(const int iChar)
	{
		return ((iChar >= '0' && iChar <= '9') || (iChar >= 'A' && iChar <= 'F') || (iChar >= 'a' && iChar <= 'f'));
	}

	/// alternative of 'iswxdigit()'
	/// @returns: true if given wide character is hexadecimal digit, false otherwise
	[[nodiscard]] constexpr bool IsHexDigit(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return ((wChar >= L'0' && wChar <= L'9') || (wChar >= L'A' && wChar <= L'F') || (wChar >= L'a' && wChar <= L'f') ||
			(wChar >= 0xFF10 && wChar <= 0xFF19) || (wChar >= 0xFF21 && wChar <= 0xFF26) || (wChar >= 0xFF41 && wChar <= 0xFF46));
	#else
		return ((wChar >= L'0' && wChar <= L'9') || (wChar >= L'A' && wChar <= L'F') || (wChar >= L'a' && wChar <= L'f'));
	#endif
	}

	/// alternative of 'isblank()'
	/// @returns: true if given character is blank, false otherwise
	[[nodiscard]] constexpr bool IsBlank(const int iChar)
	{
		return (iChar == '\t' || iChar == ' ');
	}

	/// alternative of 'iswblank()'
	/// @returns: true if given wide character is blank, false otherwise
	[[nodiscard]] constexpr bool IsBlank(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & TYPE_BLANK) != 0U;
	#else
		return (wChar == L'\t' || wChar == L' ');
	#endif
	}

	/// alternative of 'isspace()'
	/// @returns: true if given character is whitespace, false otherwise
	[[nodiscard]] constexpr bool IsSpace(const int iChar)
	{
		return (static_cast<std::uint32_t>(iChar - '\t') <= ('\r' - '\t') || iChar == ' ');
	}

	/// alternative of 'iswspace()'
	/// @returns: true if given wide character is whitespace, false otherwise
	[[nodiscard]] constexpr bool IsSpace(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & TYPE_SPACE) != 0U;
	#else
		return ((wChar >= L'\t' && wChar <= L'\r') || wChar == L' ');
	#endif
	}

	/// alternative of 'isalpha()'
	/// @returns: true if given character is alphabetic, false otherwise
	[[nodiscard]] constexpr bool IsAlpha(const int iChar)
	{
		return ((iChar >= 'A' && iChar <= 'Z') || (iChar >= 'a' && iChar <= 'z'));
	}

	/// alternative of 'iswalpha()'
	/// @returns: true if given wide character is alphabetic, false otherwise
	[[nodiscard]] constexpr bool IsAlpha(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & (TYPE_LOWER | TYPE_UPPER | TYPE_ALPHA)) != 0U;
	#else
		return ((wChar >= L'0' && wChar <= L'9') || (wChar >= L'A' && wChar <= L'Z') || (wChar >= L'a' && wChar <= L'z'));
	#endif
	}

	/// alternative of 'isalnum()'
	/// @returns: true if given character is alphabetic or numeric, false otherwise
	[[nodiscard]] constexpr bool IsAlphaNum(const int iChar)
	{
		return ((iChar >= '0' && iChar <= '9') || (iChar >= 'A' && iChar <= 'Z') || (iChar >= 'a' && iChar <= 'z'));
	}

	/// alternative of 'iswalnum()'
	/// @returns: true if given wide character is alphabetic or numeric, false otherwise
	[[nodiscard]] constexpr bool IsAlphaNum(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & (TYPE_DIGIT | TYPE_LOWER | TYPE_UPPER | TYPE_ALPHA)) != 0U;
	#else
		return ((wChar >= L'0' && wChar <= L'9') || (wChar >= L'A' && wChar <= L'Z') || (wChar >= L'a' && wChar <= L'z'));
	#endif
	}

	/// alternative of 'isprint()'
	/// @returns: true if given character is printable, false otherwise
	[[nodiscard]] constexpr bool IsPrint(const int iChar)
	{
		return (iChar >= ' ' && iChar <= '~');
	}

	/// alternative of 'iswprint()'
	/// @returns: true if given wide character is printable, false otherwise
	[[nodiscard]] constexpr bool IsPrint(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return wChar != L'\t' && (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & (TYPE_BLANK | TYPE_PUNCT | TYPE_DIGIT | TYPE_LOWER | TYPE_UPPER | TYPE_ALPHA)) != 0U;
	#else
		return (wChar >= L' ' && wChar <= L'~');
	#endif
	}

	/// alternative of 'isgraph()'
	/// @returns: true if given character has a graphical representation, false otherwise
	[[nodiscard]] constexpr bool IsGraph(const int iChar)
	{
		return (iChar >= '!' && iChar <= '~');
	}

	/// alternative of 'iswgraph()'
	/// @returns: true if given wide character has a graphical representation, false otherwise
	[[nodiscard]] constexpr bool IsGraph(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & (TYPE_PUNCT | TYPE_DIGIT | TYPE_LOWER | TYPE_UPPER | TYPE_ALPHA)) != 0U;
	#else
		return (wChar >= L'!' && wChar <= L'~');
	#endif
	}

	/// alternative of 'ispunct()'
	/// @returns: true if given character is a punctuation character, false otherwise
	[[nodiscard]] constexpr bool IsPunct(const int iChar)
	{
		return ((iChar >= '!' && iChar <= '/') || (iChar >= ':' && iChar <= '@') || (iChar >= '[' && iChar <= '`') || (iChar >= '{' && iChar <= '~'));
	}

	/// alternative of 'iswpunct()'
	/// @returns: true if given wide character is a punctuation character, false otherwise
	[[nodiscard]] constexpr bool IsPunct(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & TYPE_PUNCT) != 0U;
	#else
		return ((wChar >= L'!' && wChar <= L'/') || (wChar >= L':' && wChar <= L'@') || (wChar >= L'[' && wChar <= L'`') || (wChar >= L'{' && wChar <= L'~'));
	#endif
	}

	/// alternative of 'isupper()'
	/// @returns: true if given alphabetic character is uppercase, false otherwise
	[[nodiscard]] constexpr bool IsUpper(const int iChar)
	{
		return (iChar >= 'A' && iChar <= 'Z');
	}

	/// alternative of 'iswupper()'
	/// @returns: true if given wide character is uppercase, false otherwise
	[[nodiscard]] constexpr bool IsUpper(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & TYPE_UPPER) != 0U;
	#else
		return (wChar >= L'A' && wChar <= L'Z');
	#endif
	}

	/// alternative of 'islower()'
	/// @returns: true if given alphabetic character is lowercase, false otherwise
	[[nodiscard]] constexpr bool IsLower(const int iChar)
	{
		return (iChar >= 'a' && iChar <= 'z');
	}

	/// alternative of 'iswlower()'
	/// @returns: true if given wide character is lowercase, false otherwise
	[[nodiscard]] constexpr bool IsLower(const wint_t wChar)
	{
	#ifdef Q_CRT_STRING_WIDE_TYPE
		return (arrWideCharacterTypeLUT[arrWideCharacterTypeOffsets[wChar >> 5U] + (wChar & 0x1F)] & TYPE_LOWER) != 0U;
	#else
		return (wChar >= L'a' && wChar <= L'z');
	#endif
	}
	#pragma endregion

	/*
	 * @section: character conversion
	 * - valid only for default C locale, unless 'Q_CRT_STRING_WIDE_CASE' is defined
	 */
	#pragma region crt_character_convert
	// convert single character to uppercase, alternative of 'toupper()'
	[[nodiscard]] constexpr int CharToUpper(const int iChar)
	{
		return (IsLower(iChar) ? (iChar & ~('a' ^ 'A')) : iChar);
	}

	// convert single wide character to uppercase, alternative of 'towupper()'
	[[nodiscard]] constexpr wint_t CharToUpper(const wint_t wChar)
	{
		return (IsLower(wChar) ? (wChar & ~('a' ^ 'A')) : wChar);
	}

	// convert single character to lowercase, alternative of 'tolower()'
	[[nodiscard]] constexpr int CharToLower(const int iChar)
	{
		return (IsUpper(iChar) ? (iChar | ('a' ^ 'A')) : iChar);
	}

	// convert single wide character to lowercase, alternative of 'towlower()'
	[[nodiscard]] constexpr wint_t CharToLower(const wint_t wChar)
	{
		return (IsUpper(wChar) ? (wChar | ('a' ^ 'A')) : wChar);
	}
	#pragma endregion

	/*
	 * @section: string
	 * - @note: return value of some functions correspond to the POSIX standard but not C standard. it was necessary to reduce time complexity when use those
	 * - valid only for default C locale
	 */
	#pragma region crt_string
	/// get the length of a string, alternative of 'strlen()', 'wcslen()'
	/// @returns: number of characters in @a`tszSource`, not including the terminating null character
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr std::size_t StringLength(const T* tszSource)
	{
		const T* tszSourceEnd = tszSource;

		if constexpr (sizeof(T) != 4U)
		{
			if (!std::is_constant_evaluated())
			{
				// get up to 4-byte alignment
				while ((reinterpret_cast<std::uintptr_t>(tszSourceEnd) & 3U) != 0U)
				{
					if (*tszSourceEnd == '\0')
						return tszSourceEnd - tszSource;

					++tszSourceEnd;
				}

				// scan over 4 bytes at a time to find the terminating null
				while (true)
				{
					const std::uint32_t uBits = *reinterpret_cast<const std::uint32_t*>(tszSourceEnd);

					// check if any of the bytes is zero
					if constexpr (sizeof(T) == 2U)
					{
						if ((((uBits - 0x00010001) & ~uBits) & 0x80008000) != 0U)
							break;
					}
					else if constexpr (sizeof(T) == 1U)
					{
						if ((((uBits - 0x01010101) & ~uBits) & 0x80808080) != 0U)
							break;
					}

					tszSourceEnd += (sizeof(std::uint32_t) / sizeof(T));
				}
			}
		}

		while (*tszSourceEnd != '\0')
			++tszSourceEnd;

		return tszSourceEnd - tszSource;
	}

	/// get the length of a string limited by the max length, alternative of 'strnlen()', 'wcsnlen()'
	/// @returns: number of characters in @a`tszSource`, not including the terminating null character. if there is no null terminator within the first @a`nMaxLength` bytes of the string, then @a`nMaxLength` is returned to indicate the error condition
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	Q_CRT_NO_SANITIZE constexpr std::size_t StringLengthN(const T* tszSource, std::size_t nMaxLength)
	{
		const T* tszSourceEnd = tszSource;

		if constexpr (sizeof(T) != 4U)
		{
			if (!std::is_constant_evaluated())
			{
				// get up to 4-byte alignment
				while ((reinterpret_cast<std::uintptr_t>(tszSourceEnd) & 3U) != 0U)
				{
					if (*tszSourceEnd == '\0')
						return tszSourceEnd - tszSource;

					++tszSourceEnd;
				}

				// scan over 4 bytes at a time to find the terminating null
				// @note: read past the end of the buffer, but guaranteed to never cross the page boundaries
				while (nMaxLength >= 4U)
				{
					const std::uint32_t uBits = *reinterpret_cast<const std::uint32_t*>(tszSourceEnd);

					// check if any of the bytes is zero
					if constexpr (sizeof(T) == 2U)
					{
						if ((((uBits - 0x00010001) & ~uBits) & 0x80008000) != 0U)
							break;
					}
					else if constexpr (sizeof(T) == 1U)
					{
						if ((((uBits - 0x01010101) & ~uBits) & 0x80808080) != 0U)
							break;
					}

					tszSourceEnd += (sizeof(std::uint32_t) / sizeof(T));
					nMaxLength -= 4U;
				}
			}
		}

		while (nMaxLength != 0U && *tszSourceEnd != '\0')
		{
			++tszSourceEnd;
			--nMaxLength;
		}

		return static_cast<std::size_t>(tszSourceEnd - tszSource);
	}

	/// compare two strings, alternative of 'strcmp()', 'wcscmp()'
	/// @remarks: compares @a`tszLeft` and @a`tszRight` strings and return a value that indicates their relationship
	/// @returns: <0 - if @a`tszLeft` less than @a`tszRight`, 0 - if @a`tszLeft` is identical to @a`tszRight`, >0 - if @a`tszLeft` greater than @a`tszRight`
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr int StringCompare(const T* tszLeft, const T* tszRight)
	{
		T tchLeft, tchRight;
		do
		{
			tchLeft = *tszLeft++;
			tchRight = *tszRight++;

			if (tchLeft == '\0')
				break;
		} while (tchLeft == tchRight);

		return tchLeft - tchRight;
	}
	
	/// compare two strings up to the specified count of characters, alternative of 'strncmp()', 'wcsncmp()'
	/// @remarks: compares at most the first @a`nCount` characters of @a`tszLeft` and @a`tszRight` strings and return a value that indicates their relationship
	/// @returns: <0 - if @a`tszLeft` less than @a`tszRight`, 0 - if @a`tszLeft` is identical to @a`tszRight`, >0 - if @a`tszLeft` greater than @a`tszRight`
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr int StringCompareN(const T* tszLeft, const T* tszRight, std::size_t nCount)
	{
		T tchLeft, tchRight;
		while (nCount-- != 0U)
		{
			tchLeft = *tszLeft++;
			tchRight = *tszRight++;

			if (tchLeft != tchRight)
				return tchLeft - tchRight;

			if (tchLeft == '\0')
				break;
		}

		return 0;
	}

	/// case-insensitive compare two strings, alternative of 'stricmp()', 'wcsicmp()'
	/// @remarks: compares @a`tszLeft` to @a`tszRight` and return a value that indicates their relationship, performs conversion of each character to lowercase before comparison
	/// @returns: <0 - if @a`tszLeft` less than @a`tszRight`, 0 - if @a`tszLeft` is identical to @a`tszRight`, >0 - if @a`tszLeft` greater than @a`tszRight`
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr int StringCompareI(const T* tszLeft, const T* tszRight)
	{
		using ComparisonType_t = std::conditional_t<std::is_same_v<T, char>, int, wint_t>;

		ComparisonType_t nLeft, nRight;
		do
		{
			nLeft = CharToLower(static_cast<ComparisonType_t>(*tszLeft++));
			nRight = CharToLower(static_cast<ComparisonType_t>(*tszRight++));

			if (nLeft == '\0')
				break;
		} while (nLeft == nRight);

		return nLeft - nRight;
	}

	/// case-insensitive compare two strings up to the specified count of characters, alternative of 'strnicmp()', 'wcsnicmp()'
	/// @remarks: compares at most the first @a`nCount` characters of @a`tszLeft` and @a`tszRight` strings and return a value that indicates their relationship, performs conversion of each character to lowercase before comparison
	/// @returns: <0 - if @a`tszLeft` less than @a`tszRight`, 0 - if @a`tszLeft` is identical to @a`tszRight`, >0 - if @a`tszLeft` greater than @a`tszRight`
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr int StringCompareNI(const T* tszLeft, const T* tszRight, std::size_t nCount)
	{
		using ComparisonType_t = std::conditional_t<std::is_same_v<T, char>, int, wint_t>;

		ComparisonType_t nLeft, nRight;
		while (nCount-- != 0U)
		{
			nLeft = CharToLower(static_cast<ComparisonType_t>(*tszLeft++));
			nRight = CharToLower(static_cast<ComparisonType_t>(*tszRight++));

			if (nLeft != nRight)
				return nLeft - nRight;

			if (nLeft == '\0')
				break;
		}

		return 0;
	}

	/// find a character in a string, alternative of 'strchr()', 'wcschr()'
	/// @remarks: the terminating null character is included in the search
	/// @returns: pointer to the first occurrence of @a`tchSearch` character in @a`tszSource` on success, null otherwise
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringChar(const T* tszSource, const T tchSearch)
	{
		do
		{
			if (*tszSource == tchSearch)
				return const_cast<T*>(tszSource);
		} while (*tszSource++ != '\0');

		return nullptr;
	}

	/// find a last occurrence of character in a string, alternative of 'strrchr()', 'wcsrchr()'
	/// @remarks: the terminating null character is included in the search
	/// @returns: pointer to the last occurrence of @a`tchSearch` character in @a`tszSource` on success, null otherwise
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringCharR(const T* tszSource, const T tchSearch)
	{
		T* tszLastOccurrence = nullptr;

		do
		{
			if (*tszSource == tchSearch)
				tszLastOccurrence = const_cast<T*>(tszSource);
		} while (*tszSource++ != '\0');

		return tszLastOccurrence;
	}

	/// search for one string inside another, alternative of 'strstr()', 'wcsstr()'
	/// @remarks: finds the first occurrence of @a`tszSearch` substring in @a`tszSource`. the search does not include terminating null character
	/// @returns: pointer to the first occurrence of @a`tszSearch` substring in @a`tszSource` on success, null otherwise
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringString(const T* tszSource, const T* tszSearch)
	{
		const T* tszCurrentSource = tszSource;
		const T* tszCurrentSearch = tszSearch;

		while (true)
		{
			if (*tszCurrentSearch == '\0')
				return const_cast<T*>(tszSource);

			if (*tszCurrentSource == '\0')
				break;

			if (*tszCurrentSource++ != *tszCurrentSearch++)
			{
				tszCurrentSource = ++tszSource;
				tszCurrentSearch = tszSearch;
			}
		}

		return nullptr;
	}

	/// search for one string inside another up to the specified count of characters, alternative of 'strnstr()'
	/// @remarks: finds the first occurrence of @a`tszSearch` substring in @a`tszSource`, where not more than @a`nCount` characters are searched
	/// @returns: pointer to the first occurrence of @a`tszSearch` substring in @a`tszSource` on success, null otherwise
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringStringN(const T* tszSource, const T* tszSearch, const std::size_t nSourceLength)
	{
		if (*tszSearch == '\0')
			return const_cast<T*>(tszSource);

		while (*tszSource != '\0')
		{
			// do single comparison before entering the loop
			if (*tszSource == *tszSearch++)
			{
				std::size_t nRemainingLength = nSourceLength;
				const T* tszCurrentSource = tszSource + 1;
				const T* tszCurrentSearch = tszSearch + 1;
				while (nRemainingLength-- != 0U && *tszCurrentSource != '\0' && *tszCurrentSearch != '\0')
				{
					++tszCurrentSource;
					++tszCurrentSearch;
				}

				if (*tszCurrentSearch == '\0')
					return const_cast<T*>(tszSource);
			}

			++tszSource;
		}

		return nullptr;
	}

	/// case-insensitive search for one string inside another, alternative of 'strcasestr()'
	/// @remarks: finds the first occurrence of @a`tszSearch` substring in @a`tszSource`, performs conversion of each character to lowercase before comparison. the search does not include terminating null character
	/// @returns: pointer to the first occurrence of @a`tszSearch` substring in @a`tszSource` on success, null otherwise
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringStringI(const T* tszSource, const T* tszSearch)
	{
		using ComparisonType_t = std::conditional_t<std::is_same_v<T, char>, int, wint_t>;

		const T* tszCurrentSource = tszSource;
		const T* tszCurrentSearch = tszSearch;

		while (true)
		{
			if (*tszCurrentSearch == '\0')
				return const_cast<T*>(tszSource);

			if (*tszCurrentSource == '\0')
				break;

			if (CharToLower(static_cast<ComparisonType_t>(*tszCurrentSource++)) != CharToLower(static_cast<ComparisonType_t>(*tszCurrentSearch++)))
			{
				tszCurrentSource = ++tszSource;
				tszCurrentSearch = tszSearch;
			}
		}

		return nullptr;
	}
	
	/// scan the string for characters not in specified character set, alternative of 'strpspn()', 'wcspspn()'
	/// @remarks: search doesn't include the terminating null character
	/// @returns: pointer to the first occurence of a character in @a`tszSource` that doesn't belong to the @a`tszSet` set of characters
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringSpan(const T* tszSource, const T* tszSet)
	{
		const T* tszSourceCurrent = tszSource;
		while (*tszSourceCurrent != '\0')
		{
			// @test: we may get rid of this loop by making bitset of the set characters, reduces time complexity from O(mn) to O(m+n) | only for ansi set
			const T* tszSetCurrent = tszSet;
			while (*tszSetCurrent != '\0')
			{
				if (*tszSourceCurrent == *tszSetCurrent)
					break;

				++tszSetCurrent;
			}

			if (*tszSetCurrent == '\0')
				break;

			++tszSourceCurrent;
		}

		return const_cast<T*>(tszSourceCurrent);
	}

	/// scan the string for characters in specified character set, alternative of 'strpbrk()', 'wcspbrk()'
	/// @remarks: search doesn't include the terminating null character
	/// @returns: pointer to the first occurence of a character in @a`tszSource` that belongs to @a`tszSet` set of characters on success, null otherwise
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringBreak(const T* tszSource, const T* tszSet)
	{
		while (*tszSource != '\0')
		{
			// @test: we may get rid of this loop by making bitset of the set characters, reduces time complexity from O(mn) to O(m+n) | only for ansi set
			const T* tszSetCurrent = tszSet;
			while (*tszSetCurrent != '\0')
			{
				if (*tszSource == *tszSetCurrent++)
					return const_cast<T*>(tszSource);
			}

			++tszSource;
		}

		return nullptr;
	}

	/// break string into sequence of tokens, alternative of 'strtok_r()'
	/// @param[in] tszSource string containing tokens
	/// @param[in] tszDelimiters set of delimiter characters
	/// @param[out] ptszLast pointer to the beginning of the next token search
	/// @remarks: finds the next token in @a`tszSource`, each call modifies @a`tszSource` by substituting a terminating null character for the first delimiter of @a`tszDelimiters` that occurs after the returned token
	/// @returns: pointer to the next token found in @a`tszSource` on success, null otherwise
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringToken(T* tszSource, const T* tszDelimiters, T** ptszLast)
	{
		if (tszSource == nullptr)
			tszSource = *ptszLast;

		tszSource = StringSpan(tszSource, tszDelimiters);
		if (*tszSource == '\0')
		{
			*ptszLast = tszSource;
			return nullptr;
		}

		// @test: unit tests
		T* tszToken = tszSource;
		tszSource = StringBreak(tszSource, tszDelimiters);
		if (tszSource == nullptr)
			*ptszLast = StringChar(tszToken, '\0');
		else
		{
			*tszSource++ = '\0';
			*ptszLast = tszSource;
		}
		
		return tszToken;
	}

	/// copy a one string to another, alternative of 'stpcpy()', 'wcpcpy()'
	/// @remarks: copies @a`tszSource`, including the terminating null character, to the @a`tszDestination`. the behavior is undefined if the source and destination strings overlap
	/// @returns: pointer to the terminating null character in @a`tszDestination`
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringCopy(T* tszDestination, const T* tszSource)
	{
		while (*tszSource != '\0')
			*tszDestination++ = *tszSource++;

		*tszDestination = '\0';
		return tszDestination;
	}

	/// copy a one string to another up to the specified count of characters, alternative of 'stpncpy()', 'wcpncpy()'
	/// @remarks: copies the initial @a`nCount` characters of @a`tszSource` to @a`tszDestination`. if @a`nCount` is less than or equal to the length of @a`tszSource`, the terminating null character isn't appended to the copied string. if @a`nCount` is greater than the length of @a`tszSource`, the @a`tszDestination` is padded with null characters up to @a`nCount`. the behavior is undefined if the source and destination strings overlap
	/// @returns: pointer to the @a`tszDestination` advanced by @a`nCount`
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringCopyN(T* tszDestination, const T* tszSource, std::size_t nCount)
	{
		while (nCount-- != 0U)
			*tszDestination++ = (*tszSource != '\0' ? *tszSource++ : '\0');

		return tszDestination;
	}

	/// append a one string to another, alternative of 'stpcat()', 'wcpcat()'
	/// @remarks: appends @a`tszSource` to @a`tszDestination` and terminates the resulting string with a null character. the initial character of @a`tszSource` overwrites the terminating null character of @a`tszDestination`. the behavior is undefined if the source and destination strings overlap
	/// @returns: pointer to the terminating null character in @a`tszDestination`
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringCat(T* tszDestination, const T* tszSource)
	{
		while (*tszDestination != '\0')
			++tszDestination;

		while (*tszSource != '\0')
			*tszDestination++ = *tszSource++;

		*tszDestination = '\0';
		return tszDestination;
	}

	/// append a one string to another up to the specified count of characters, alternative of 'stpncat()', 'wcpncat()'
	/// @remarks: appends at most the first @a`nCount` characters of @a`tszSource` to @a`tszDestination`. the initial character of @a`tszSource` overwrites the terminating null character of @a`tszDestination`. if the terminating null character appears in @a`tszSource` before @a`nCount` characters are appended, function appends all characters from @a`tszSource`, up to the null character. if @a`nCount` is greater than the length of @a`tszSource`, the length of @a`tszSource` is used in place of @a`nCount`. in all cases, the resulting string is terminated with a null character. if copying takes place between strings that overlap, the behavior is undefined
	/// @returns: pointer to the terminating null character in @a`tszDestination`
	template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
	constexpr T* StringCatN(T* tszDestination, const T* tszSource, std::size_t nCount)
	{
		while (*tszDestination != '\0')
			++tszDestination;

		while (*tszSource != '\0' && nCount-- != 0U)
			*tszDestination++ = *tszSource++;

		*tszDestination = '\0';
		return tszDestination;
	}

	#ifdef Q_CRT_STRING_NATURAL
	#include "string/natural.inl"
	#endif
	#pragma endregion

	/*
	 * @section: string conversion
	 * - these functions can write past the end of a buffer that is too small.
	 *   to prevent buffer overruns, ensure that buffer is large enough to hold the converted data and the trailing null-character.
	 *   misuse of these functions can cause serious security issues in your code
	 * - valid only for default C locale
	 */
	#pragma region crt_string_convert
	#ifdef Q_CRT_STRING_CONVERT
	#include "string/convert.inl"
	#endif
	#pragma endregion

	/*
	 * @section: string encoding
	 * - valid only for default C locale
	 */
	#pragma region crt_string_encode
	#ifdef Q_CRT_STRING_ENCODE
	#include "string/encode.inl"
	#endif
	#pragma endregion

