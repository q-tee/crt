#ifndef Q_CRT_STRING_NATURAL_IMPLEMENTATION
#define Q_CRT_STRING_NATURAL_IMPLEMENTATION
/// compare two strings using "natural order" algorithm, alternative of 'strnatcmp()'
/// @remarks: compares @a`tszLeft` and @a`tszRight` strings and return a value that indicates their relationship. if a number appears in both strings and is different, the return value indicates the string containing the larger decimal value. fractional values are treated as integers
/// @returns: <0 - if @a`tszLeft` less than @a`tszRight`, 0 - if @a`tszLeft` is identical to @a`tszRight`, >0 - if @a`tszLeft` greater than @a`tszRight`
template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
constexpr int StringNaturalCompare(const T* tszLeft, const T* tszRight)
{
	int nLeft, nRight;
	while (*tszLeft != '\0' || *tszRight != '\0')
	{
		if (IsDigit(*tszLeft) && IsDigit(*tszRight))
		{
			// convert first digit to number
			nLeft = *tszLeft++ - '0';
			// append continuous digits to number
			while (IsDigit(*tszLeft))
				nLeft = nLeft * 10 + (*tszLeft++ - '0');

			// convert first digit to number
			nRight = *tszRight++ - '0';
			// append continuous digits to number
			while (IsDigit(*tszRight))
				nRight = nRight * 10 + (*tszRight++ - '0');
		}
		else
		{
			nLeft = static_cast<std::uint8_t>(*tszLeft++);
			nRight = static_cast<std::uint8_t>(*tszRight++);
		}

		// check either for larger number or regular equality
		// @test: we can do if (nLeft != nRight) return nLeft - nRight; | check the compiled code for both and select the better
		if (nLeft < nRight)
			return -1;
		if (nLeft > nRight)
			return 1;
	}

	return 0;
}

/// case-insensitive compare two strings using "natural order" algorithm, alternative of 'strnatcasecmp()'
/// @remarks: compares @a`tszLeft` and @a`tszRight` strings and return a value that indicates their relationship, performs conversion of each character to lowercase before comparison. if a number appears in both strings and is different, the return value indicates the string containing the larger decimal value. fractional values are treated as integers
/// @returns: <0 - if @a`tszLeft` less than @a`tszRight`, 0 - if @a`tszLeft` is identical to @a`tszRight`, >0 - if @a`tszLeft` greater than @a`tszRight`
template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
constexpr int StringNaturalCompareI(const T* tszLeft, const T* tszRight)
{
	int nLeft, nRight;
	while (*tszLeft != '\0' || *tszRight != '\0')
	{
		if (IsDigit(*tszLeft) && IsDigit(*tszRight))
		{
			// convert first digit to number
			nLeft = *tszLeft++ - '0';
			// append continuous digits to number
			while (IsDigit(*tszLeft))
				nLeft = nLeft * 10 + (*tszLeft++ - '0');

			// convert first digit to number
			nRight = *tszRight++ - '0';
			// append continuous digits to number
			while (IsDigit(*tszRight))
				nRight = nRight * 10 + (*tszRight++ - '0');
		}
		else
		{
			nLeft = CharToLower(*tszLeft++);
			nRight = CharToLower(*tszRight++);
		}

		// check either for larger number or regular equality
		// @test: we can do if (nLeft != nRight) return nLeft - nRight;
		if (nLeft < nRight)
			return -1;
		if (nLeft > nRight)
			return 1;
	}

	return 0;
}
#endif
