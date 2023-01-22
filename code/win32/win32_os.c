INTERNAL void
os_print(String8 to_print)
{
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if(stdout_handle)
    {
        // NOTE(leo): Just in case we decide to use AllocConsole, but for now, we are good
        // with just the debugger output window.

        DWORD written;
        ASSERT_FUNCTION(
            WriteConsoleA(stdout_handle, to_print.data, to_print.length, &written, NULL));
        ASSERT(written == to_print.length);
    }

    OutputDebugStringA(to_print.data);
}
