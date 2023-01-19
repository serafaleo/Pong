#define TARGET_ASPECT_RATIO_NUMERATOR   16.0f
#define TARGET_ASPECT_RATIO_DENOMINATOR 9.0f

#define TARGET_ASPECT_RATIO (TARGET_ASPECT_RATIO_NUMERATOR / TARGET_ASPECT_RATIO_DENOMINATOR)

#define COLOR(r, g, b) ((Color) {r, g, b})

// ===========================================================================================

typedef struct
{
    f32 r, g, b;

} Color;

typedef struct
{
    s32 x, y;
    s32 width;
    s32 height;

} PixelRect;

GLOBAL struct
{
    void *pixels;
    s32   pixels_count;
    s32   width;
    s32   height;
    f32   aspect_ratio;

} g_back_buffer = {0};

// ===========================================================================================

INTERNAL u32
color_to_u32(Color color)
{
    s32 r = round_f32_to_s32_up(color.r * 255.0f);
    s32 g = round_f32_to_s32_up(color.g * 255.0f);
    s32 b = round_f32_to_s32_up(color.b * 255.0f);

    if(r < 0)
    {
        r = 0;
    }

    if(g < 0)
    {
        g = 0;
    }

    if(b < 0)
    {
        b = 0;
    }

    if(r > 255)
    {
        r = 255;
    }

    if(g > 255)
    {
        g = 255;
    }

    if(b > 255)
    {
        b = 255;
    }

    return (u32)(b | (g << 8) | (r << 16));
}

INTERNAL void
clear_back_buffer(Color color)
{
    u32 color_u32 = color_to_u32(color);

#ifdef OPTIMIZATIONS_ON
    // NOTE(leo): On non-optimized builds (-Od), this code is faster than the 64-bit and
    // 128-bit version bellow.
    u32 *pixel = (u32 *)g_back_buffer.pixels;

    for(s32 i = 0; i < g_back_buffer.pixels_count; ++i)
    {
        *pixel++ = color_u32;
    }

#else
    u64 color_u64 = ((u64)color_u32 << 32) | color_u32;

    u128 *pixels = (u128 *)g_back_buffer.pixels;
    u128  colors = ((u128)color_u64 << 64) | color_u64;

    for(s32 i = 0; i < g_back_buffer.pixels_count / 4; ++i)
    {
        *pixels++ = colors;
    }

    u32 *pixel = (u32 *)pixels;

    // NOTE(leo): Filling the remaining pixels in case of an odd pixels_count.
    for(s32 i = 0; i < g_back_buffer.pixels_count % 4; ++i)
    {
        *pixel++ = color_u32;
    }

#endif // OPTIMIZATIONS_ON
}

INTERNAL void
draw_rectangle_in_pixels(s32 x, s32 y, s32 rect_width, s32 rect_height, Color color)
{
    u32 color_u32 = color_to_u32(color);

    if(x < 0)
    {
        rect_width += x; // NOTE(leo): Here we are reducing rect_width, not increasing.
        x = 0;
    }

    if(y < 0)
    {
        rect_height += y; // NOTE(leo): Here we are reducing rect_height, not increasing.
        y = 0;
    }

    s32 rect_right  = x + rect_width;
    s32 rect_bottom = y + rect_height;

    if(rect_right > g_back_buffer.width)
    {
        rect_width -= rect_right - g_back_buffer.width;
    }

    if(rect_bottom > g_back_buffer.height)
    {
        rect_height -= rect_bottom - g_back_buffer.height;
    }

    if(rect_width > 0 && rect_height > 0)
    {
        s32  goto_next_line = g_back_buffer.width - rect_width;
        u32 *pixel          = (u32 *)g_back_buffer.pixels + x + (g_back_buffer.width * y);

        for(s32 h = 0; h < rect_height; ++h)
        {
            for(s32 w = 0; w < rect_width; ++w)
            {
                *pixel++ = color_u32;
            }

            pixel += goto_next_line;
        }
    }
}

INTERNAL PixelRect
draw_rectangle(f32   rect_center_x,
               f32   rect_center_y,
               f32   rect_width,
               f32   rect_height,
               Color color)
{
    f32 rect_width_px = rect_width * ((f32)g_back_buffer.width / 2.0f);
    f32 rect_height_px =
        rect_height * ((f32)g_back_buffer.height / 2.0f) * g_back_buffer.aspect_ratio;

    f32 rect_center_x_px = ((rect_center_x + 1.0f) * (f32)g_back_buffer.width) / 2.0f;
    f32 rect_center_y_px = ((-rect_center_y + 1.0f) * (f32)g_back_buffer.height) / 2.0f;

    s32 x = round_f32_to_s32_up(rect_center_x_px - (rect_width_px / 2.0f));
    s32 y = round_f32_to_s32_up(rect_center_y_px - (rect_height_px / 2.0f));
    s32 w = round_f32_to_s32_up(rect_width_px);
    s32 h = round_f32_to_s32_up(rect_height_px);

    draw_rectangle_in_pixels(x, y, w, h, color);

    return (PixelRect) {x, y, w, h};
}
