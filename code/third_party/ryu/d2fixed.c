// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

#include "common.h"
#include "d2fixed_full_table.h"
#include "digit_table.h"
#include "d2s_intrinsics.h"

#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11
#define DOUBLE_BIAS          1023

#define POW10_ADDITIONAL_BITS 120

static inline __uint128_t
umul256(const __uint128_t  a,
        const uint64_t     bHi,
        const uint64_t     bLo,
        __uint128_t *const productHi)
{
    const uint64_t aLo = (uint64_t)a;
    const uint64_t aHi = (uint64_t)(a >> 64);

    const __uint128_t b00 = (__uint128_t)aLo * bLo;
    const __uint128_t b01 = (__uint128_t)aLo * bHi;
    const __uint128_t b10 = (__uint128_t)aHi * bLo;
    const __uint128_t b11 = (__uint128_t)aHi * bHi;

    const uint64_t b00Lo = (uint64_t)b00;
    const uint64_t b00Hi = (uint64_t)(b00 >> 64);

    const __uint128_t mid1   = b10 + b00Hi;
    const uint64_t    mid1Lo = (uint64_t)(mid1);
    const uint64_t    mid1Hi = (uint64_t)(mid1 >> 64);

    const __uint128_t mid2   = b01 + mid1Lo;
    const uint64_t    mid2Lo = (uint64_t)(mid2);
    const uint64_t    mid2Hi = (uint64_t)(mid2 >> 64);

    const __uint128_t pHi = b11 + mid1Hi + mid2Hi;
    const __uint128_t pLo = ((__uint128_t)mid2Lo << 64) | b00Lo;

    *productHi = pHi;
    return pLo;
}

// Returns the high 128 bits of the 256-bit product of a and b.
static inline __uint128_t
umul256_hi(const __uint128_t a, const uint64_t bHi, const uint64_t bLo)
{
    // Reuse the umul256 implementation.
    // Optimizers will likely eliminate the instructions used to compute the
    // low part of the product.
    __uint128_t hi;
    umul256(a, bHi, bLo, &hi);
    return hi;
}

// Unfortunately, gcc/clang do not automatically turn a 128-bit integer division
// into a multiplication, so we have to do it manually.
static inline uint32_t
uint128_mod1e9(const __uint128_t v)
{
    // After multiplying, we're going to shift right by 29, then truncate to uint32_t.
    // This means that we need only 29 + 32 = 61 bits, so we can truncate to uint64_t before
    // shifting.
    const uint64_t multiplied =
        (uint64_t)umul256_hi(v, 0x89705F4136B4A597u, 0x31680A88F8953031u);

    // For uint32_t truncation, see the mod1e9() comment in d2s_intrinsics.h.
    const uint32_t shifted = (uint32_t)(multiplied >> 29);

    return ((uint32_t)v) - 1000000000 * shifted;
}

// Best case: use 128-bit type.
static inline uint32_t
mulShift_mod1e9(const uint64_t m, const uint64_t *const mul, const int32_t j)
{
    const __uint128_t b0 = ((__uint128_t)m) * mul[0]; // 0
    const __uint128_t b1 = ((__uint128_t)m) * mul[1]; // 64
    const __uint128_t b2 = ((__uint128_t)m) * mul[2]; // 128

    // assert(j >= 128);
    // assert(j <= 180);

    //  j: [128, 256)
    const __uint128_t mid = b1 + (uint64_t)(b0 >> 64);  // 64
    const __uint128_t s1  = b2 + (uint64_t)(mid >> 64); // 128
    return uint128_mod1e9(s1 >> (j - 128));
}

// Convert `digits` to a sequence of decimal digits. Append the digits to the result.
// The caller has to guarantee that:
//   10^(olength-1) <= digits < 10^olength
// e.g., by passing `olength` as `decimalLength9(digits)`.
static inline void
append_n_digits(const uint32_t olength, uint32_t digits, char *const result)
{
    uint32_t i = 0;
    while(digits >= 10000)
    {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
        const uint32_t c = digits - 10000 * (digits / 10000);
#else
        const uint32_t c = digits % 10000;
#endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + olength - i - 2, DIGIT_TABLE + c0, 2);
        memcpy(result + olength - i - 4, DIGIT_TABLE + c1, 2);
        i += 4;
    }
    if(digits >= 100)
    {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + olength - i - 2, DIGIT_TABLE + c, 2);
        i += 2;
    }
    if(digits >= 10)
    {
        const uint32_t c = digits << 1;
        memcpy(result + olength - i - 2, DIGIT_TABLE + c, 2);
    }
    else
    {
        result[0] = (char)('0' + digits);
    }
}

// Convert `digits` to decimal and write the last `count` decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void
append_c_digits(const uint32_t count, uint32_t digits, char *const result)
{
    // Copy pairs of digits from DIGIT_TABLE.
    uint32_t i = 0;
    for(; i < count - 1; i += 2)
    {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + count - i - 2, DIGIT_TABLE + c, 2);
    }
    // Generate the last digit if count is odd.
    if(i < count)
    {
        const char c          = (char)('0' + (digits % 10));
        result[count - i - 1] = c;
    }
}

// Convert `digits` to decimal and write the last 9 decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void
append_nine_digits(uint32_t digits, char *const result)
{
    if(digits == 0)
    {
        memset(result, '0', 9);
        return;
    }

    for(uint32_t i = 0; i < 5; i += 4)
    {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
        const uint32_t c = digits - 10000 * (digits / 10000);
#else
        const uint32_t c = digits % 10000;
#endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + 7 - i, DIGIT_TABLE + c0, 2);
        memcpy(result + 5 - i, DIGIT_TABLE + c1, 2);
    }
    result[0] = (char)('0' + digits);
}

static inline uint32_t
indexForExponent(const uint32_t e)
{
    return (e + 15) / 16;
}

static inline uint32_t
pow10BitsForIndex(const uint32_t idx)
{
    return 16 * idx + POW10_ADDITIONAL_BITS;
}

static inline uint32_t
lengthForIndex(const uint32_t idx)
{
    // +1 for ceil, +16 for mantissa, +8 to round up when dividing by 9
    return (log10Pow2(16 * (int32_t)idx) + 1 + 16 + 8) / 9;
}

static inline int
copy_special_str_printf(char *const result, const bool sign, const uint64_t mantissa)
{
    if(mantissa)
    {
        memcpy(result, "nan", 3);
        return 3;
    }
    if(sign)
    {
        result[0] = '-';
    }
    memcpy(result + sign, "Infinity", 8);
    return sign + 8;
}

static int
d2fixed_buffered_n(double d, uint32_t precision, char *result)
{
    const uint64_t bits = double_to_bits(d);

    // Decode bits into sign, mantissa, and exponent.
    const bool ieeeSign = ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
    const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
    const uint32_t ieeeExponent =
        (uint32_t)((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));

    // Case distinction; exit early for the easy cases.
    if(ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u))
    {
        return copy_special_str_printf(result, ieeeSign, ieeeMantissa);
    }
    if(ieeeExponent == 0 && ieeeMantissa == 0)
    {
        int index = 0;
        if(ieeeSign)
        {
            result[index++] = '-';
        }
        result[index++] = '0';
        if(precision > 0)
        {
            result[index++] = '.';
            memset(result + index, '0', precision);
            index += precision;
        }
        return index;
    }

    int32_t  e2;
    uint64_t m2;
    if(ieeeExponent == 0)
    {
        e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = ieeeMantissa;
    }
    else
    {
        e2 = (int32_t)ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
    }

    int  index   = 0;
    bool nonzero = false;
    if(ieeeSign)
    {
        result[index++] = '-';
    }
    if(e2 >= -52)
    {
        const uint32_t idx     = e2 < 0 ? 0 : indexForExponent((uint32_t)e2);
        const uint32_t p10bits = pow10BitsForIndex(idx);
        const int32_t  len     = (int32_t)lengthForIndex(idx);

        for(int32_t i = len - 1; i >= 0; --i)
        {
            const uint32_t j = p10bits - e2;
            // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or
            // above, which is a slightly faster code path in mulShift_mod1e9. Instead, we can
            // just increase the multipliers.
            const uint32_t digits = mulShift_mod1e9(m2 << 8,
                                                    POW10_SPLIT[POW10_OFFSET[idx] + i],
                                                    (int32_t)(j + 8));
            if(nonzero)
            {
                append_nine_digits(digits, result + index);
                index += 9;
            }
            else if(digits != 0)
            {
                const uint32_t olength = decimalLength9(digits);
                append_n_digits(olength, digits, result + index);
                index += olength;
                nonzero = true;
            }
        }
    }
    if(!nonzero)
    {
        result[index++] = '0';
    }
    if(precision > 0)
    {
        result[index++] = '.';
    }

    if(e2 < 0)
    {
        const int32_t  idx    = -e2 / 16;
        const uint32_t blocks = precision / 9 + 1;
        // 0 = don't round up; 1 = round up unconditionally; 2 = round up if odd.
        int      roundUp = 0;
        uint32_t i       = 0;
        if(blocks <= MIN_BLOCK_2[idx])
        {
            i = blocks;
            memset(result + index, '0', precision);
            index += precision;
        }
        else if(i < MIN_BLOCK_2[idx])
        {
            i = MIN_BLOCK_2[idx];
            memset(result + index, '0', 9 * i);
            index += 9 * i;
        }
        for(; i < blocks; ++i)
        {
            const int32_t  j = ADDITIONAL_BITS_2 + (-e2 - 16 * idx);
            const uint32_t p = POW10_OFFSET_2[idx] + i - MIN_BLOCK_2[idx];
            if(p >= POW10_OFFSET_2[idx + 1])
            {
                // If the remaining digits are all 0, then we might as well use memset.
                // No rounding required in this case.
                const uint32_t fill = precision - 9 * i;
                memset(result + index, '0', fill);
                index += fill;
                break;
            }

            // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or
            // above, which is a slightly faster code path in mulShift_mod1e9. Instead, we can
            // just increase the multipliers.
            uint32_t digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT_2[p], j + 8);

            if(i < blocks - 1)
            {
                append_nine_digits(digits, result + index);
                index += 9;
            }
            else
            {
                const uint32_t maximum   = precision - 9 * i;
                uint32_t       lastDigit = 0;
                for(uint32_t k = 0; k < 9 - maximum; ++k)
                {
                    lastDigit = digits % 10;
                    digits /= 10;
                }

                if(lastDigit != 5)
                {
                    roundUp = lastDigit > 5;
                }
                else
                {
                    // Is m * 10^(additionalDigits + 1) / 2^(-e2) integer?
                    const int32_t requiredTwos = -e2 - (int32_t)precision - 1;
                    const bool    trailingZeros =
                        requiredTwos <= 0
                        || (requiredTwos < 60
                            && multipleOfPowerOf2(m2, (uint32_t)requiredTwos));
                    roundUp = trailingZeros ? 2 : 1;
                }
                if(maximum > 0)
                {
                    append_c_digits(maximum, digits, result + index);
                    index += maximum;
                }
                break;
            }
        }

        if(roundUp != 0)
        {
            int roundIndex = index;
            int dotIndex   = 0; // '.' can't be located at index 0
            while(true)
            {
                --roundIndex;
                char c;
                if(roundIndex == -1 || (c = result[roundIndex], c == '-'))
                {
                    result[roundIndex + 1] = '1';
                    if(dotIndex > 0)
                    {
                        result[dotIndex]     = '0';
                        result[dotIndex + 1] = '.';
                    }
                    result[index++] = '0';
                    break;
                }
                if(c == '.')
                {
                    dotIndex = roundIndex;
                    continue;
                }
                else if(c == '9')
                {
                    result[roundIndex] = '0';
                    roundUp            = 1;
                    continue;
                }
                else
                {
                    if(roundUp == 2 && c % 2 == 0)
                    {
                        break;
                    }
                    result[roundIndex] = c + 1;
                    break;
                }
            }
        }
    }
    else
    {
        memset(result + index, '0', precision);
        index += precision;
    }
    return index;
}
