#ifndef __clang__
// NOTE(leo): We are using some Clang-only stuff like __uint128, so it's better off not to
// bother with trying to make it compile in another compiler like MSVC's.
    #error This code should only be compiled with Clang.
#endif // __clang__

#ifndef __x86_64__
// NOTE(leo): Since we are not linking with the Windows CRT (C Runtime Library), there's a lot
// to implement if we wish to be able to compile for 32-bit. Again, it's better off not to
// bother.
    #error This code should only be compiled for x64.
#endif // __x86_64__

// ===========================================================================================

// NOTE(leo): Forward declaring the replaced CRT functions.
void *memset(void *dest_buffer, int value_to_set_per_byte, size_t num_of_bytes_to_set);
void *memcpy(void *dest_buffer, void const *src_buffer, size_t num_of_bytes_to_copy);

#include "../game_main.c"

// ===========================================================================================

// NOTE(leo): Setting the minimum supported Windows version to 7.
#define WINVER       0x0601
#define _WIN32_WINNT WINVER

#define NOGDICAPMASKS    // CC_ *, LC_ *, PC_ *, CP_ *, TC_ *, RC_
// #define NOVIRTUALKEYCODES // VK_ *
// #define NOWINMESSAGES     // WM_ *, EM_ *, LB_ *, CB_ *
// #define NOWINSTYLES       // WS_ *, CS_ *, ES_ *, LBS_ *, SBS_ *, CBS_ *
// #define NOSYSMETRICS     // SM_ *
#define NOMENUS          // MF_ *
#define NOICONS          // IDI_ *
#define NOKEYSTATES      // MK_ *
#define NOSYSCOMMANDS    // SC_ *
// #define NORASTEROPS      // Binary and Tertiary raster ops
// #define NOSHOWWINDOW     // SW_ *
#define OEMRESOURCE      // OEM Resource values
#define NOATOM           // Atom Manager routines
#define NOCLIPBOARD      // Clipboard routines
#define NOCOLOR          // Screen colors
#define NOCTLMGR         // Control and Dialog routines
#define NODRAWTEXT       // DrawText() and DT_ *
// #define NOGDI            // All GDI defines and routines
#define NOKERNEL         // All KERNEL defines and routines
// #define NOUSER            // All USER defines and routines
#define NONLS            // All NLS defines and routines
#define NOMB             // MB_ *and MessageBox()
#define NOMEMMGR         // GMEM_ *, LMEM_ *, GHND, LHND, associated routines
#define NOMETAFILE       // typedef METAFILEPICT
#define NOMINMAX         // Macros min(a, b) and max(a, b)
// #define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE       // OpenFile(), OemToAnsi, AnsiToOem, and OF_ *
#define NOSCROLL         // SB_ *and scrolling routines
#define NOSERVICE        // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND          // Sound driver routines
#define NOTEXTMETRIC     // typedef TEXTMETRIC and associated routines
#define NOWH             // SetWindowsHook and WH_ *
#define NOWINOFFSETS     // GWL_ *, GCL_ *, associated routines
#define NOCOMM           // COMM driver routines
#define NOKANJI          // Kanji support stuff.
#define NOHELP           // Help engine interface.
#define NOPROFILER       // Profiler interface.
#define NODEFERWINDOWPOS // DeferWindowPos routines
#define NOMCX            // Modem Configuration Extensions

#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <Windows.h>

#define COM_NO_WINDOWS_H
#define COBJMACROS

#include <timeapi.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

// ===========================================================================================

// NOTE(leo): Since it seams that initguid.h is broken for WASAPI, we have to look into the
// header files to get the GUIDs manually and be able to use the library in plain C.

// NOTE(leo): The following GUIDs were gotten from mmdeviceapi.h or Audioclient.h

// BCDE0395-E52F-467C-8E3D-C4579291692E
INTERNAL const CLSID CLSID_MMDeviceEnumerator = {
    0xBCDE0395,
    0xE52F,
    0x467C,
    {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E}
};

// A95664D2-9614-4F35-A746-DE8DB63617E6
INTERNAL const IID IID_IMMDeviceEnumerator = {
    0xA95664D2,
    0x9614,
    0x4F35,
    {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6}
};

// 1CB9AD4C-DBFA-4C32-B178-C2F568A703B2
INTERNAL const IID IID_IAudioClient = {
    0x1CB9AD4C,
    0xDBFA,
    0x4C32,
    {0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2}
};

INTERNAL const IID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {
    0x00000003,
    0x0000,
    0x0010,
    {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}
};

// ===========================================================================================

#ifdef DEVELOPMENT
    #include "win32_os.c"
#endif // DEVELOPMENT

GLOBAL f32 g_cpu_ticks_per_second;

// NOTE(leo): This GLOBAL is only global because the way Windows handle window messages
// doesn't allow us to pass parameters. It uses a callback function.
GLOBAL struct
{
    HWND    window_handle;
    HDC     window_dc;
    HBITMAP bitmap_handle;
    HDC     bitmap_dc;
    RECT    fullscreen_rect;
    RECT    windowed_rect;
    s32     blit_dest_x;
    s32     blit_dest_y;
    s32     last_client_width;
    s32     last_client_height;

} g_win32 = {0};

// ===========================================================================================

INTERNAL void
win32_exit(UINT exit_code)
{
    // NOTE(leo): There is no need to call timeEndPeriod since the OS will automatically
    // restore the scheduler as soon as the process ends.

    ExitProcess(exit_code);
}

INTERNAL void
win32_error(void)
{
    win32_exit(1);
}

INTERNAL void
win32_resize_graphics(s32 new_width, s32 new_height)
{
    if((new_width != g_back_buffer.width) || (new_height != g_back_buffer.height))
    {
        BITMAPINFO bitmap_info = {0};

        bitmap_info.bmiHeader.biSize        = sizeof(bitmap_info.bmiHeader);
        bitmap_info.bmiHeader.biWidth       = new_width;
        bitmap_info.bmiHeader.biHeight      = -new_height; // NOTE(leo): Negative for top-down
        bitmap_info.bmiHeader.biPlanes      = 1;
        bitmap_info.bmiHeader.biBitCount    = 32;
        bitmap_info.bmiHeader.biCompression = BI_RGB;
        // NOTE(leo): There are more members that are beeing set to 0 and NULL.

        HDC temp_bitmap_dc = NULL;

        void   *temp_pixels;
        HBITMAP temp_bitmap = CreateDIBSection(g_win32.window_dc,
                                               &bitmap_info,
                                               DIB_RGB_COLORS,
                                               &temp_pixels,
                                               NULL,
                                               0);
        if(!temp_bitmap)
        {
            win32_error();
        }

        temp_bitmap_dc = CreateCompatibleDC(g_win32.window_dc);

        if(!temp_bitmap_dc)
        {
            win32_error();
        }

        if(!SelectObject(temp_bitmap_dc, temp_bitmap))
        {
            win32_error();
        }

        if(g_win32.bitmap_handle)
        {
            ASSERT_FUNCTION(DeleteObject(g_win32.bitmap_handle));
        }

        if(g_win32.bitmap_dc)
        {
            ASSERT_FUNCTION(DeleteDC(g_win32.bitmap_dc));
        }

        g_win32.bitmap_handle = temp_bitmap;
        g_win32.bitmap_dc     = temp_bitmap_dc;
        g_back_buffer.pixels  = temp_pixels;

        g_back_buffer.width        = new_width;
        g_back_buffer.height       = new_height;
        g_back_buffer.pixels_count = new_width * new_height;
        g_back_buffer.aspect_ratio = (f32)new_width / (f32)new_height;

        // NOTE(leo): 0.03f is an epsilon.
        ASSERT((g_back_buffer.aspect_ratio >= (TARGET_ASPECT_RATIO - 0.03f))
               && (g_back_buffer.aspect_ratio <= (TARGET_ASPECT_RATIO + 0.03f)));
    }
}

INTERNAL void
win32_toggle_fullscreen(void)
{
    RECT current_rect;
    if(GetWindowRect(g_win32.window_handle, &current_rect))
    {
        int x, y, w, h;

        if((current_rect.left == g_win32.fullscreen_rect.left)
           && (current_rect.top == g_win32.fullscreen_rect.top)
           && (current_rect.right == g_win32.fullscreen_rect.right)
           && (current_rect.left == g_win32.fullscreen_rect.left))
        {
            x = g_win32.windowed_rect.left;
            y = g_win32.windowed_rect.top;
            w = g_win32.windowed_rect.right - x;
            h = g_win32.windowed_rect.bottom - y;
        }
        else
        {
            x = g_win32.fullscreen_rect.left;
            y = g_win32.fullscreen_rect.top;
            w = g_win32.fullscreen_rect.right - x;
            h = g_win32.fullscreen_rect.bottom - y;
        }

        if(MoveWindow(g_win32.window_handle, x, y, w, h, true))
        {
            g_win32.windowed_rect = current_rect;
        }
    }
}

INTERNAL LRESULT CALLBACK
win32_window_callback(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;

    switch(message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            win32_exit(0);
            break;
        }
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *minmax_info = (MINMAXINFO *)l_param;
            // NOTE(leo): It seems that only the ptMaxTrackSize.y is less than the required
            // window size for a fullscreen window, and it also seems that it is always 19
            // less, so we can just set it to be 19 more than the current maximum, but let's
            // keep an eye on it.
            minmax_info->ptMaxTrackSize.y = GetSystemMetrics(SM_CYMAXTRACK) + 19;
            break;
        }
        case WM_SIZE:
        {
            s32 client_width  = LOWORD(l_param);
            s32 client_height = HIWORD(l_param);

            if((client_width && client_height)
               && ((client_width != g_win32.last_client_width)
                   || (client_height != g_win32.last_client_height)))
            {
                f32 aspect_ratio = (f32)client_width / (f32)client_height;

                s32 width_to_render  = client_width;
                s32 height_to_render = client_height;

                g_win32.blit_dest_x = 0;
                g_win32.blit_dest_y = 0;

                if(aspect_ratio > TARGET_ASPECT_RATIO)
                {
                    width_to_render = round_f32_to_s32_up(
                        ((f32)client_height * TARGET_ASPECT_RATIO_NUMERATOR)
                        / TARGET_ASPECT_RATIO_DENOMINATOR);
                    g_win32.blit_dest_x = round_f32_to_s32_up(
                        ((f32)client_width - (f32)width_to_render) / 2.0f);
                }
                else if(aspect_ratio < TARGET_ASPECT_RATIO)
                {
                    height_to_render = round_f32_to_s32_up(
                        ((f32)client_width * TARGET_ASPECT_RATIO_DENOMINATOR)
                        / TARGET_ASPECT_RATIO_NUMERATOR);
                    g_win32.blit_dest_y = round_f32_to_s32_up(
                        ((f32)client_height - (f32)height_to_render) / 2.0f);
                }

                win32_resize_graphics(width_to_render, height_to_render);

                g_win32.last_client_width  = client_width;
                g_win32.last_client_height = client_height;
            }

            break;
        }

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            u32 vk_code     = safe_cast_u64_to_u32(w_param);
            b32 is_down     = !GET_BIT(l_param, 31);
            b32 was_down    = GET_BIT(l_param, 30);
            b32 alt_is_down = GET_BIT(l_param, 29);

            if(((vk_code == VK_F11) || (vk_code == VK_RETURN && alt_is_down)) && is_down)
            {
                win32_toggle_fullscreen();
            }
            else if(vk_code == 'W')
            {
                g_is_key_down[KEY_W] = is_down;
            }
            else if(vk_code == 'S')
            {
                g_is_key_down[KEY_S] = is_down;
            }
            else if(vk_code == VK_UP)
            {
                g_is_key_down[KEY_UP] = is_down;
            }
            else if(vk_code == VK_DOWN)
            {
                g_is_key_down[KEY_DOWN] = is_down;
            }
            else if(vk_code == VK_RETURN)
            {
                g_is_key_down[KEY_ENTER] = is_down;
            }
#define KEY_UP(key) (vk_code == (key) && !is_down && was_down)
            else if((KEY_UP(VK_F4) && alt_is_down) || KEY_UP(VK_ESCAPE))
            {
                win32_exit(0);
            }
#undef KEY_UP

            break;
        }
        case WM_MOUSEMOVE:
        {
            SetCursor(NULL);
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC         device_context = BeginPaint(window_handle, &paint);
            int         x              = paint.rcPaint.left;
            int         y              = paint.rcPaint.top;
            int         w              = paint.rcPaint.right - x;
            int         h              = paint.rcPaint.bottom - y;

            ASSERT_FUNCTION(PatBlt(device_context, x, y, w, h, BLACKNESS));
            ASSERT_FUNCTION(BitBlt(g_win32.window_dc,
                                   g_win32.blit_dest_x,
                                   g_win32.blit_dest_y,
                                   g_back_buffer.width,
                                   g_back_buffer.height,
                                   g_win32.bitmap_dc,
                                   0,
                                   0,
                                   SRCCOPY));

            EndPaint(window_handle, &paint);
            break;
        }
        default:
        {
            result = DefWindowProcA(window_handle, message, w_param, l_param);
            break;
        }
    }

    return result;
}

INTERNAL void
win32_create_window(void)
{
    HINSTANCE executable_instance = GetModuleHandleA(NULL);

    if(!executable_instance)
    {
        win32_error();
    }

    WNDCLASSEXA window_class = {0};

    window_class.cbSize      = sizeof(window_class);
    window_class.style       = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_window_callback;
    window_class.hInstance   = executable_instance;
    // TODO(leo): window_class.hIcon;
    window_class.lpszClassName = PROGRAM_NAME " Window Class";
    // TODO(leo): window_class.hIconSm;

    if(!RegisterClassExA(&window_class))
    {
        win32_error();
    }

    // NOTE(leo): Width and height of the primary screen;
    int screen_width_px  = GetSystemMetrics(SM_CXSCREEN);
    int screen_height_px = GetSystemMetrics(SM_CYSCREEN);

    if(!screen_width_px || !screen_height_px)
    {
        win32_error();
    }

    s32 windowed_init_width  = 1280;
    s32 windowed_init_height = 720;

    // NOTE(leo): Centralizing the window in the screen.
    g_win32.windowed_rect.left   = (screen_width_px / 2) - (windowed_init_width / 2);
    g_win32.windowed_rect.top    = (screen_height_px / 2) - (windowed_init_height / 2);
    g_win32.windowed_rect.right  = g_win32.windowed_rect.left + windowed_init_width;
    g_win32.windowed_rect.bottom = g_win32.windowed_rect.top + windowed_init_height;

    g_win32.fullscreen_rect.left   = 0;
    g_win32.fullscreen_rect.top    = 0;
    g_win32.fullscreen_rect.right  = screen_width_px;
    g_win32.fullscreen_rect.bottom = screen_height_px;

    DWORD style = WS_OVERLAPPEDWINDOW;
    // DWORD style = WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;

    if(!AdjustWindowRectEx(&g_win32.windowed_rect, style, false, 0)
       || !AdjustWindowRectEx(&g_win32.fullscreen_rect, style, false, 0))
    {
        win32_error();
    }

#ifdef DEVELOPMENT
    int x = g_win32.windowed_rect.left;
    int y = g_win32.windowed_rect.top;
    int w = g_win32.windowed_rect.right - x;
    int h = g_win32.windowed_rect.bottom - y;
#else
    int x = g_win32.fullscreen_rect.left;
    int y = g_win32.fullscreen_rect.top;
    int w = g_win32.fullscreen_rect.right - x;
    int h = g_win32.fullscreen_rect.bottom - y;
#endif // DEVELOPMENT

    g_win32.window_handle = CreateWindowExA(0,
                                            window_class.lpszClassName,
                                            PROGRAM_NAME,
                                            style,
                                            x,
                                            y,
                                            w,
                                            h,
                                            NULL,
                                            NULL,
                                            executable_instance,
                                            NULL);
    if(!g_win32.window_handle)
    {
        win32_error();
    }

    g_win32.window_dc = GetDC(g_win32.window_handle);

    if(!g_win32.window_dc)
    {
        win32_error();
    }
}

INTERNAL f32
win32_get_monitor_refresh_rate(HWND window_handle)
{
    HMONITOR window_monitor = MonitorFromWindow(window_handle, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFOEXA monitor_info;
    monitor_info.cbSize = sizeof(monitor_info);
    if(!GetMonitorInfoA(window_monitor, (MONITORINFO *)&monitor_info))
    {
        win32_error();
    }

    DEVMODEA display_mode;
    if(!EnumDisplaySettingsA(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &display_mode))
    {
        win32_error();
    }

    return (f32)display_mode.dmDisplayFrequency;
}

INTERNAL s64
win32_get_cpu_tick(void)
{
    LARGE_INTEGER li_counter;
    // NOTE(leo): According to MSDN, will never fail on Windows XP or later. We only ASSERT.
    ASSERT_FUNCTION(QueryPerformanceCounter(&li_counter));
    return li_counter.QuadPart;
}

INTERNAL f32
win32_get_seconds_elapsed(s64 init_tick, s64 end_tick)
{
    s64 delta = end_tick - init_tick;
    ASSERT(delta > 0);
    return (f32)delta / g_cpu_ticks_per_second;
}

INTERNAL void
win32_init_sound_system(void)
{
    HRESULT result;
    if(FAILED(result = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY)))
    {
        win32_error();
    }

    IMMDeviceEnumerator *device_enumerator;
    if(FAILED(result = CoCreateInstance(&CLSID_MMDeviceEnumerator,
                                        NULL,
                                        CLSCTX_ALL,
                                        &IID_IMMDeviceEnumerator,
                                        (void **)&device_enumerator)))
    {
        win32_error();
    }

    IMMDevice *default_output_endpoint;
    result = IMMDeviceEnumerator_GetDefaultAudioEndpoint(device_enumerator,
                                                         eRender,
                                                         eConsole,
                                                         &default_output_endpoint);
    IMMDeviceEnumerator_Release(device_enumerator);

    if(FAILED(result))
    {
        win32_error();
    }

    IAudioClient *client;
    result = IMMDevice_Activate(default_output_endpoint,
                                &IID_IAudioClient,
                                CLSCTX_ALL,
                                NULL,
                                (void **)&client);

    IMMDevice_Release(default_output_endpoint);

    if(FAILED(result))
    {
        win32_error();
    }

    WAVEFORMATEXTENSIBLE shared_mode_format;
    shared_mode_format.Format.wFormatTag     = WAVE_FORMAT_EXTENSIBLE;
    shared_mode_format.Format.nChannels      = 2;
    shared_mode_format.Format.nSamplesPerSec = 48000;
    shared_mode_format.Format.wBitsPerSample = 8 * sizeof(f32);

    shared_mode_format.Format.nBlockAlign =
        (shared_mode_format.Format.nChannels * shared_mode_format.Format.wBitsPerSample) / 8;

    shared_mode_format.Format.nAvgBytesPerSec =
        shared_mode_format.Format.nSamplesPerSec * shared_mode_format.Format.nBlockAlign;

    shared_mode_format.Format.cbSize =
        sizeof(shared_mode_format) - sizeof(shared_mode_format.Format);

    shared_mode_format.Samples.wValidBitsPerSample = shared_mode_format.Format.wBitsPerSample;
    shared_mode_format.dwChannelMask               = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    shared_mode_format.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

    WAVEFORMATEX *closest_match;
    result = IAudioClient_IsFormatSupported(client,
                                            AUDCLNT_SHAREMODE_SHARED,
                                            &shared_mode_format.Format,
                                            &closest_match);
    if(result != S_OK)
    {
        // TODO(leo): The audio engine (for shared mode stream) does not support one or more
        // of the following format properties: 32-bit floats, 48000 samples per second, 2
        // channels.
        win32_error();
    }
}

void
WinMainCRTStartup(void)
{
    // NOTE(leo): This is to prevent error mode dialogs from hanging the application.
    SetErrorMode(SEM_FAILCRITICALERRORS);

    LARGE_INTEGER li_frequency;
    if(!QueryPerformanceFrequency(&li_frequency))
    {
        // NOTE(leo): According to MSDN, will never fail on Windows XP or later.
        win32_error();
    }

    g_cpu_ticks_per_second = (f32)li_frequency.QuadPart;

    win32_create_window();
    win32_init_sound_system();

    f32 refresh_rate         = win32_get_monitor_refresh_rate(g_win32.window_handle);
    f32 target_frame_seconds = 1.0f / refresh_rate;

    ASSERT_FUNCTION_COMPARE(timeBeginPeriod(1), ==, TIMERR_NOERROR);

    GameState game_state;
    game_state_init(&game_state);

    // NOTE(leo): Setting to an aproximate value for the first frame.
    f32 last_frame_time_seconds = target_frame_seconds;

    ShowWindow(g_win32.window_handle, SW_SHOW);
    SetFocus(g_win32.window_handle);
    SetCursor(NULL);

    while(1)
    {
        s64 frame_begin_tick = win32_get_cpu_tick();

        MSG msg;
        while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        OS_PRINTF_LITERAL("Frame time (ms): %.2f (target: %.2f)\n",
                          last_frame_time_seconds * 1000.0f,
                          target_frame_seconds * 1000.0f);

        game_update_and_render(&game_state, last_frame_time_seconds);

        f32 frame_work_seconds =
            win32_get_seconds_elapsed(frame_begin_tick, win32_get_cpu_tick());

        s32 ms_to_sleep = (s32)((target_frame_seconds - frame_work_seconds) * 1000.0f);

        OS_PRINTF_LITERAL("Frame work (ms): %.2f\n", frame_work_seconds * 1000.0f);

        s32 fine_tuning = 1;
        if(ms_to_sleep > fine_tuning)
        {
            // NOTE(leo): THIS IS NOT V-SYNC!!! This is only to aproximate the frame
            // duration to the duration of a v-blank so that we can get a more smooth
            // gameplay by reducing number of v-blanks missed. We are subtraction 1 because
            // it seams that this way we can get closer to the actual value of
            // target_frame_seconds. For that we also have to check whether it is greater
            // than this fine_tuning value.
            Sleep((DWORD)(ms_to_sleep - fine_tuning));
        }

        ASSERT_FUNCTION(BitBlt(g_win32.window_dc,
                               g_win32.blit_dest_x,
                               g_win32.blit_dest_y,
                               g_back_buffer.width,
                               g_back_buffer.height,
                               g_win32.bitmap_dc,
                               0,
                               0,
                               SRCCOPY));

        last_frame_time_seconds =
            win32_get_seconds_elapsed(frame_begin_tick, win32_get_cpu_tick());
    }

    // NOTE(leo): Never gets here.
}

// NOTE(leo): CRT replacement stuff.
// ===========================================================================================

int _fltused = 0x9875;

void *
memset(void *dest_buffer, int value_to_set_per_byte, size_t num_of_bytes_to_set)
{
#ifdef OPTIMIZATIONS_ON
    // NOTE(leo): Byte by byte seems to be faster than 128-bit by 128-bit when using -O3.

    u8 *byte_to_set = dest_buffer;

    while(num_of_bytes_to_set--)
    {
        *byte_to_set++ = (u8)value_to_set_per_byte;
    }

    return byte_to_set;

#else
    // NOTE(leo): 128-bit by 128-bit seems to be faster than byte by byte when using -O0.

    u8    value_u8     = (u8)value_to_set_per_byte;
    u128 *destination  = dest_buffer;
    u128  value_to_set = value_u8;

    for(size_t byte_index = 1; byte_index < sizeof(*destination); ++byte_index)
    {
        value_to_set |= ((u128)value_u8 << (byte_index * 8));
    }

    u64 chuncks_to_set = num_of_bytes_to_set / sizeof(*destination);

    while(chuncks_to_set--)
    {
        *destination++ = value_to_set;
    }

    u32 missing_bytes_to_set = num_of_bytes_to_set % sizeof(*destination);
    u8 *missing              = (u8 *)destination;

    while(missing_bytes_to_set--)
    {
        *missing++ = (u8)value_to_set_per_byte;
    }

    return missing;

#endif // OPTIMIZATIONS_ON
}

void *
memcpy(void *dest_buffer, void const *src_buffer, size_t num_of_bytes_to_copy)
{
#ifdef OPTIMIZATIONS_ON
    // NOTE(leo): Byte by byte seems to be faster than 128-bit by 128-bit when using -O3.

    u8       *dest_byte = dest_buffer;
    const u8 *src_byte  = src_buffer;

    while(num_of_bytes_to_copy--)
    {
        *dest_byte++ = *src_byte++;
    }

    return dest_byte;

#else
    // NOTE(leo): 128-bit by 128-bit seems to be faster than byte by byte when using -O0.

    u128       *destination     = dest_buffer;
    const u128 *source          = (const u128 *)src_buffer;
    u64         chuncks_to_copy = num_of_bytes_to_copy / sizeof(*destination);

    while(chuncks_to_copy--)
    {
        *destination++ = *source++;
    }

    u32       remaining_bytes_to_copy = num_of_bytes_to_copy % sizeof(*destination);
    u8       *missing                 = (u8 *)destination;
    const u8 *remaining               = (const u8 *)source;

    while(remaining_bytes_to_copy--)
    {
        *missing++ = *remaining++;
    }

    return missing;

#endif // OPTIMIZATIONS_ON
}
