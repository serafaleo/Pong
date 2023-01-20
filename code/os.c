#ifdef DEVELOPMENT
    #define OS_PRINTF_LITERAL(literal_format, ...)                                           \
        os_printf(STRING8_LITERAL(literal_format), __VA_ARGS__)

    #define OS_PRINT_LITERAL(literal_to_print) os_print(STRING8_LITERAL(literal_to_print))
#else
    #define OS_PRINTF_LITERAL
    #define OS_PRINT_LITERAL
#endif

// ===========================================================================================

INTERNAL void os_print(String8 to_print);

// ===========================================================================================

INTERNAL void
os_printf(String8 format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    char formated[1024];
    u32  length = str8_format_va(formated, STATIC_ARRAY_LENGTH(formated), format, arguments);
    va_end(arguments);
    os_print((String8) {formated, length});
}
