#define OS_PRINTF_LITERAL(literal_format, ...)                                               \
    os_printf(STRING8_LITERAL(literal_format), __VA_ARGS__)

#define OS_PRINT_LITERAL(literal_to_print) os_print(STRING8_LITERAL(literal_to_print))

// ===========================================================================================

INTERNAL void os_print(String8 to_print);

// ===========================================================================================

#ifdef DEVELOPMENT

INTERNAL void
os_printf(String8 format, ...)
{
    GET_formated_AND_formated_length_FROM_FORMAT_STRING8(format);
    os_print((String8) {formated, formated_length});
}

#endif // DEVELOPMENT
