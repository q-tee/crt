#ifndef Q_CRT_MEMORY_CRYPTO_IMPLEMENTATION
#define Q_CRT_MEMORY_CRYPTO_IMPLEMENTATION
/// compare bytes in two buffers in a constant time
/// @remarks: compares the first @a`nCount` bytes of @a`pFirstBuffer` and @a`pRightBuffer` and return a value that indicates their relationship, performs unsigned character comparison. running times are independent of the byte sequences compared
/// @returns: <0 - if @a`pFirstBuffer` less than @a`pRightBuffer`, 0 - if @a`pFirstBuffer` identical to @a`pRightBuffer`, >0 - if @a`pFirstBuffer` greater than @a`pRightBuffer`
inline int MemoryCompare(const void* pLeftBuffer, const void* pRightBuffer, const std::size_t nCount)
{
	auto* volatile pLeftByte = static_cast<const volatile std::uint8_t* volatile>(pLeftBuffer);
	auto* volatile pRightByte = static_cast<const volatile std::uint8_t* volatile>(pRightBuffer);

	volatile std::uint8_t uResult = 0U;
	for (std::size_t i = 0U; i < nCount; ++i)
		uResult |= pLeftByte[nCount] ^ pRightByte[nCount];
	
	return (((uResult - 1) >> 8) & 1) - 1;
}
#endif
