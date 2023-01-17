typedef struct
{
    f32 x, y;

} v2;

INTERNAL v2
v2_add(v2 a, v2 b)
{
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

INTERNAL v2
v2_scalar_multiply(v2 v, f32 scalar)
{
    v2 result;
    result.x = v.x * scalar;
    result.y = v.y * scalar;
    return result;
}

INTERNAL s32
round_f32_to_s32_up(f32 to_round)
{
    return (s32)(to_round + 0.5f);
}
