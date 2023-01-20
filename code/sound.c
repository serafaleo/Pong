#define NUMBER_OF_CHANNELS 2
#define SAMPLES_PER_SECOND 48000
#define BYTES_PER_SAMPLE   (NUMBER_OF_CHANNELS * sizeof(f32))

#define POINT_SOUND_DURATION_SECONDS 0.7f
#define POINT_SOUND_FREQUENCY_HZ     240

#define BALL_HIT_WALL_SOUND_DURATION_SECONDS 0.1f
#define BALL_HIT_WALL_SOUND_FREQUENCY_HZ     POINT_SOUND_FREQUENCY_HZ

#define BALL_HIT_PADDLE_SOUND_DURATION_SECONDS BALL_HIT_WALL_SOUND_DURATION_SECONDS
#define BALL_HIT_PADDLE_SOUND_FREQUENCY_HZ     BALL_HIT_WALL_SOUND_FREQUENCY_HZ * 2

#define VOLUME 0.05f

// ===========================================================================================

typedef struct
{
    u8 *samples;
    u32 sample_count;

} Sound;

GLOBAL Sound g_point_sound;
GLOBAL Sound g_ball_hit_wall_sound;
GLOBAL Sound g_ball_hit_paddle_sound;

GLOBAL u8 *g_samples_to_play;

GLOBAL u32 g_bytes_remaining   = 0;
GLOBAL u32 g_next_byte_to_copy = 0;

// ===========================================================================================

INTERNAL void
generate_game_sounds(void)
{
    Sound *sounds_to_gen[] = {&g_point_sound,
                              &g_ball_hit_wall_sound,
                              &g_ball_hit_paddle_sound};

    f32 durations[] = {POINT_SOUND_DURATION_SECONDS,
                       BALL_HIT_WALL_SOUND_DURATION_SECONDS,
                       BALL_HIT_PADDLE_SOUND_DURATION_SECONDS};

    f32 frequencies[] = {POINT_SOUND_FREQUENCY_HZ,
                         BALL_HIT_WALL_SOUND_FREQUENCY_HZ,
                         BALL_HIT_PADDLE_SOUND_FREQUENCY_HZ};

    for(u64 i = 0; i < STATIC_ARRAY_LENGTH(sounds_to_gen); ++i)
    {
        Sound *sound = sounds_to_gen[i];

        sound->sample_count = (u32)round_f32_to_s32_up(SAMPLES_PER_SECOND * durations[i]);
        sound->samples      = malloc(sound->sample_count * BYTES_PER_SAMPLE);

        u32 samples_per_half_wave_period =
            (u32)round_f32_to_closest_even_s32((f32)SAMPLES_PER_SECOND / frequencies[i]) / 2;

        f32  volume = -VOLUME;
        f32 *left   = (f32 *)sound->samples;
        f32 *right  = left + 1;

        for(u32 sample_index = 0; sample_index < sound->sample_count; ++sample_index)
        {
            if((sample_index % samples_per_half_wave_period) == 0)
            {
                volume = -volume;
            }

            *left  = volume;
            *right = volume;

            left += 2;
            right += 2;
        }
    }

    // NOTE(leo): Here we set any of the sound effects just to avoid a NULL pointer. Nothing
    // will play while bytes_remaining is zero.
    g_samples_to_play = g_point_sound.samples;
}
