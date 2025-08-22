#ifndef Q_CRT_STRING_CONVERT_IMPLEMENTATION
#define Q_CRT_STRING_CONVERT_IMPLEMENTATION
namespace DETAIL
{
	Q_INLINE inline std::uint64_t Multiply64To128(std::uint64_t ullMultiplicand, std::uint64_t ullMultiplier, std::uint64_t* pullProductHigh)
	{
	#if defined(Q_COMPILER_MSC) && Q_ARCH_BIT == 64
	#if defined(Q_ARCH_ARM64)
		// @test: haven't tested that
		*pullProductHigh = ::__umulh(ullMultiplicand, ullMultiplier);
		return ullLow * ullMultiplier;
	#else
		return ::_umul128(ullMultiplier, ullMultiplicand, pullProductHigh);
	#endif
	#elif (defined(Q_COMPILER_CLANG) || defined(Q_COMPILER_GCC)) && Q_ARCH_BIT == 64
		const __uint128_t uResult = (static_cast<__uint128_t>(this->ullMultiplicand) * ullMultiplier;
		*pullProductHigh = static_cast<std::uint64_t>(uResult >> 64ULL);
		return static_cast<std::uint64_t>(uResult);
	#else
		const std::uint64_t ullLow = (ullMultiplicand & 0xFFFFFFFF) * ullMultiplier;
		std::uint64_t ullHigh = (ullMultiplicand >> 32ULL) * ullMultiplier;
		ullHigh += ullLow >> 32ULL;
		*pullProductHigh = ullHigh >> 32ULL;
		return (ullHigh << 32ULL) | (ullLow & 0xFFFFFFFF);
	#endif
	}
}

/* @section: [internal] constants */
// constant of largest valid base
inline constexpr int kMaxNumberBase = 36;
// every possible character to represent the number with the largest valid base
inline constexpr const char* arrAlphanumericLUT = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// lookup table for decimal integer in [00 .. 99] range to string conversion
inline constexpr char arrDigitPairLUT[] =
"0001020304050607080910111213141516171819"
"2021222324252627282930313233343536373839"
"4041424344454647484950515253545556575859"
"6061626364656667686970717273747576777879"
"8081828384858687888990919293949596979899";
// lookup table for hexadecimal integer in [00 .. FF] range to string conversion
inline constexpr char arrDigitPairHexLUT[] =
"000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F"
"202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F"
"404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F"
"606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F"
"808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9F"
"A0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
"C0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
"E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";

/*
 * - according to IEEE-754:
 *
 * 1. binary32 floating point
 *           +/-  exponent (+127)            mantissa (fractional part)
 *  *-------*---*-----------------*-----------------------------------------------*
 *  | width | 1 |        8        |                      23                       |
 *  *-------*---*-----------------*-----------------------------------------------*
 *  | mask  | 0 | 0 1 1 1 1 1 0 0 | 0 0 0 1 0 0 0 1 1 1 0 1 0 0 0 1 0 1 0 0 1 1 1 |
 *  *-------*---*-----------------*-----------------------------------------------*
 *            ^   ^             ^   ^                                           ^
 *           31   30           23   22                                          0
 *
 * 2. binary64 floating point
 *            +/-    exponent (+1023)                 mantissa (fractional part)
 *   *-------*---*-----------------------*---------------------------------------------------*
 *   | width | 1 |          11           |                        52                         |
 *   *-------*---*-----------------------*---------------------------------------------------*
 *   | mask  | 0 | 0 1 1 1 1 1 0 0 0 0 0 | 0 0 0 1 0 0 0 1 1 1 0 1 0 0 0 1 0 1 0 0 1 1 1 ... |
 *   *-------*---*-----------------------*---------------------------------------------------*
 *             ^   ^                   ^   ^                                               ^
 *            63   62                 52   51                                              0
 */
template <typename T> requires (std::is_floating_point_v<T>)
struct RealTraits_t
{
	static_assert(std::numeric_limits<T>::is_iec559);

	// bit-equivalent integer type for given floating point type
	using BitEquivalent_t = std::conditional_t<std::is_same_v<T, double> || std::is_same_v<T, long double>, std::uint64_t, std::uint32_t>;

	static constexpr std::uint32_t kMantissaWidth = std::numeric_limits<T>::digits - 1U;
	static constexpr std::uint32_t kExponentWidth = (std::is_same_v<T, double> ? 11U : 8U);
	static constexpr std::uint32_t kSignWidth = 1U;
	static constexpr std::uint32_t kTotalWidth = kMantissaWidth + kExponentWidth + kSignWidth;

	static constexpr int kExponentBias = (1U << (kExponentWidth - 1U)) - 1U;
	static constexpr int kExponentMin = (std::is_same_v<T, double> ? -1022 : -126);
	static constexpr int kExponentMax = (std::is_same_v<T, double> ? 1023 : 127);
	static constexpr int kExponentDecimalMin = (std::is_same_v<T, double> ? -324 : -45);
	static constexpr int kExponentDecimalMax = (std::is_same_v<T, double> ? 308 : 38);
	static constexpr int kExponentDenormalMin = (kExponentMin - static_cast<int>(kMantissaWidth));

	static constexpr BitEquivalent_t kMantissaMask = (BitEquivalent_t(1U) << kMantissaWidth) - 1U;
	static constexpr BitEquivalent_t kSignMask = (BitEquivalent_t(1U) << (kExponentWidth + kMantissaWidth));
	static constexpr BitEquivalent_t kExponentMask = ~(kSignMask | kMantissaMask);
	static constexpr BitEquivalent_t kQuietNanMask = (BitEquivalent_t(1U) << (kMantissaWidth - 1U));
};

template <typename T, std::size_t uBase> requires (std::is_integral_v<T> && uBase > 0U && uBase <= kMaxNumberBase)
struct IntegerToString_t
{
	/// @returns: maximum count of characters needed for integer-to-string conversion, including terminating null and negative sign where appropriate
	static consteval std::size_t MaxCount()
	{
		std::size_t nDigitsCount = 0U;

		constexpr std::uint64_t ullNegativeMax = (std::is_unsigned_v<T> ? (std::numeric_limits<T>::max)() : (static_cast<std::uint64_t>((std::numeric_limits<T>::max)()) + 1ULL));
		for (std::uint64_t nShift = ullNegativeMax; nShift != 0ULL; nShift /= uBase)
			++nDigitsCount;

		return (nDigitsCount + (std::is_signed_v<T> && uBase == 10U ? 1U : 0U) + 1U);
	}
};

struct BigInteger_t
{
	Q_INLINE BigInteger_t& ShiftLeft(const std::uint32_t nCount)
	{
		if (nCount >= 128U)
		{
			this->ullHigh = 0ULL;
			this->ullLow = 0ULL;
		}
		else if (nCount >= 64U)
		{
			this->ullHigh = this->ullLow << (nCount - 64U);
			this->ullLow = 0ULL;
		}
		else
		{
		#if defined(Q_COMPILER_MSC) && Q_ARCH_BIT == 64
			this->ullHigh = ::__shiftleft128(this->ullLow, this->ullHigh, nCount);
		#else
			this->ullHigh = (this->ullLow >> (64U - nCount)) | (this->ullHigh << nCount);
		#endif
			this->ullLow = this->ullLow << nCount;
		}

		return *this;
	}

	Q_INLINE BigInteger_t& ShiftRight(const std::uint32_t nCount)
	{
		if (nCount >= 128U)
		{
			this->ullLow = 0ULL;
			this->ullHigh = 0ULL;
		}
		else if (nCount >= 64U)
		{
			this->ullLow = this->ullHigh >> (nCount - 64U);
			this->ullHigh = 0ULL;
		}
		else
		{
		#if defined(Q_COMPILER_MSC) && Q_ARCH_BIT == 64
			this->ullLow = ::__shiftright128(this->ullLow, this->ullHigh, nCount);
		#else
			this->ullLow = (this->ullHigh << (64U - nCount)) | (this->ullLow >> nCount);
		#endif
			this->ullHigh >>= nCount;
		}

		return *this;
	}

	Q_INLINE BigInteger_t& Add(const BigInteger_t& other)
	{
	#if defined(Q_COMPILER_CLANG) && Q_ARCH_BIT == 64
		std::uint64_t ullCarryLow;
		this->ullLow = ::__builtin_addcll(this->ullLow, other.ullLow, 0ULL, reinterpret_cast<unsigned long long*>(&ullCarryLow));
		this->ullHigh += other.ullHigh + ullCarryLow;
	#elif defined(Q_COMPILER_GCC) && Q_ARCH_BIT == 64
		// @test: haven't tested that
		this->ullHigh += ::__builtin_add_overflow(this->ullLow, other.ullLow, &this->ullLow);
	#elif defined(Q_COMPILER_MSC) && Q_ARCH_BIT == 64
		const unsigned char uCarryLow = ::_addcarry_u64(0U, this->ullLow, other.ullLow, &this->ullLow);
		this->ullHigh += other.ullHigh + uCarryLow;
	#else
		const std::uint64_t ullPreviousLow = this->ullLow;
		this->ullLow += other.ullLow;
		this->ullHigh += other.ullHigh;
		if (this->ullLow < ullPreviousLow)
			++this->ullHigh;
	#endif
		return *this;
	}

	Q_INLINE BigInteger_t& Multiply(const std::uint64_t ullMultiplier)
	{
	#if (defined(Q_COMPILER_CLANG) || defined(Q_COMPILER_GCC)) && Q_ARCH_BIT == 64
		const __uint128_t uResult = ((static_cast<__uint128_t>(this->ullHigh) << 64ULL) | this->ullLow) * ullMultiplier;
		this->ullLow = static_cast<std::uint64_t>(uResult);
		this->ullHigh = static_cast<std::uint64_t>(uResult >> 64ULL);
	#else
		std::uint64_t ullLowCarry;
		this->ullLow = DETAIL::Multiply64To128(this->ullLow, ullMultiplier, &ullLowCarry);
		this->ullHigh = this->ullHigh * ullMultiplier + ullLowCarry;
	#endif
		return *this;
	}

	Q_INLINE BigInteger_t& Divide10()
	{
		// 128-bit right shift by 1
		std::uint64_t ullMiddleLow = (this->ullLow >> 1ULL) | (this->ullHigh << 63ULL);
		std::uint64_t ullMiddleHigh = this->ullHigh >> 1ULL;

		// calculate intermediate value with carry
		std::uint64_t ullSum = ullMiddleLow;
		ullSum += ullMiddleHigh;
		if (ullSum < ullMiddleLow) // handle carry from addition
			ullSum++;

		// calculate right shift by 66
		std::uint64_t ullProductHigh;
		DETAIL::Multiply64To128(ullSum, 0xCCCCCCCCCCCCCCCD, &ullProductHigh);
		ullProductHigh >>= 2ULL;

		// calculate remainder modulo 5
		ullSum -= ullProductHigh * 5ULL;

		// subtract remainder from shifted value
		const std::uint64_t ullMiddleLowOld = ullMiddleLow;
		ullMiddleLow -= ullSum;
		if (ullMiddleLow > ullMiddleLowOld) // handle borrow
			ullMiddleHigh--;

		// multiply by fixed point constant
		this->ullLow = DETAIL::Multiply64To128(ullMiddleLow, 0xCCCCCCCCCCCCCCCD, &ullProductHigh);
		ullProductHigh += ullMiddleLow * 0xCCCCCCCCCCCCCCCC;
		this->ullHigh = ullProductHigh + ullMiddleHigh * 0xCCCCCCCCCCCCCCCD;
		return *this;
	}

	std::uint64_t ullLow;
	std::uint64_t ullHigh;
};

template <typename T>
concept HasTmZone_t = requires { T::tm_zone; };
template <typename T>
concept HasTmGmtOff_t = requires { T::tm_gmtoff; };

/// reverse every character in the string, alternative of '_strrev()', '_wcsrev()'
/// @returns: pointer to the @a`tszSource`
template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
constexpr T* StringReverse(T* tszSource)
{
	T* tszSourceCurrent = tszSource, *tszSourceEnd = tszSource + StringLength(tszSource);
	while (tszSourceCurrent < tszSourceEnd)
	{
		// @test: is xor would be really faster than temporary store?
		T tchCurrent = *tszSourceCurrent++, tchEnd = *--tszSourceEnd;
		tchCurrent ^= tchEnd;
		tchEnd ^= tchCurrent;
		*tszSourceCurrent = tchCurrent ^ tchEnd;
		*tszSourceEnd = tchEnd;
	}

	return tszSource;
}

/// convert every character in the string to uppercase, alternative of '_strupr()', @todo: '_wcsupr()'
/// @returns: pointer to the @a`tszDestination`
template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
constexpr T* StringToUpper(T* tszDestination)
{
	T* tszDestinationOut = tszDestination;

	while (*tszDestinationOut != '\0')
		*tszDestinationOut = static_cast<T>(CharToUpper(*tszDestinationOut++));

	return tszDestination;
}

/// convert every character in the string to lowercase, alternative of '_strlwr()', '_wcslwr()'
/// @returns: pointer to the @a`tszDestination`
template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
constexpr T* StringToLower(T* tszDestination)
{
	T* tszDestinationOut = tszDestination;

	while (*tszDestinationOut != '\0')
		*tszDestinationOut = static_cast<T>(CharToLower(*tszDestinationOut++));

	return tszDestination;
}

// @todo: rework sprintf like, with specific format flags right here
/// convert an integer to a string, alternative of 'to_string()', 'to_chars()', '_itoa()', '_ltoa()', '_ultoa()', '_i64toa()', '_ui64toa()'
/// @param[in] nDestinationSize size of the destination buffer including the terminating null, in characters
/// @param[in] iBase numeric base in range [2 .. 36] to use to represent the number
/// @remarks: converts the digits of the given value argument to a null-terminated character string and store the result in @a`tszDestination` buffer. all writes begin from the end of the buffer and may overrun past the bounds of a buffer that is too small, to ensure that buffer is large enough, use @code CRT::IntegerToString_t<T, iBase>::MaxCount() @endcode
/// @returns: pointer to the beginning of the converted integer in the buffer
template <typename V, typename T> requires (std::is_integral_v<V> && (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>))
T* IntegerToString(const V value, T* tszDestination, const std::size_t nDestinationSize, int iBase = 10) // @todo: couldn't it be constexpr?
{
	if (iBase < 0 || iBase == 1 || iBase > kMaxNumberBase)
	{
		Q_ASSERT(false); // given number base is out of range
		return tszDestination;
	}

	const bool bIsPositive = (value >= 0);
	std::make_unsigned_t<V> uValue = (bIsPositive ? static_cast<std::make_unsigned_t<V>>(value) : static_cast<std::make_unsigned_t<V>>(0 - value)); // @test: how it actually compiles, can avoid branch at compile time

	T* tszDestinationEnd = tszDestination + nDestinationSize;
	*--tszDestinationEnd = '\0';

	if (uValue == 0U)
		*--tszDestinationEnd = '0';
	else if (iBase == 10)
	{
		// for decimal base write two digits in a group
		while (uValue >= 10U)
		{
			tszDestinationEnd -= 2;

			const char* szTwoDigits = &arrDigitPairLUT[(uValue % 100U) * 2U];
			tszDestinationEnd[0] = szTwoDigits[0];
			tszDestinationEnd[1] = szTwoDigits[1];

			uValue /= 100U;
		}

		if (uValue != 0U)
			*--tszDestinationEnd = arrAlphanumericLUT[uValue];

		// insert negative sign, only decimal base can have it
		if (!bIsPositive)
			*--tszDestinationEnd = '-';
	}
	else if (iBase == 16)
	{
		// for hexadecimal base write two digits in a group
		while (uValue > 0xFF)
		{
			tszDestinationEnd -= 2;

			const char* szTwoDigits = &arrDigitPairHexLUT[(uValue & 0xFF) * 2U];
			tszDestinationEnd[0] = szTwoDigits[0];
			tszDestinationEnd[1] = szTwoDigits[1];

			uValue >>= 8U;
		}

		if (uValue < 0x10)
			*--tszDestinationEnd = arrAlphanumericLUT[uValue];
		else
		{
			tszDestinationEnd -= 2;

			const char* szTwoDigits = &arrDigitPairHexLUT[uValue * 2U];
			tszDestinationEnd[0] = szTwoDigits[0];
			tszDestinationEnd[1] = szTwoDigits[1];
		}
	}
	else
	{
		// for other bases perform write by single digit
		while (uValue != 0U)
		{
			*--tszDestinationEnd = arrAlphanumericLUT[uValue % iBase];
			uValue /= iBase;
		}
	}

	return tszDestinationEnd;
}

// @todo: rework sprintf like, with specific format flags right here
/// convert a floating-point number to a string, alternative of 'to_string()', 'to_chars()'
/// @param[in] nDestinationSize size of the destination buffer including the terminating null, in characters
/// @param[in] iPrecision number of fractional digits to print
/// @remarks: converts the digits of the given floating-point @a`value` to a null-terminated string and store the result in @a`tszDestination` buffer. can write either from begin of the buffer or from the end, or past the bounds of a buffer that is too small, to ensure that buffer is large enough, use 310 + @a`iPrecision`
/// @returns: pointer to the beginning of the converted floating-point number in the buffer
template <typename V, typename T> requires (std::is_floating_point_v<V> && (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>))
T* RealToString(const V value, T* tszDestination, const std::size_t nDestinationSize, int iPrecision = 6)
{
	using UIntType_t = typename RealTraits_t<V>::BitEquivalent_t;
	auto uBits = std::bit_cast<UIntType_t>(value);
	auto uExponent = static_cast<std::uint16_t>((uBits & RealTraits_t<V>::kExponentMask) >> RealTraits_t<V>::kMantissaWidth);
	auto uMantissa = uBits & RealTraits_t<V>::kMantissaMask;
	const bool bIsNegative = (uBits & RealTraits_t<V>::kSignMask) != 0U;

	T* tszDestinationEnd = tszDestination;

	// check if the all bits of the exponent is set
	if (uExponent == (1U << RealTraits_t<V>::kExponentWidth) - 1U)
	{
		if (bIsNegative)
			*tszDestinationEnd++ = '-';

		// check if any of the mantissa bits is set, indicating that value is NaN
		if (uMantissa != 0U)
		{
			// a double argument representing a NaN is converted in one of the styles [-]nan or [-]nan(n-wchar-sequence) - which style, and the meaning of any n-wchar-sequence, is implementation-defined (C standard 7.21.6.1)
			*tszDestinationEnd++ = 'n';
			*tszDestinationEnd++ = 'a';
			*tszDestinationEnd++ = 'n';
			*tszDestinationEnd = '\0';
		}
		else
		{
			// a double argument representing an infinity is converted in one of the styles [-]inf or [-]infinity - which style is implementation-defined (C standard 7.21.6.1)
			*tszDestinationEnd++ = 'i';
			*tszDestinationEnd++ = 'n';
			*tszDestinationEnd++ = 'f';
			*tszDestinationEnd = '\0';
		}

		return tszDestination;
	}

	std::int16_t iExponentUnbiased;
	if (uExponent == 0U)
	{
		// check if the value is zero
		if (uMantissa == 0U)
		{
			if (bIsNegative)
				*tszDestinationEnd++ = '-';

			*tszDestinationEnd++ = '0';

			if (iPrecision > 0)
			{
				*tszDestinationEnd++ = '.';
				do
				{
					*tszDestinationEnd++ = '0';
				} while (--iPrecision > 0);
			}

			*tszDestinationEnd = '\0';
			return tszDestination;
		}

		// set minimum exponent for denormalized numbers
		iExponentUnbiased = RealTraits_t<V>::kExponentDenormalMin;
	}
	else
	{
		// calculate unbiased exponent
		iExponentUnbiased = uExponent - RealTraits_t<V>::kExponentBias;
		// add implicit bit for normalized numbers
		uMantissa |= static_cast<UIntType_t>(1U) << RealTraits_t<V>::kMantissaWidth;
	}

	// check if the number has large value
	const int iExponentShift = iExponentUnbiased - RealTraits_t<V>::kMantissaWidth;
	if (iExponentShift >= 0)
	{
		tszDestinationEnd += nDestinationSize;
		*--tszDestinationEnd = '\0';

		if (iPrecision > 0)
		{
			do
			{
				*--tszDestinationEnd = '0';
			} while (--iPrecision > 0);

			*--tszDestinationEnd = '.';
		}

		// @todo: fast path for 64/128 bits? should also fully cover float range (39 digits)
		const int nTotalBits = iExponentUnbiased + 1;
		const int nTotalChunks = (nTotalBits + 31) / 32;

		const std::uint32_t nChunkIndex = iExponentShift / 32U;
		const std::uint32_t uBitShift = iExponentShift % 32U;
		std::uint64_t ullIntegerPart = static_cast<std::uint64_t>(uMantissa) << uBitShift;

		std::uint32_t arrChunks[32] = { };
		arrChunks[nChunkIndex] = static_cast<std::uint32_t>(ullIntegerPart); // low chunk
		ullIntegerPart >>= 32ULL;
		arrChunks[nChunkIndex + 1U] = static_cast<std::uint32_t>(ullIntegerPart); // middle chunk
		// keep it conditional to not write past the end of chunks buffer
		if (const std::uint32_t uHighChunk = static_cast<std::uint32_t>(uMantissa >> (64U - uBitShift)); uHighChunk != 0U)
			arrChunks[nChunkIndex + 2U] = uHighChunk; // high chunk

		std::uint32_t uQuotient = 0U;
		do
		{
			std::uint32_t uRemainder = 0U;
			for (int i = nTotalChunks - 1; i >= 0; --i)
			{
				const std::uint64_t ullChunkValue = (static_cast<std::uint64_t>(uRemainder) << 32ULL | arrChunks[i]);
				uQuotient = static_cast<std::uint32_t>(ullChunkValue / 1000000000U);
				uRemainder = static_cast<std::uint32_t>(ullChunkValue % 1000000000U);
				arrChunks[i] = uQuotient;
			}

			// faster than unrolling and printing the whole chunk at time
			T* szDestinationChunkEnd = tszDestinationEnd - 8;
			while (tszDestinationEnd != szDestinationChunkEnd)
			{
				tszDestinationEnd -= 2;

				const char* szTwoDigits = &arrDigitPairLUT[(uRemainder % 100U) * 2U];
				tszDestinationEnd[0] = szTwoDigits[0];
				tszDestinationEnd[1] = szTwoDigits[1];

				uRemainder /= 100U;
			}
			*--tszDestinationEnd = static_cast<T>('0' + uRemainder);
		} while (uQuotient != 0U);

		// skip leading zeros
		while (*tszDestinationEnd == '0')
			++tszDestinationEnd;

		// insert negative sign
		if (bIsNegative)
			*--tszDestinationEnd = '-';

		return tszDestinationEnd;
	}

	// extract integer and fractional parts
	std::uint64_t ullIntegerPart = 0U;
	std::uint64_t ullFractionalPart = uMantissa;
	if (const std::uint32_t uShift = -iExponentShift; uShift <= RealTraits_t<V>::kMantissaWidth + 1U)
	{
		ullIntegerPart = (uMantissa >> uShift);
		ullFractionalPart &= ((1ULL << uShift) - 1ULL);
	}

	// reserve space for the integer part
	tszDestinationEnd += IntegerToString_t<UIntType_t, 10U>::MaxCount(); // 64-bit maximum is 20 digits and 1 for rounding carry
	T* tszIntegerEnd = tszDestinationEnd;

	// insert decimal point
	if (iPrecision > 0)
		*tszDestinationEnd++ = '.';

	// adjust fixed-point representation of fractional part based on exponent
	std::uint64_t ullStateLow = ullFractionalPart;
	std::uint64_t ullStateHigh = 0ULL;
	if (const int iStateShift = 128 + iExponentShift; iStateShift >= 128)
	{
		ullStateHigh = ullStateLow << (iStateShift - 128);
		ullStateLow = 0ULL;
	}
	else if (iStateShift >= 64)
	{
		ullStateHigh = ullStateLow << (iStateShift - 64);
		ullStateLow = 0ULL;
	}
	else if (iStateShift >= 0)
	{
		ullStateHigh = ullStateLow >> (64 - iStateShift);
		ullStateLow = ullStateLow << iStateShift;
	}
	else
	{
		if (const int iRightShift = -iStateShift; iRightShift >= 128)
		{
			ullStateLow = 0ULL;
			ullStateHigh = 0ULL;
		}
		else if (iRightShift >= 64)
		{
			ullStateLow = ullStateHigh >> (iRightShift - 64);
			ullStateHigh = 0ULL;
		}
		else
		{
			ullStateLow = (ullStateHigh << (64 - iRightShift)) | (ullStateLow >> iRightShift);
			ullStateHigh = ullStateHigh >> iRightShift;
		}
	}

	// calculate fractional part
	// @note: we need to do this even if the specified precision is zero, to round the integer part correctly
	std::uint32_t uCarry = 0U;
	while (true)
	{
		// early exit if fractional part exhausted
		if ((ullStateHigh | ullStateLow) == 0ULL)
		{
			while (iPrecision-- > 0)
				*tszDestinationEnd++ = '0';

			uCarry = 0U;
			break;
		}

		// 128-bit multiply of the state by 100
		std::uint64_t ullLowProduct;
		std::uint64_t ullHighProduct;
		ullStateLow = DETAIL::Multiply64To128(ullStateLow, 100ULL, &ullLowProduct);
		ullStateHigh = DETAIL::Multiply64To128(ullStateHigh, 100ULL, &ullHighProduct) + ullLowProduct;
		uCarry = static_cast<std::uint32_t>(ullHighProduct);

		if (iPrecision >= 2)
		{
			const char* szTwoDigits = &arrDigitPairLUT[uCarry * 2U];
			*tszDestinationEnd++ = szTwoDigits[0];
			*tszDestinationEnd++ = szTwoDigits[1];
			iPrecision -= 2;

			// store excess digit for rounding decision
			uCarry %= 10U;
		}
		else if (iPrecision > 0)
		{
			*tszDestinationEnd++ = static_cast<char>('0' + uCarry / 10U);

			// store excess digit for rounding decision
			uCarry %= 10U;
			break;
		}
		else
		{
			// store excess digit for rounding decision
			uCarry /= 10U;
			break;
		}
	}

	// check if we must round the value (if the number falls midway, it is rounded to the nearest value with an even least significant digit)
	if (T* tszPropagateDigit = tszDestinationEnd - 1; uCarry > 5U || (uCarry == 5U && ((ullStateHigh | ullStateLow) != 0ULL || (tszPropagateDigit != tszIntegerEnd && (*tszPropagateDigit - '0') & 1))))
	{
		bool bCarryPropagation = true;
		while (bCarryPropagation && tszPropagateDigit > tszIntegerEnd)
		{
			if (*tszPropagateDigit == '9')
			{
				*tszPropagateDigit = '0';
				bCarryPropagation = true;
			}
			else
			{
				*tszPropagateDigit += 1;
				bCarryPropagation = false;
				break;
			}

			--tszPropagateDigit;
		}

		// handle carry propagating to integer part
		if (bCarryPropagation)
			++ullIntegerPart;
	}

	// insert integer part
	if (ullIntegerPart == 0ULL)
		*--tszIntegerEnd = '0';
	else
	{
		while (ullIntegerPart >= 10ULL)
		{
			tszIntegerEnd -= 2;

			const char* szTwoDigits = &arrDigitPairLUT[(ullIntegerPart % 100U) * 2U];
			tszIntegerEnd[0] = szTwoDigits[0];
			tszIntegerEnd[1] = szTwoDigits[1];

			ullIntegerPart /= 100ULL;
		}

		if (ullIntegerPart != 0U)
			*--tszIntegerEnd = static_cast<char>('0' + ullIntegerPart);
	}

	// insert negative sign
	if (bIsNegative)
		*--tszIntegerEnd = '-';

	*tszDestinationEnd = '\0';
	return tszIntegerEnd;
}

/**
 * convert the time point to a string, alternative of 'strftime()', 'wcsftime()'
 * @param[in] nDestinationSize size of the destination buffer including the terminating null, in characters
 * @param[in] tszFormat string specifying the format of conversion. formatting codes are:
 * - '%a' abbreviated weekday name
 * - '%A' full weekday name
 * - '%b' abbreviated month name
 * - '%B' full month name
 * - '%c' date and time representation appropriate for the locale
 * - '%C' century as a decimal number [0 .. 99]
 * - '%d' day of the month as a decimal number [01 .. 31]
 * - '%D' equivalent to "%m/%d/%y"
 * - '%e' day of the month as a decimal number [1 .. 31], where single digits are preceded by a space
 * - '%F' equivalent to "%Y-%m-%d", the ISO-8601 date format, when the year is between [1000 .. 9999] inclusive
 * - '%g' week-based year without century as a decimal number, the ISO-8601 time format [00 .. 99]
 * - '%G' week-based year as a decimal number, the ISO-8601 time format [0000 .. 9999]
 * - '%h' abbreviated month name (equivalent to "%b")
 * - '%H' hour in 24-hour format as a decimal number [00 .. 23]
 * - '%I' hour in 12-hour format as a decimal number [01 .. 12]
 * - '%j' day of the year as a decimal number [001 .. 366]
 * - '%m' month as a decimal number [01 .. 12]
 * - '%M' minute as a decimal number [00 .. 59]
 * - '%n' new line character escape
 * - '%p' the locale's AM/PM designations associated with a 12-hour clock
 * - '%r' the locale's 12-hour clock time
 * - '%R' equivalent to "%H:%M"
 * - '%S' second as decimal number [00 .. 59]
 * - '%t' tab character escape
 * - '%T' equivalent to "%H:%M:%S", the ISO-8601 time format
 * - '%u' weekday as a decimal number, the ISO-8601 time format [1 .. 7]
 * - '%U' week number of the year as a decimal number, where the first Sunday is the first day of week 1 [00 .. 53]
 * - '%V' week number as a decimal number, the ISO-8601 time format [01 .. 53]
 * - '%w' weekday as a decimal number [0 .. 6]
 * - '%W' week number of the year as a decimal number, where the first Monday is the first day of week 1 [00 .. 53]
 * - '%x' date representation appropriate for the locale
 * - '%X' time representation appropriate for the locale
 * - '%y' year without century, as decimal number [00 .. 99]
 * - '%Y' year with century, as decimal number [1900 .. 9999]
 * - '%z' the offset from UTC, the ISO-8601 format, or no characters if no time zone is determinable
 * - '%Z' time zone name or abbreviation, or no characters if no time zone is determinable
 * - '%%' percent character escape
 * @param[in] pTime pointer to the time data structure
 * @remarks: format the @a`pTime` time point according to the @a`tszFormat` and store the result in the buffer @a`tszDestination`. can write past the end of the buffer that is too small
 * @returns: the number of characters placed in @a`szDestination` not including the terminating null, if the total number of characters, including the terminating null, is more than @a`nDestinationSize`, returns 0 and the contents of @a`szDestination` are indeterminate
 **/
template <typename T> requires (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
std::size_t TimeToString(T* tszDestination, std::size_t nDestinationSize, const T* tszFormat, const std::tm* pTime)
{
	// full names of weekdays
	constexpr const char* arrWeekdayNames[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
	// full names of months
	constexpr const char* arrMonthNames[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

	T* tszDestinationEnd = tszDestination;
	std::size_t nRemainingSize = nDestinationSize;

	while (*tszFormat != '\0' && nRemainingSize != 0U)
	{
		// check for the format token
		if (*tszFormat == '%')
		{
			++tszFormat;

			// @note: we don't support any locale (as of default C locale) and non-standard modifiers
			if (*tszFormat == '#' || *tszFormat == 'E' || *tszFormat == 'O')
				++tszFormat;

			const T* tszPreviousEnd = tszDestinationEnd;

			/*
			 * handle format token
			 * valid for default C locale (C standard 7.29.3.5)
			 * @test: C standard and POSIX are very unclear and don't explicitly specify the behaviour when time components are negative
			 */
			switch (*tszFormat)
			{
			case 'a':
			{
				if (nRemainingSize <= 3U)
					return 0U;

				const char* szWeekdayName = arrWeekdayNames[pTime->tm_wday];
				std::size_t nCharacter = 0U;
				do
				{
					*tszDestinationEnd++ = static_cast<T>(szWeekdayName[nCharacter]);
				} while (++nCharacter < 3U);

				break;
			}
			case 'A':
			{
				const char* szWeekdayName = arrWeekdayNames[pTime->tm_wday];
				if (nRemainingSize <= StringLength(szWeekdayName))
					return 0U;

				while (*szWeekdayName != '\0')
					*tszDestinationEnd++ = static_cast<T>(*szWeekdayName++);

				break;
			}
			case 'b':
				[[fallthrough]];
			case 'h': // equivalent to "%b"
			{
				if (nRemainingSize <= 3U)
					return 0U;

				const char* szMonthName = arrMonthNames[pTime->tm_mon];
				std::size_t nCharacter = 0U;
				do
				{
					*tszDestinationEnd++ = static_cast<T>(szMonthName[nCharacter]);
				} while (++nCharacter < 3U);

				break;
			}
			case 'B':
			{
				const char* szMonthName = arrMonthNames[pTime->tm_mon];
				if (nRemainingSize <= StringLength(szMonthName))
					return 0U;

				while (*szMonthName != '\0')
					*tszDestinationEnd++ = static_cast<T>(*szMonthName++);

				break;
			}
			case 'c':
			{
				if (nRemainingSize <= 24U)
					return 0U;

				// default locale equivalent to "%a %b %e %H:%M:%S %Y"
				if constexpr (std::is_same_v<T, wchar_t>)
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR(L"%a %b %e %H:%M:%S %Y"), pTime);
				else
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR("%a %b %e %H:%M:%S %Y"), pTime);

				break;
			}
			case 'C':
			{
				const int iYear = pTime->tm_year + 1900;
				const int iCentury = iYear / 100;

				if (iYear >= -99 && iYear < 0)
				{
					*tszDestinationEnd++ = '-';
					*tszDestinationEnd++ = '0';
				}
				else if (iCentury >= 0 && iCentury < 100)
				{
					const char* szCenturyNumber = &arrDigitPairLUT[static_cast<unsigned int>(iCentury) * 2U];
					*tszDestinationEnd++ = static_cast<T>(*szCenturyNumber++);
					*tszDestinationEnd++ = static_cast<T>(*szCenturyNumber);
				}
				else
					*tszDestinationEnd++ = static_cast<T>(iCentury % 10U + '0');

				break;
			}
			case 'd':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				const char* szMonthDayNumber = &arrDigitPairLUT[static_cast<unsigned int>(pTime->tm_mday) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szMonthDayNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szMonthDayNumber);
				break;
			}
			case 'D':
				[[fallthrough]];
			case 'x':
			{
				if (nRemainingSize <= 8U)
					return 0U;

				// default locale equivalent to "%m/%d/%y"
				if constexpr (std::is_same_v<T, wchar_t>)
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR(L"%m/%d/%y"), pTime);
				else
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR("%m/%d/%y"), pTime);

				break;
			}
			case 'e':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				unsigned int uMonthDay = pTime->tm_mday;
				const char* szMonthDayNumber = &arrDigitPairLUT[uMonthDay * 2U];
				*tszDestinationEnd++ = static_cast<T>(uMonthDay < 10U ? ' ' : *szMonthDayNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szMonthDayNumber);
				break;
			}
			case 'F':
			{
				if (nRemainingSize <= 10U)
					return 0U;

				// equivalent to "%Y-%m-%d"
				if constexpr (std::is_same_v<T, wchar_t>)
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR(L"%Y-%m-%d"), pTime);
				else
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR("%Y-%m-%d"), pTime);

				break;
			}
			case 'g':
				[[fallthrough]];
			case 'G':
				[[fallthrough]];
			case 'V':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				unsigned int uYear = pTime->tm_year + 1900U;
				int iDays = (pTime->tm_yday - (pTime->tm_yday - pTime->tm_wday + 382) % 7 + 3);

				// check if it's a leap year
				if (iDays < 0)
				{
					uYear -= 1U;

					const int iPreviousYearEndDay = pTime->tm_yday + (365 + ((uYear & 3U) == 0U && (uYear % 100U != 0U || uYear % 400U == 0U)));
					iDays = (iPreviousYearEndDay - (iPreviousYearEndDay - pTime->tm_wday + 382) % 7 + 3);
				}
				else
				{
					const int iCurrentYearEndDay = pTime->tm_yday - (365 + ((uYear & 3U) == 0U && (uYear % 100U != 0U || uYear % 400U == 0U)));
					const int iNextDays = (iCurrentYearEndDay - (iCurrentYearEndDay - pTime->tm_wday + 382) % 7 + 3);

					if (iNextDays >= 0)
					{
						uYear += 1U;
						iDays = iNextDays;
					}
				}

				switch (*tszFormat)
				{
				case 'G':
				{
					const char* szCenturyNumber = &arrDigitPairLUT[(uYear / 100U) * 2U];
					*tszDestinationEnd++ = static_cast<T>(*szCenturyNumber++);
					*tszDestinationEnd++ = static_cast<T>(*szCenturyNumber);
					[[fallthrough]];
				}
				case 'g':
				{
					const char* szYearNumber = &arrDigitPairLUT[(uYear % 100U) * 2U];
					*tszDestinationEnd++ = static_cast<T>(*szYearNumber++);
					*tszDestinationEnd++ = static_cast<T>(*szYearNumber);
					break;
				}
				default:
				{
					const char* szWeekNumber = &arrDigitPairLUT[(iDays / 7U + 1U) * 2U];
					*tszDestinationEnd++ = static_cast<T>(*szWeekNumber++);
					*tszDestinationEnd++ = static_cast<T>(*szWeekNumber);
					break;
				}
				}

				break;
			}
			case 'H':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				const char* szHourNumber = &arrDigitPairLUT[static_cast<unsigned int>(pTime->tm_hour) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szHourNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szHourNumber);
				break;
			}
			case 'I':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				unsigned int uHourFormat = pTime->tm_hour % 12U;
				uHourFormat = ((uHourFormat == 0U) ? 12U : uHourFormat);

				const char* szHourNumber = &arrDigitPairLUT[uHourFormat * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szHourNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szHourNumber);
				break;
			}
			case 'j':
			{
				if (nRemainingSize <= 3U)
					return 0U;

				unsigned int uYearDay = pTime->tm_yday + 1U;
				*tszDestinationEnd++ = static_cast<T>((uYearDay / 100U) % 10U + '0');

				const char* szYearDayNumber = &arrDigitPairLUT[(uYearDay % 100U) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szYearDayNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szYearDayNumber);
				break;
			}
			case 'm':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				const char* szMonthNumber = &arrDigitPairLUT[(pTime->tm_mon + 1U) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szMonthNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szMonthNumber);
				break;
			}
			case 'M':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				const char* szMinuteNumber = &arrDigitPairLUT[static_cast<unsigned int>(pTime->tm_min) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szMinuteNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szMinuteNumber);
				break;
			}
			case 'n':
			{
				*tszDestinationEnd++ = '\n';
				break;
			}
			case 'p':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				*tszDestinationEnd++ = ((pTime->tm_hour < 12) ? 'A' : 'P');
				*tszDestinationEnd++ = 'M';
				break;
			}
			case 'r':
			{
				if (nRemainingSize <= 11U)
					return 0U;

				// default locale equivalent to "%I:%M:%S %p"
				if constexpr (std::is_same_v<T, wchar_t>)
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR(L"%I:%M:%S %p"), pTime);
				else
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR("%I:%M:%S %p"), pTime);

				break;
			}
			case 'R':
			{
				if (nRemainingSize <= 5U)
					return 0U;

				const char* szHourNumber = &arrDigitPairLUT[static_cast<unsigned int>(pTime->tm_hour) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szHourNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szHourNumber);

				*tszDestinationEnd++ = ':';

				const char* szMinuteNumber = &arrDigitPairLUT[static_cast<unsigned int>(pTime->tm_min) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szMinuteNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szMinuteNumber);
				break;
			}
			case 'S':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				const char* szSecondNumber = &arrDigitPairLUT[static_cast<unsigned int>(pTime->tm_sec) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szSecondNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szSecondNumber);
				break;
			}
			case 't':
			{
				*tszDestinationEnd++ = '\t';
				break;
			}
			case 'T':
				[[fallthrough]];
			case 'X':
			{
				if (nRemainingSize <= 8U)
					return 0U;

				// default locale equivalent to "%H:%M:%S"
				if constexpr (std::is_same_v<T, wchar_t>)
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR(L"%H:%M:%S"), pTime);
				else
					tszDestinationEnd += TimeToString(tszDestinationEnd, nRemainingSize, Q_STR("%H:%M:%S"), pTime);

				break;
			}
			case 'u':
			{
				*tszDestinationEnd++ = static_cast<T>(((pTime->tm_wday + 6U) % 7U + 1U) + '0');
				break;
			}
			case 'U':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				const char* szWeekNumber = &arrDigitPairLUT[((pTime->tm_yday + 7U - pTime->tm_wday) / 7U) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szWeekNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szWeekNumber);
				break;
			}
			case 'w':
			{
				*tszDestinationEnd++ = static_cast<T>(pTime->tm_wday + '0');
				break;
			}
			case 'W':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				const char* szWeekNumber = &arrDigitPairLUT[((pTime->tm_yday + 7U - ((pTime->tm_wday + 6U) % 7U)) / 7U) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szWeekNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szWeekNumber);
				break;
			}
			case 'y':
			{
				if (nRemainingSize <= 2U)
					return 0U;

				const char* szYearNumber = &arrDigitPairLUT[((pTime->tm_year + 1900U) % 100U) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szYearNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szYearNumber);
				break;
			}
			case 'Y':
			{
				if (nRemainingSize <= 4U)
					return 0U;

				unsigned int uYear = pTime->tm_year + 1900U;

				const char* szCenturyNumber = &arrDigitPairLUT[(uYear / 100U) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szCenturyNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szCenturyNumber);

				const char* szYearNumber = &arrDigitPairLUT[(uYear % 100U) * 2U];
				*tszDestinationEnd++ = static_cast<T>(*szYearNumber++);
				*tszDestinationEnd++ = static_cast<T>(*szYearNumber);
				break;
			}
			case 'z':
			{
				// check if the time zone extension member is present
				if constexpr (HasTmGmtOff_t<std::tm>)
				{
					auto GetTimeZoneOffset = []<HasTmGmtOff_t Tm_t>(const Tm_t* pTimeEx)
					{
						return pTimeEx->tm_gmtoff;
					};

					// check if time zone information is available
					if (pTime->tm_isdst < 0)
						break;

					if (nRemainingSize <= 5U)
						return 0U;

					std::int64_t llOffset = GetTimeZoneOffset(pTime) / 60LL;
					if (llOffset < 0LL)
					{
						*tszDestinationEnd++ = '-';
						llOffset = -llOffset;
					}
					else
						*tszDestinationEnd++ = '+';

					const char* szDigitPair = &arrDigitPairLUT[(llOffset / 60U) * 2U];
					*tszDestinationEnd++ = static_cast<T>(*szDigitPair++);
					*tszDestinationEnd++ = static_cast<T>(*szDigitPair);
					szDigitPair = &arrDigitPairLUT[(llOffset % 60U) * 2U];
					*tszDestinationEnd++ = static_cast<T>(*szDigitPair++);
					*tszDestinationEnd++ = static_cast<T>(*szDigitPair);
				}

				// @test: do we have other ways to get it on windows other than winapi calls?

				break;
			}
			case 'Z':
			{
				// check if the time zone extension member is present
				if constexpr (HasTmZone_t<std::tm>)
				{
					auto GetTimeZoneName = []<HasTmZone_t Tm_t>(const Tm_t* pTimeEx)
					{
						return pTimeEx->tm_zone;
					};

					const char* szTimeZoneName = GetTimeZoneName(pTime);
					if (nRemainingSize <= StringLength(szTimeZoneName))
						return 0U;

					while (*szTimeZoneName != '\0')
						*tszDestinationEnd++ = static_cast<T>(*szTimeZoneName++);
				}
				// otherwise print the time zone name
				else
				{
					if (nRemainingSize <= 3U)
						return 0U;

					*tszDestinationEnd++ = 'P';
					*tszDestinationEnd++ = pTime->tm_isdst ? 'D' : 'S';
					*tszDestinationEnd++ = 'T';
				}
				// @test: do we have other ways to get it on windows other than winapi calls?

				break;
			}
			case '%': // percent escape
			{
				*tszDestinationEnd++ = *tszFormat;
				break;
			}
			default:
				Q_ASSERT(false); // unknown token!
				break;
			}

			nRemainingSize -= tszDestinationEnd - tszPreviousEnd;
			++tszFormat;
			continue;
		}

		*tszDestinationEnd++ = *tszFormat++;
		--nRemainingSize;
	}

	// check if the limit was reached before the entire string could be stored
	if (nRemainingSize == 0U)
		return 0U;

	*tszDestinationEnd = '\0';
	return nDestinationSize - nRemainingSize;
}

/// convert the string to an integer number, alternative of 'atoi()', '_wtoi()', '_atoi64()', '_wtoi64()', 'atol()', '_wtol()', 'atoll()', '_wtoll()', 'strtol()', 'wcstol()', 'strtoll()', 'wcstoll()', '_strtoi64()', '_wcstoi64()', 'strtoul()', 'wcstoul()', 'strtoull()', 'wcstoull()'
/// @param[in] tszSourceBegin string to convert value from
/// @param[out] ptszSourceEnd [optional] pointer to the last scanned character during conversion
/// @param[in] iBase number of digits used to represent number. value in range [2 .. 36] or 0 to automatically determine number base in range [2 .. 16]
/// @param[out] pnError [optional] code of the conversion error, if any
/// @remarks: converts @a`tszSourceBegin` string containing sequence of characters in form [whitespace][{+|-}][digits] that can be interpreted as a numeric value to specified integer type. stops reading at the first character that can't be recognized as part of number and store its position into @a`ptszSourceEnd`
/// @returns: integer number converted from a string or 0 if no conversion could be performed
template <typename V = int, typename T> requires (std::is_integral_v<V> && (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>))
constexpr V StringToInteger(const T* tszSourceBegin, T** ptszSourceEnd = nullptr, int iBase = 0, int* pnError = nullptr)
{
	// set a local variable as error output if it's not set
	int nError = 0;
	if (pnError == nullptr)
		pnError = &nError;

	if (iBase < 0 || iBase == 1 || iBase > kMaxNumberBase)
	{
		*pnError = EINVAL; // given number base is out of range
		return 0;
	}

	const T* tszSourceCurrent = tszSourceBegin;

	// strip off the leading blanks
	while (IsSpace(static_cast<int>(*tszSourceCurrent)))
		++tszSourceCurrent;

	// check for a sign
	const bool bPositive = ((*tszSourceCurrent == '+' || *tszSourceCurrent == '-') ? (*tszSourceCurrent++ == '+') : true);
	constexpr bool bUnsigned = !std::numeric_limits<V>::is_signed;

	// check if user provided exact number base
	if (iBase > 0)
	{
		// strip 0x or 0X
		if (iBase == 16 && *tszSourceCurrent == '0' && (tszSourceCurrent[1] | ('a' ^ 'A')) == 'x')
			tszSourceCurrent += 2;
	}
	// otherwise try to determine base automatically
	else if (*tszSourceCurrent == '0')
	{
		if ((*tszSourceCurrent++ | ('a' ^ 'A')) == 'x')
		{
			// a hexadecimal constant consists of the prefix 0x or 0X followed by a sequence of the decimal digits and the letters a (or A) through f (or F) with values 10 through 15 respectively (C standard 6.4.4.1)
			iBase = 16;
			++tszSourceCurrent;
		}
		else
			// an octal constant consists of the prefix 0 optionally followed by a sequence of the digits 0 through 7 only (C standard 6.4.4.1)
			iBase = 8;
	}
	else
		// a decimal constant begins with a nonzero digit and consists of a sequence of decimal digits (C standard 6.4.4.1)
		iBase = 10;

	constexpr std::uint64_t ullNegativeMax = (bUnsigned ? (std::numeric_limits<V>::max)() : (static_cast<std::uint64_t>((std::numeric_limits<V>::max)()) + 1ULL));
	const std::uint64_t ullAbsoluteMax = (bPositive ? (std::numeric_limits<V>::max)() : ullNegativeMax);
	const std::uint64_t ullAbsoluteMaxOfBase = ullAbsoluteMax / iBase;

	bool bIsNumber = false;
	std::uint64_t ullResult = 0ULL;

	for (bool bIsDigit, bIsAlpha = false; ((bIsDigit = IsDigit(static_cast<int>(*tszSourceCurrent)))) || ((bIsAlpha = IsAlpha(static_cast<int>(*tszSourceCurrent)))); ) // @note: double parenthesis to suppress warnings
	{
		int iCurrentDigit = 0;

		if (bIsDigit)
			iCurrentDigit = *tszSourceCurrent - '0';
		else if (bIsAlpha)
			iCurrentDigit = (*tszSourceCurrent | ('a' ^ 'A')) - 'a' + 0xA;

		if (iCurrentDigit >= iBase)
			break;

		bIsNumber = true;
		++tszSourceCurrent;

		// if the number has already hit the maximum value for the current type then the result cannot change, but we still need to advance source to the end of the number
		if (ullResult == ullAbsoluteMax)
		{
			*pnError = ERANGE; // numeric overflow
			continue;
		}

		if (ullResult <= ullAbsoluteMaxOfBase)
			ullResult *= iBase;
		else
		{
			ullResult = ullAbsoluteMax;
			*pnError = ERANGE; // numeric overflow
		}

		if (ullResult <= ullAbsoluteMax - iCurrentDigit)
			ullResult += iCurrentDigit;
		else
		{
			ullResult = ullAbsoluteMax;
			*pnError = ERANGE; // numeric overflow
		}
	}

	if (ptszSourceEnd != nullptr)
		*ptszSourceEnd = const_cast<T*>(bIsNumber ? tszSourceCurrent : tszSourceBegin);

	// clamp on overflow
	if (*pnError == ERANGE)
		return ((bPositive || bUnsigned) ? (std::numeric_limits<V>::max)() : (std::numeric_limits<V>::min)());

	return (bPositive ? static_cast<V>(ullResult) : -static_cast<V>(ullResult));
}

/// convert the string to a floating-point number, alternative of 'atof()', _wtof(), 'atod()', 'strtof()', 'strtod()'
/// @param[in] tszSourceBegin string to convert value from
/// @param[out] ptszSourceEnd [optional] pointer to the last scanned character during conversion
/// @param[out] pnError [optional] code of the conversion error, if any
/// @remarks: converts @a`tszSourceBegin` string containing sequence of characters in form [whitespace][{+|-}][digits][.digits][{e|E}[{+|-}]digits] that can be interpreted as a numeric value to specified floating-point type. stops reading at the first character that can't be recognized as part of number and store its position into @a`ptszSourceEnd`
/// @returns: floating-point number converted from a string or 0.0 if no conversion could be performed
template <typename V = float, typename T> requires (std::is_floating_point_v<V> && (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>))
constexpr V StringToReal(const T* tszSourceBegin, T** ptszSourceEnd = nullptr, int* pnError = nullptr)
{
	using UIntType_t = typename RealTraits_t<V>::BitEquivalent_t;
	UIntType_t uBits;

	// set a local variable as error output if it's not set
	int nError = 0;
	if (pnError == nullptr)
		pnError = &nError;

	const T* tszSourceEnd = tszSourceBegin;

	// skip leading whitespaces
	while (IsSpace(static_cast<int>(*tszSourceEnd)))
		++tszSourceEnd;

	// extract the sign
	UIntType_t uSign = 0U;
	if (*tszSourceEnd == '+')
		++tszSourceEnd;
	else if (*tszSourceEnd == '-')
	{
		uSign = RealTraits_t<V>::kSignMask;
		++tszSourceEnd;
	}

	// check for NaN value
	// @todo: we must also support nan(n-char-sequence)
	if ((tszSourceEnd[0] | ('a' ^ 'A')) == 'n' && (tszSourceEnd[1] | ('a' ^ 'A')) == 'a' && (tszSourceEnd[2] | ('a' ^ 'A')) == 'n')
	{
		if (ptszSourceEnd != nullptr)
		{
			tszSourceEnd += 3;
			*ptszSourceEnd = const_cast<T*>(tszSourceEnd);
		}

		uBits = (uSign | RealTraits_t<V>::kExponentMask | RealTraits_t<V>::kQuietNanMask);
		return std::bit_cast<V>(uBits);
	}

	// check for infinity value
	if ((tszSourceEnd[0] | ('a' ^ 'A')) == 'i' && (tszSourceEnd[1] | ('a' ^ 'A')) == 'n' && (tszSourceEnd[2] | ('a' ^ 'A')) == 'f')
	{
		// @test: tho C standard states that pointer to "inite" should be stored, we do also account it
		if (ptszSourceEnd != nullptr)
		{
			tszSourceEnd += 3;
			if ((tszSourceEnd[0] | ('a' ^ 'A')) == 'i' && (tszSourceEnd[1] | ('a' ^ 'A')) == 'n' && (tszSourceEnd[2] | ('a' ^ 'A')) == 'i' && (tszSourceEnd[3] | ('a' ^ 'A')) == 't' && (tszSourceEnd[4] | ('a' ^ 'A')) == 'y')
				tszSourceEnd += 5;

			*ptszSourceEnd = const_cast<T*>(tszSourceEnd);
		}

		uBits = (uSign | RealTraits_t<V>::kExponentMask);
		return std::bit_cast<V>(uBits);
	}

	// skip leading zeros
	while (*tszSourceEnd == '0')
		++tszSourceEnd;

	// max count of digits to parse, including excess digit for correct rounding
	constexpr int kMaxPrecisionDigits = std::numeric_limits<double>::max_digits10 + 1;

	int nDigitCount = 0;
	int iExponentDecimal = 0;
	BigInteger_t mantissaDecimal = { };

	// @todo: we must also support hexadecimal form
	// extract integer part
	while (IsDigit(static_cast<int>(*tszSourceEnd)))
	{
		if (++nDigitCount <= kMaxPrecisionDigits)
			mantissaDecimal.ullLow = mantissaDecimal.ullLow * 10U + (*tszSourceEnd - '0');
		else
			++iExponentDecimal;

		++tszSourceEnd;
	}

	// extract fractional part
	if ((*tszSourceEnd | ('a' ^ 'A')) == 'e' || *tszSourceEnd == '.')
	{
		if (*tszSourceEnd == '.')
			++tszSourceEnd;

		while (IsDigit(static_cast<int>(*tszSourceEnd)))
		{
			if (++nDigitCount <= kMaxPrecisionDigits)
			{
				mantissaDecimal.ullLow = mantissaDecimal.ullLow * 10U + (*tszSourceEnd - '0');
				--iExponentDecimal;
			}

			++tszSourceEnd;
		}
	}

	// check if exponent is present
	if ((*tszSourceEnd | ('a' ^ 'A')) == 'e')
	{
		++tszSourceEnd;

		// extract exponent sign
		bool bExponentSign = false;
		if (*tszSourceEnd == '+')
			++tszSourceEnd;
		else if (*tszSourceEnd == '-')
		{
			bExponentSign = true;
			++tszSourceEnd;
		}

		// extract exponent value
		std::uint32_t uExponentValue = 0U;
		while (IsDigit(static_cast<int>(*tszSourceEnd)))
			uExponentValue = uExponentValue * 10U + (*tszSourceEnd++ - '0');

		iExponentDecimal += bExponentSign ? -static_cast<int>(uExponentValue) : static_cast<int>(uExponentValue);
	}

	if (ptszSourceEnd != nullptr)
		*ptszSourceEnd = const_cast<T*>(tszSourceEnd);

	// check for exponent underflow
	if (iExponentDecimal + nDigitCount < (RealTraits_t<V>::kExponentDecimalMin + 1))
	{
		*pnError = ERANGE;

		// signed zero
		uBits = uSign;
		return std::bit_cast<V>(uBits);
	}

	// check for exponent overflow
	if (iExponentDecimal > (RealTraits_t<V>::kExponentDecimalMax + kMaxPrecisionDigits))
	{
		*pnError = ERANGE;

		// signed infinity
		uBits = uSign | RealTraits_t<V>::kExponentMask;
		return std::bit_cast<V>(uBits);
	}

	// convert decimal exponent into binary
	int iExponent = 0;

	// check if the number is true zero
	if (mantissaDecimal.ullLow == 0ULL)
	{
		iExponent = RealTraits_t<V>::kExponentMin - 1;
		mantissaDecimal.ullLow = 0ULL;
	}
	else
	{
		// normalize mantissa
		// keep high 4 bits non-zero
		std::uint8_t nLeadingZerosCount;
		if (mantissaDecimal.ullHigh != 0ULL)
			nLeadingZerosCount = static_cast<std::uint8_t>(std::countl_zero(mantissaDecimal.ullHigh)) - 4U;
		else
			nLeadingZerosCount = static_cast<std::uint8_t>(std::countl_zero(mantissaDecimal.ullLow)) + 60U;

		mantissaDecimal.ShiftLeft(nLeadingZerosCount);
		iExponent -= nLeadingZerosCount;

		// take performed shift into account
		iExponent += 123;

		// @todo: normalization approach is robust but quite slow
		// check for positive exponent
		if (iExponentDecimal > 0)
		{
			do
			{
				mantissaDecimal.Multiply(10ULL);

				// keep high 4 bits all zero
				while ((mantissaDecimal.ullHigh >> 60ULL) != 0ULL)
				{
					mantissaDecimal.ShiftRight(1U);
					++iExponent;
				}
			} while (--iExponentDecimal > 0);
		}
		// otherwise check for negative exponent
		else if (iExponentDecimal < 0)
		{
			do
			{
				mantissaDecimal.Divide10();

				if ((mantissaDecimal.ullHigh | mantissaDecimal.ullLow) != 0ULL)
				{
					// @todo: do use bsr/clz only when it's guaranteed to compile into appropriate instructions otherwise it will be less efficient
					// keep high 4 bits non-zero
#if 0
					int nLeadingZerosCount;
					if (mantissaDecimal.ullHigh != 0ULL)
						nLeadingZerosCount = std::countl_zero(mantissaDecimal.ullHigh) - 4;
					else
						nLeadingZerosCount = std::countl_zero(mantissaDecimal.ullLow) + 60;

					mantissaDecimal.ShiftLeft(nLeadingZerosCount);
					iExponent -= nLeadingZerosCount;
#else
					while ((mantissaDecimal.ullHigh >> 60ULL) == 0ULL)
					{
						mantissaDecimal.ShiftLeft(1U);
						--iExponent;
					}

					mantissaDecimal.ShiftRight(1U);
					++iExponent;
#endif
				}
			} while (++iExponentDecimal < 0);
		}

		mantissaDecimal.ShiftLeft(4U);
	}

	// convert decimal mantissa into binary
	UIntType_t uMantissa;
	if (iExponent > RealTraits_t<V>::kExponentMax)
	{
		// infinity
		iExponent = (1 << RealTraits_t<V>::kExponentWidth) - 1;
		uMantissa = 0U;

		*pnError = ERANGE;
	}
	else
	{
		// check if value is denormalized
		if (iExponent < RealTraits_t<V>::kExponentMin)
		{
			// check for true zero
			if ((mantissaDecimal.ullHigh | mantissaDecimal.ullLow) == 0ULL)
				uMantissa = 0U;
			// check for denormalized exponent underflow
			else if (iExponent < RealTraits_t<V>::kExponentDenormalMin - 1)
			{
				uMantissa = 0U;
				*pnError = ERANGE;
			}
			else
			{
				mantissaDecimal.ullHigh >>= (-RealTraits_t<V>::kExponentBias - iExponent);
				uMantissa = mantissaDecimal.ullHigh >> (64ULL - RealTraits_t<V>::kMantissaWidth);
			}

			iExponent = -RealTraits_t<V>::kExponentMax;
		}
		// otherwise value is normalized
		else
		{
			// set implicit bit
			mantissaDecimal.ullHigh <<= 1ULL;
			uMantissa = mantissaDecimal.ullHigh >> (64ULL - RealTraits_t<V>::kMantissaWidth);
		}

		/*
		 * apply rounding rules:
		 * [GRS] guard (G), round (R), and sticky (S) bits
		 *  000 -> NO ROUND
		 *  010 -> NO ROUND
		 *  100 -> NO ROUND (TIE)
		 *  101 -> ROUND UP
		 *  110 -> ROUND UP
		 *  111 -> ROUND UP
		 */
		const std::uint32_t kGuardShift = 64U - RealTraits_t<V>::kMantissaWidth - 1U;
		const bool bRoundUp = ((mantissaDecimal.ullHigh & (1ULL << kGuardShift)) != 0ULL && (mantissaDecimal.ullHigh & ((1ULL << kGuardShift) - 1ULL)) != 0ULL);
		uMantissa = (uMantissa + bRoundUp) & RealTraits_t<V>::kMantissaMask;

		// check for mantissa overflow
		if (uMantissa == 0U && bRoundUp)
			++iExponent;

		// add bias to exponent
		iExponent += RealTraits_t<V>::kExponentBias;
	}

	uBits = static_cast<UIntType_t>(uSign) | (static_cast<UIntType_t>(iExponent) << RealTraits_t<V>::kMantissaWidth) | uMantissa;
	return std::bit_cast<V>(uBits);
}
#endif
