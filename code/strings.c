// NOTE(leo): In case that 'formated' buffer is not large enough for the result string, when
// ASSERTIONS_ON is defined, an INVALID_CODE_PATH will be triggered, making us know that we
// need to increase its size. When ASSERTIONS_ON is not defined, the str8_format_va will
// return the number of characters written to the point it realized that there were not enough
// space. Then we will print this amount of characters anyway.

// clang-format off
#define GET_formated_AND_formated_length_FROM_FORMAT_STRING8(format_string8)                 \
    va_list variable_arguments;                                                              \
    va_start(variable_arguments, format_string8);                                            \
    char formated[1024];                                                                     \
    u32  formated_length = str8_format_va(formated,                                          \
                                          STATIC_ARRAY_LENGTH(formated),                     \
                                          format_string8,                                    \
                                          variable_arguments);                               \
    va_end(variable_arguments);
// clang-format on

#define STRING8_LITERAL(literal) ((String8) {literal, STATIC_ARRAY_LENGTH(literal) - 1})

#define STR8_FORMAT_LITERAL(result_buffer, result_buffer_capacity, literal_format, ...)      \
    str8_format(result_buffer,                                                               \
                result_buffer_capacity,                                                      \
                STRING8_LITERAL(literal_format),                                             \
                __VA_ARGS__)

// ===========================================================================================

typedef struct
{
    char *data;
    u32   length;

} String8;

// ===========================================================================================

INTERNAL u32
str8_format_va(char *result, u64 result_capacity, String8 format, va_list variable_arguments)
{
    u32   chars_written = 0;
    char *character     = format.data;

#define CHECK_BOUNDS(to_write_count)                                                         \
    if(chars_written + to_write_count >= result_capacity)                                    \
    {                                                                                        \
        INVALID_CODE_PATH;                                                                   \
        goto end;                                                                            \
    }

    for(;;)
    {
        if(character >= format.data + format.length)
        {
            break;
        }

        if(*character == '%')
        {
            character++;

            b32   commas_in_thousands       = false;
            char *digits_string             = "0123456789";
            u32   int_base                  = 10;
            b32   float_precision_specified = false;

            // NOTE(leo): MUST be set to zero here, otherwise the while loop bellow will
            // calculate it wrong.
            u32 float_precision = 0;

            if(*character == '\'')
            {
                character++;
                commas_in_thousands = true;
            }
            else if(*character == 'x' || *character == 'X')
            {
                digits_string = *character == 'x' ? "0123456789abcdef" : "0123456789ABCDEF";

                int_base = 16;

                character++;
            }
            else if(*character == '.')
            {
                character++;

                float_precision_specified = true;
                ASSERT(*character >= '0' && *character <= '9');
                while(*character >= '0' && *character <= '9')
                {
                    float_precision *= 10;
                    float_precision += (u32)(*character++ - '0');
                }
            }

            if(!float_precision_specified)
            {
                float_precision = 6;
            }

            switch(*character)
            {
                case 'S': // String8
                {
                    String8 string = *va_arg(variable_arguments, String8 *);
                    CHECK_BOUNDS(string.length);
                    for(u32 char_index = 0; char_index < string.length; ++char_index)
                    {
                        result[chars_written++] = string.data[char_index];
                    }

                    break;
                }
                case 'a': // ansi
                {
                    char *null_term_string = va_arg(variable_arguments, char *);
                    for(; *null_term_string; ++null_term_string)
                    {
                        CHECK_BOUNDS(1);
                        result[chars_written++] = *null_term_string;
                    }

                    break;
                }
                case 'w': // wide ansi
                {
                    wchar_t *null_term_wide_string = va_arg(variable_arguments, wchar_t *);

                    for(; *null_term_wide_string; ++null_term_wide_string)
                    {
                        ASSERT(*((char *)null_term_wide_string + 1) == 0);
                        CHECK_BOUNDS(1);
                        result[chars_written++] = *((char *)null_term_wide_string);
                    }

                    break;
                }
                case 's':
                case 'u':
                {
                    u64 unsigned_value;

                    if(*character++ == 's')
                    {
                        s64 signed_value = 0; // NOTE(leo): Initialized just to shup up Clang.

                        switch(*character)
                        {
                            case '8':
                            {
#ifdef __clang__
                                signed_value = va_arg(variable_arguments, s32);
#else
                                signed_value   = va_arg(variable_arguments, s8);
#endif // __clang__
                                break;
                            }
                            case '1': // 16
                            {
                                character++;
#ifdef __clang__
                                signed_value = va_arg(variable_arguments, s32);
#else
                                signed_value   = va_arg(variable_arguments, s16);
#endif // __clang__
                                break;
                            }
                            case '3': // 32
                            {
                                character++;
                                signed_value = va_arg(variable_arguments, s32);
                                break;
                            }
                            case '6': // 64
                            {
                                character++;
                                signed_value = va_arg(variable_arguments, s64);
                                break;
                            }
                            default:
                            {
                                INVALID_CODE_PATH;
                                break;
                            }
                        }

                        if(signed_value < 0)
                        {
                            signed_value = -signed_value;
                            CHECK_BOUNDS(1);
                            result[chars_written++] = '-';
                        }

                        unsigned_value = (u64)signed_value;
                    }
                    else
                    {
                        switch(*character)
                        {
                            case '8':
                            {
#ifdef __clang__
                                unsigned_value = va_arg(variable_arguments, u32);
#else
                                unsigned_value = va_arg(variable_arguments, u8);
#endif // __clang__
                                break;
                            }
                            case '1': // 16
                            {
                                character++;
#ifdef __clang__
                                unsigned_value = va_arg(variable_arguments, u32);
#else
                                unsigned_value = va_arg(variable_arguments, u16);
#endif // __clang__
                                break;
                            }
                            case '3': // 32
                            {
                                character++;
                                unsigned_value = va_arg(variable_arguments, u32);
                                break;
                            }
                            case '6': // 64
                            {
                                character++;
                                unsigned_value = va_arg(variable_arguments, u64);
                                break;
                            }
                            default:
                            {
                                INVALID_CODE_PATH;
                                break;
                            }
                        }
                    }

                    if(int_base == 16)
                    {
                        CHECK_BOUNDS(2);
                        result[chars_written++] = '0';
                        result[chars_written++] = 'x';
                    }

                    if(unsigned_value == 0)
                    {
                        CHECK_BOUNDS(1);
                        result[chars_written++] = '0';
                    }
                    else
                    {
                        u32 digits_count = 0;
                        u64 temp_value   = unsigned_value;

                        while(temp_value)
                        {
                            digits_count++;
                            temp_value /= int_base;
                        }

                        u32 chars_to_write = digits_count;

                        if(commas_in_thousands)
                        {
                            ASSERT(int_base == 10);
                            chars_to_write += digits_count / 3;
                            if(digits_count % 3 == 0)
                            {
                                chars_to_write--;
                            }
                        }

                        CHECK_BOUNDS(chars_to_write);
                        u32 chars_to_write_temp = chars_to_write;

                        u32 count = 0;
                        while(unsigned_value)
                        {
                            u32 digit_index = unsigned_value % int_base;
                            result[chars_written + --chars_to_write] =
                                digits_string[digit_index];

                            if(++count == 3 && commas_in_thousands)
                            {
                                result[chars_written + --chars_to_write] = ',';

                                count = 0;
                            }

                            unsigned_value /= int_base;
                        }

                        chars_written += chars_to_write_temp;
                    }

                    break;
                }
                case 'f': // TODO(leo): Add a scientific notation format, also from Ryu.
                {
                    f64 float_value  = va_arg(variable_arguments, f64);
                    int temp_written = d2fixed_buffered_n(float_value,
                                                          float_precision,
                                                          result + chars_written);
                    ASSERT(temp_written > 0);
                    // NOTE(leo): Same as CHECK_BOUNDS
                    if(chars_written + (u32)temp_written >= result_capacity)
                    {
                        INVALID_CODE_PATH;
                        // NOTE(leo): Here we can add temp_written to chars_written even
                        // though we pass the bounds, because since the code got here, it
                        // didn't crash, and so why not print the complete float string.
                        chars_written += (u32)temp_written;
                        goto end;
                    }

                    chars_written += (u32)temp_written;
                    break;
                }
                case '%':
                {
                    CHECK_BOUNDS(1);
                    result[chars_written++] = '%';
                    break;
                }
                case 'b':
                {
                    character++;
                    b32 bool_value = 0; // NOTE(leo): Initialized just to shut up Clang.
                    switch(*character)
                    {
                        case '8':
                        {
#ifdef __clang__
                            bool_value = va_arg(variable_arguments, b32);
#else
                            bool_value = va_arg(variable_arguments, b8);
#endif // __clang__
                            break;
                        }
                        case '3': // 32
                        {
                            bool_value = va_arg(variable_arguments, b32);
                            character++;
                            break;
                        }
                        default:
                        {
                            INVALID_CODE_PATH;
                            break;
                        }
                    }

                    String8 bool_str =
                        bool_value ? STRING8_LITERAL("true") : STRING8_LITERAL("false");

                    CHECK_BOUNDS(bool_str.length);
                    for(u32 char_index = 0; char_index < bool_str.length; ++char_index)
                    {
                        result[chars_written++] = bool_str.data[char_index];
                    }

                    break;
                }
                default:
                {
                    // NOTE(leo): Unknown format.
                    INVALID_CODE_PATH;
                    break;
                }
            }
        }
        else
        {
            CHECK_BOUNDS(1);
            result[chars_written++] = *character;
        }

        character++;
    }

#undef CHECK_BOUNDS

end:
    result[chars_written] = '\0';
    return chars_written;
}

INTERNAL u32
str8_format(char *result, u64 result_capacity, String8 format, ...)
{
    va_list variable_arguments;
    va_start(variable_arguments, format);
    u32 formated_length = str8_format_va(result, result_capacity, format, variable_arguments);
    va_end(variable_arguments);
    return formated_length;
}
