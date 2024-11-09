#ifndef Q_CRT_STRING_ENCODE_IMPLEMENTATION
#define Q_CRT_STRING_ENCODE_IMPLEMENTATION
// @todo: missing unit tests for those, atm it just naive implementation, but lacks error handling of some cases

/// @returns: UTF-8 characters count of the UTF-16 string
inline std::size_t StringLengthMultiByte(const wchar_t* wszSourceBegin, const wchar_t* wszSourceEnd = nullptr)
{
	std::size_t nOctetCount = 0U;

	while (*wszSourceBegin != L'\0' && (wszSourceEnd == nullptr || wszSourceBegin < wszSourceEnd))
	{
		if (const wchar_t wSourceChar = *wszSourceBegin++; wSourceChar < 0x80)
			// 0XXXXXXX
			nOctetCount += 1U;
		else if (wSourceChar < 0x800)
			// 110XXXXX 10XXXXXX
			nOctetCount += 2U;
		else if (wSourceChar < 0xFFFF)
			// 1110XXXX 10XXXXXX 10XXXXXX
			nOctetCount += 3U;
	}

	return nOctetCount;
}

/// alternative of '_mbslen()'
/// @returns: UTF-16 characters count of UTF-8 string
inline std::size_t StringLengthUnicode(const char* szSourceBegin, const char* szSourceEnd = nullptr)
{
	std::size_t nCharCount = 0U;

	while (*szSourceBegin != '\0' && (szSourceEnd == nullptr || szSourceBegin < szSourceEnd))
	{
		if ((*szSourceBegin & 0x80) == 0 || (*szSourceBegin & 0xE0) == 0xC0 || (*szSourceBegin & 0xF0) == 0xE0)
			++nCharCount;
		else if ((*szSourceBegin & 0xF8) == 0xF0)
			nCharCount += 2U;

		++szSourceBegin;
	}

	return nCharCount;
}

/// convert UTF-8 string to UTF-16 string, alternative of 'MultiByteToWideChar()', 'mbstowcs()'
/// @param[in] nDestinationSize size of the destination buffer including the terminating null, in characters
/// @remarks: locale-independent
/// @returns: length of the converted UTF-16 string
inline std::ptrdiff_t StringMultiByteToUnicode(wchar_t* wszDestination, std::size_t nDestinationSize, const char* szSourceBegin, const char* szSourceEnd = nullptr)
{
	wchar_t* wszDestinationCurrent = wszDestination;
	const std::uint8_t* pSourceCurrent = reinterpret_cast<const std::uint8_t*>(szSourceBegin);

	// reserve space for null terminator
	--nDestinationSize;

	std::uint32_t nCodePoint;
	while (*pSourceCurrent != '\0' && (szSourceEnd == nullptr || pSourceCurrent < reinterpret_cast<const std::uint8_t*>(szSourceEnd)) && nDestinationSize > 0U)
	{
		const wchar_t* wszDestinationPrevious = wszDestinationCurrent;

		// check for continuation value
		if ((*pSourceCurrent & 0x80) == 0U)
			*wszDestinationCurrent++ = *pSourceCurrent++;
		else if ((*pSourceCurrent & 0xE0) == 0xC0)
		{
			if (*pSourceCurrent < 0xC2)
				continue;

			nCodePoint = (*pSourceCurrent++ & 0x1F) << 6U;
			if ((*pSourceCurrent & 0xC0) != 0x80)
				continue;

			*wszDestinationCurrent++ = static_cast<wchar_t>(nCodePoint + (*pSourceCurrent++ & 0x3F));
		}
		else if ((*pSourceCurrent & 0xF0) == 0xE0)
		{
			if ((*pSourceCurrent == 0xE0 && (pSourceCurrent[1] < 0xA0 || pSourceCurrent[1] > 0xBF)) ||
				(*pSourceCurrent == 0xED && pSourceCurrent[1] > 0x9F))
				continue;

			nCodePoint = (*pSourceCurrent++ & 0x0F) << 12U;
			if ((*pSourceCurrent & 0xC0) != 0x80)
				continue;

			nCodePoint += (*pSourceCurrent++ & 0x3F) << 6U;
			if ((*pSourceCurrent & 0xC0) != 0x80)
				continue;

			*wszDestinationCurrent++ = static_cast<wchar_t>(nCodePoint + (*pSourceCurrent++ & 0x3F));
		}
		else if ((*pSourceCurrent & 0xF8) == 0xF0)
		{
			if (*pSourceCurrent > 0xF4 ||
				(*pSourceCurrent == 0xF0 && (pSourceCurrent[1] < 0x90 || pSourceCurrent[1] > 0xBF)) ||
				(*pSourceCurrent == 0xF4 && pSourceCurrent[1] > 0x8F))
				continue;

			nCodePoint = (*pSourceCurrent++ & 0x07) << 18U;
			if ((*pSourceCurrent & 0xC0) != 0x80)
				continue;

			nCodePoint += (*pSourceCurrent++ & 0x3F) << 12U;
			if ((*pSourceCurrent & 0xC0) != 0x80)
				continue;

			nCodePoint += (*pSourceCurrent++ & 0x3F) << 6U;
			if ((*pSourceCurrent & 0xC0) != 0x80)
				continue;

			nCodePoint += (*pSourceCurrent++ & 0x3F);
			// utf-8 encodings of values used in surrogate pairs are invalid
			if ((nCodePoint & 0xFFFFF800) == 0xD800 || nCodePoint < 0x10000)
				continue;

			if (nDestinationSize >= 2U)
			{
				nCodePoint -= 0x10000;
				*wszDestinationCurrent++ = 0xD800 | (0x3FF & (nCodePoint >> 10U));
				*wszDestinationCurrent++ = 0xDC00 | (0x3FF & nCodePoint);
			}
		}

		nDestinationSize -= wszDestinationCurrent - wszDestinationPrevious;
	}

	*wszDestinationCurrent = L'\0';
	return wszDestinationCurrent - wszDestination;
}

/// convert UTF-16 string to UTF-8 string, alternative of 'WideToMultiByteChar()', 'wcstombs()'
/// @param[in] nDestinationSize size of the destination buffer including the terminating null, in characters
/// @remarks: locale-independent, handles decoding error by skipping forward
/// @returns: length of the converted multibyte UTF-8 string
template <typename DestinationChar_t, typename SourceChar_t> requires (std::is_same_v<char, DestinationChar_t> || std::is_same_v<char8_t, DestinationChar_t>) && (std::is_same_v<wchar_t, SourceChar_t> || std::is_same_v<char16_t, SourceChar_t> || std::is_same_v<char32_t, SourceChar_t>)
inline std::ptrdiff_t StringUnicodeToMultiByte(char* tszDestination, std::size_t nDestinationSize, const SourceChar_t* tszSourceBegin, const SourceChar_t* tszSourceEnd = nullptr)
{
	DestinationChar_t* tszDestinationCurrent = tszDestination;

	// reserve space for null terminator
	--nDestinationSize;

	while (*tszSourceBegin != '\0' && (tszSourceEnd == nullptr || tszSourceBegin < tszSourceEnd) && nDestinationSize > 0U)
	{
		const DestinationChar_t* tszDestinationPrevious = tszDestinationCurrent;

		// check if the character is ASCII
		if (const SourceChar_t wSourceChar = *tszSourceBegin++; wSourceChar < 0x80)
			*tszDestinationCurrent++ = static_cast<char>(wSourceChar);
		// check if the character is in UTF-16 range
		else if (wSourceChar < 0x800 && nDestinationSize >= 2U)
		{
			*tszDestinationCurrent++ = static_cast<char>(0xC0 | (wSourceChar >> 6U));
			*tszDestinationCurrent++ = static_cast<char>(0x80 | (wSourceChar & 0x3F));
		}
		// check if the character is in UTF-16 range, excluding surrogate pairs
		else if ((wSourceChar <= 0xD7FF || (wSourceChar >= 0xE000 && wSourceChar <= 0xFFFF)) && nDestinationSize >= 3U)
		{
			*tszDestinationCurrent++ = static_cast<char>(0xE0 | (wSourceChar >> 12U));
			*tszDestinationCurrent++ = static_cast<char>(0x80 | ((wSourceChar >> 6U) & 0x3F));
			*tszDestinationCurrent++ = static_cast<char>(0x80 | (wSourceChar & 0x3F));
		}
		// check if the character is in UTF-32 range or UTF-16 surrogate pair
		else if (((sizeof(SourceChar_t) == 2U && wSourceChar >= 0xD800 && wSourceChar < 0xDC00) || (sizeof(SourceChar_t) == 4U && wSourceChar <= 0x10FFFF)) && nDestinationSize >= 4U)
		{
			const SourceChar_t wLowSurrogate = *tszSourceBegin++;
			const std::uint32_t uCodePoint = (((wSourceChar & 0x3FFF) << 10) | (wLowSurrogate & 0x3FFF)) + 0x10000;

			*tszDestinationCurrent++ = static_cast<char>(0xF0 | (uCodePoint >> 18U));
			*tszDestinationCurrent++ = static_cast<char>(0x80 | ((uCodePoint >> 12U) & 0x3F));
			*tszDestinationCurrent++ = static_cast<char>(0x80 | ((uCodePoint >> 6U) & 0x3F));
			*tszDestinationCurrent++ = static_cast<char>(0x80 | (uCodePoint & 0x3F));
		}

		nDestinationSize -= tszDestinationCurrent - tszDestinationPrevious;
	}

	*tszDestinationCurrent = '\0';
	return tszDestinationCurrent - tszDestination;
}
#endif
