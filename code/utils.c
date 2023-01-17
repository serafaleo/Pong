#define INTERNAL   static
#define GLOBAL     static
#define PERSISTENT static

typedef char       s8;
typedef short      s16;
typedef int        s32;
typedef long long  s64;
typedef __int128_t s128;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef __uint128_t        u128;

typedef bool b8;
typedef u32  b32;

typedef float  f32;
typedef double f64;

#define S8_MIN  INT8_MIN
#define S8_MAX  INT8_MAX
#define S16_MIN INT16_MIN
#define S16_MAX INT16_MAX
#define S32_MIN INT32_MIN
#define S32_MAX INT32_MAX
#define S64_MIN INT64_MIN
#define S64_MAX INT64_MAX

#define U8_MAX  UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

// clang-format off
#ifdef ASSERTIONS_ON
    #define INVALID_CODE_PATH __builtin_trap()
    #define ASSERT(expression) do { if(!(expression)) { INVALID_CODE_PATH; } } while(0)
    #define ASSERT_FUNCTION(function_call) ASSERT(function_call)
    #define ASSERT_FUNCTION_COMPARE(call, operator, value) ASSERT((call) operator (value))
#else
    #define INVALID_CODE_PATH
    #define ASSERT(expression)
    #define ASSERT_FUNCTION(function_call) function_call
    #define ASSERT_FUNCTION_COMPARE(call, operator, value) call
#endif // ASSERTIONS_ON
// clang-format on

#define GET_BIT(variable, bit_index) (((variable) >> (bit_index)) & 1)
#define STATIC_ARRAY_LENGTH(array)   (sizeof((array)) / sizeof(*(array)))

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

INTERNAL u32
safe_cast_u64_to_u32(u64 to_cast)
{
    ASSERT(to_cast <= U32_MAX);
    return (u32)to_cast;
}

INTERNAL u16
safe_cast_u64_to_u16(u64 to_cast)
{
    ASSERT(to_cast <= U16_MAX);
    return (u16)to_cast;
}

INTERNAL u8
safe_cast_u64_to_u8(u64 to_cast)
{
    ASSERT(to_cast <= U8_MAX);
    return (u8)to_cast;
}

INTERNAL s64
safe_cast_u64_to_s64(u64 to_cast)
{
    ASSERT(to_cast <= S64_MAX);
    return (s64)to_cast;
}

INTERNAL s32
safe_cast_u64_to_s32(u64 to_cast)
{
    ASSERT(to_cast <= S32_MAX);
    return (s32)to_cast;
}

INTERNAL s16
safe_cast_u64_to_s16(u64 to_cast)
{
    ASSERT(to_cast <= S16_MAX);
    return (s16)to_cast;
}

INTERNAL s8
safe_cast_u64_to_s8(u64 to_cast)
{
    ASSERT(to_cast <= S8_MAX);
    return (s8)to_cast;
}

INTERNAL u16
safe_cast_u32_to_u16(u32 to_cast)
{
    ASSERT(to_cast <= U16_MAX);
    return (u16)to_cast;
}

INTERNAL u8
safe_cast_u32_to_u8(u32 to_cast)
{
    ASSERT(to_cast <= U8_MAX);
    return (u8)to_cast;
}

INTERNAL s32
safe_cast_u32_to_s32(u32 to_cast)
{
    ASSERT(to_cast <= S32_MAX);
    return (s32)to_cast;
}

INTERNAL s16
safe_cast_u32_to_s16(u32 to_cast)
{
    ASSERT(to_cast <= S16_MAX);
    return (s16)to_cast;
}

INTERNAL s8
safe_cast_u32_to_s8(u32 to_cast)
{
    ASSERT(to_cast <= S8_MAX);
    return (s8)to_cast;
}

INTERNAL u8
safe_cast_u16_to_u8(u16 to_cast)
{
    ASSERT(to_cast <= U8_MAX);
    return (u8)to_cast;
}

INTERNAL s16
safe_cast_u16_to_s16(u16 to_cast)
{
    ASSERT(to_cast <= S16_MAX);
    return (s16)to_cast;
}

INTERNAL s8
safe_cast_u8_to_s8(u8 to_cast)
{
    ASSERT(to_cast <= S8_MAX);
    return (s8)to_cast;
}

INTERNAL u64
safe_cast_s64_to_u64(s64 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u64)to_cast;
}

INTERNAL u32
safe_cast_s64_to_u32(s64 to_cast)
{
    ASSERT(to_cast >= 0);
    ASSERT(to_cast <= U32_MAX);
    return (u32)to_cast;
}

INTERNAL u16
safe_cast_s64_to_u16(s64 to_cast)
{
    ASSERT(to_cast >= 0);
    ASSERT(to_cast <= U16_MAX);
    return (u16)to_cast;
}

INTERNAL u8
safe_cast_s64_to_u8(s64 to_cast)
{
    ASSERT(to_cast >= 0);
    ASSERT(to_cast <= U8_MAX);
    return (u8)to_cast;
}

INTERNAL s32
safe_cast_s64_to_s32(s64 to_cast)
{
    ASSERT(to_cast >= S32_MIN);
    ASSERT(to_cast <= S32_MAX);
    return (s32)to_cast;
}

INTERNAL s16
safe_cast_s64_to_s16(s64 to_cast)
{
    ASSERT(to_cast >= S16_MIN);
    ASSERT(to_cast <= S16_MAX);
    return (s16)to_cast;
}

INTERNAL s8
safe_cast_s64_to_s8(s64 to_cast)
{
    ASSERT(to_cast >= S8_MIN);
    ASSERT(to_cast <= S8_MAX);
    return (s8)to_cast;
}

INTERNAL u64
safe_cast_s32_to_u64(s32 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u64)to_cast;
}

INTERNAL u32
safe_cast_s32_to_u32(s32 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u32)to_cast;
}

INTERNAL u16
safe_cast_s32_to_u16(s32 to_cast)
{
    ASSERT(to_cast >= 0);
    ASSERT(to_cast <= U16_MAX);
    return (u16)to_cast;
}

INTERNAL u8
safe_cast_s32_to_u8(s32 to_cast)
{
    ASSERT(to_cast >= 0);
    ASSERT(to_cast <= U8_MAX);
    return (u8)to_cast;
}

INTERNAL s16
safe_cast_s32_to_s16(s32 to_cast)
{
    ASSERT(to_cast >= S16_MIN);
    ASSERT(to_cast <= S16_MAX);
    return (s16)to_cast;
}

INTERNAL s8
safe_cast_s32_to_s8(s32 to_cast)
{
    ASSERT(to_cast >= S8_MIN);
    ASSERT(to_cast <= S8_MAX);
    return (s8)to_cast;
}

INTERNAL u64
safe_cast_s16_to_u64(s16 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u64)to_cast;
}

INTERNAL u32
safe_cast_s16_to_u32(s16 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u32)to_cast;
}

INTERNAL u16
safe_cast_s16_to_u16(s16 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u16)to_cast;
}

INTERNAL u8
safe_cast_s16_to_u8(s16 to_cast)
{
    ASSERT(to_cast >= 0);
    ASSERT(to_cast <= U8_MAX);
    return (u8)to_cast;
}

INTERNAL s8
safe_cast_s16_to_s8(s16 to_cast)
{
    ASSERT(to_cast >= S8_MIN);
    ASSERT(to_cast <= S8_MAX);
    return (s8)to_cast;
}

INTERNAL u64
safe_cast_s8_to_u64(s8 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u64)to_cast;
}

INTERNAL u32
safe_cast_s8_to_u32(s8 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u32)to_cast;
}

INTERNAL u16
safe_cast_s8_to_u16(s8 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u16)to_cast;
}

INTERNAL u8
safe_cast_s8_to_u8(s8 to_cast)
{
    ASSERT(to_cast >= 0);
    return (u8)to_cast;
}

#pragma clang diagnostic pop
