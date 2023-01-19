INTERNAL void
os_print(String8 to_print)
{
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if(stdout_handle)
    {
        DWORD written;
        ASSERT_FUNCTION(
            WriteConsoleA(stdout_handle, to_print.data, to_print.length, &written, NULL));
        ASSERT(written == to_print.length);
    }

    OutputDebugStringA(to_print.data);
}
