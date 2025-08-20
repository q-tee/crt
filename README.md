# about
a lightweight and efficient implementation of the C-runtime standard library for C++20

> [!NOTE]
> be aware that implementation of the some functions conform to the POSIX standard rather than the C standard. but inline intrinsics does preserve original C standard behaviour. see [compability](#compability) for details

# usage
you can also define the next values to include additional features:
definition             | note
---------------------- | ----
Q_CRT_MEMORY_CRYPTO    | add crypto-specific (timing-safe etc) memory functionality
Q_CRT_STRING_WIDE_TYPE | add full Unicode support for methods related to character types. note that this includes lookup tables with a total binary size of about ~11KB
Q_CRT_STRING_NATURAL   | add functionality based on "natural" order comparison algorithm
Q_CRT_STRING_CONVERT   | add string conversion functionality to convert strings to other data types and vice versa. note that this includes lookup tables with a total binary size of about ~1KB
Q_CRT_STRING_ENCODE    | add encoding and decoding functionality, in particular UTF

# compability
behaviour of some functions has been changed in favor of the POSIX specification or GNU extensions to make them more sensible or less complex:
function    | note
----------- | ----
MemorySet   | return value changed in favor of `mempset()`, type of the fill byte argument is changed to `std::uint8_t`
MemorySetW  | return value changed in favor of `wmempset()`
MemoryCopy  | return value changed in favor of `mempcpy()`
MemoryCopyW | return value changed in favor of `wmempcpy()`
StringCopy  | return value changed in favor of `stpcpy()` \/ `wcpcpy()`
StringCopyN | return value changed in favor of `stpncpy()` \/ `wcpncpy()`
StringCat   | return value changed in favor of `stpcat()` \/ `wcpcat()`
StringCatN  | return value changed in favor of `stpncat()` \/ `wcpncat()`
StringSpan  | return value changed in favor of `strpspn()` \/ `wcspspn()`

other than that behaviour of the rest functions are based on ISO-9899 (C standard)

# further information
requires [common](https://github.com/q-tee/common/) library to be also installed.
you can read about installation, contributing and look for other general information on the [q-tee](https://github.com/q-tee/) main page.