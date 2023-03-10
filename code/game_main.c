#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"

#include "third_party/ryu/d2fixed.c"

#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"

#include "third_party/pcg-c-basic-0.9/pcg_basic.c"

#pragma clang diagnostic pop

#include "utils.c"

#include "strings.c"
#include "os.c"

#include "math.c"
#include "software_renderer.c"
#include "sound.c"

// ===========================================================================================

#define BACKGROUND_COLOR COLOR(0.03f, 0.03f, 0.03f)
#define ENTITIES_COLOR   COLOR(1.0f, 1.0f, 1.0f)

#define PADDLE_WIDTH                 0.015f
#define PADDLE_HEIGHT                0.09f
#define PADDLE_SIMETRICAL_X_POSITION 0.9f
#define LEFT_PADDLE_POSITION_X       (-PADDLE_SIMETRICAL_X_POSITION)
#define RIGHT_PADDLE_POSITION_X      (PADDLE_SIMETRICAL_X_POSITION)
#define BALL_SCALE                   0.015f

#define MIDDLE_LINE_TICK_WIDTH  0.002f
#define MIDDLE_LINE_TICK_HEIGHT 0.02f
#define MIDDLE_LINE_TICK_GAP    (MIDDLE_LINE_TICK_HEIGHT / 2.0f)

#define SCREEN_TOP    1.0f
#define SCREEN_BOTTOM -1.0f
#define SCREEN_LEFT   -1.0f
#define SCREEN_RIGHT  1.0f

#define HALF_HEIGHT(height) ((height * TARGET_ASPECT_RATIO) / 2.0f)

#define MIDDLE_LINE_TICK_AT_TOP    (SCREEN_TOP - HALF_HEIGHT(MIDDLE_LINE_TICK_HEIGHT))
#define MIDDLE_LINE_TICK_AT_BOTTOM (SCREEN_BOTTOM + HALF_HEIGHT(MIDDLE_LINE_TICK_HEIGHT))

#define PADDLE_AT_TOP    (SCREEN_TOP - HALF_HEIGHT(PADDLE_HEIGHT))
#define PADDLE_AT_BOTTOM (SCREEN_BOTTOM + HALF_HEIGHT(PADDLE_HEIGHT))

#define BALL_AT_TOP    (SCREEN_TOP - HALF_HEIGHT(BALL_SCALE))
#define BALL_AT_BOTTOM (SCREEN_BOTTOM + HALF_HEIGHT(BALL_SCALE))

// ===========================================================================================

typedef struct
{
    v2  position;
    v2  velocity;
    f32 width;
    f32 height;

} Entity;

typedef enum
{
    WINNER_NONE,
    WINNER_LEFT,
    WINNER_RIGHT

} Winner;

typedef struct
{
    Entity ball;
    Entity left_paddle;
    Entity right_paddle;
    Winner winner;
    b32    match_started;
    u32    left_points;
    u32    right_points;

} GameState;

enum
{
    KEY_W,
    KEY_S,
    KEY_UP,
    KEY_DOWN,
    KEY_ENTER,

    KEYS_COUNT
};

typedef enum
{
    SOUND_POINT,
    SOUND_WALL,
    SOUND_PADDLE

} SoundToPlay;

// NOTE(leo): It doesn't matter how we initialize this variable, as long as g_bytes_remaining
// is zero, no sound will play.
GLOBAL SoundToPlay g_sound_to_play;

GLOBAL b32 g_is_key_down[KEYS_COUNT] = {0};
GLOBAL b32 g_collision_detected      = false;

// ===========================================================================================

INTERNAL void
set_random_ball_y_position(GameState *game_state)
{
    game_state->ball.position.y = random_f32_0_1();

    // NOTE(leo): We need to add (if position is negative) or subtract (if position is
    // positive) the ball scale to avoid starting with the ball already at the wall, which
    // would trigger the collision detector and play sound.
    game_state->ball.position.y = pcg32_boundedrand_r(&g_rng, 2)
                                    ? game_state->ball.position.y - BALL_SCALE
                                    : -game_state->ball.position.y + BALL_SCALE;
}

INTERNAL void
game_main(GameState *game_state)
{
    memset(game_state, 0, sizeof(*game_state));

    game_state->left_paddle.position.x = LEFT_PADDLE_POSITION_X;
    game_state->left_paddle.width      = PADDLE_WIDTH;
    game_state->left_paddle.height     = PADDLE_HEIGHT;

    game_state->right_paddle.position.x = RIGHT_PADDLE_POSITION_X;
    game_state->right_paddle.width      = PADDLE_WIDTH;
    game_state->right_paddle.height     = PADDLE_HEIGHT;

    game_state->ball.width  = BALL_SCALE;
    game_state->ball.height = BALL_SCALE;

    pcg32_srandom_r(&g_rng, __rdtsc() ^ (u64)game_state, (u64)&memset);

    set_random_ball_y_position(game_state);
}

INTERNAL void
render_middle_line(void)
{
    f32 tick_y_position = MIDDLE_LINE_TICK_AT_TOP;

    while(tick_y_position > MIDDLE_LINE_TICK_AT_BOTTOM)
    {
        draw_rectangle(0.0f,
                       tick_y_position,
                       MIDDLE_LINE_TICK_WIDTH,
                       MIDDLE_LINE_TICK_HEIGHT,
                       ENTITIES_COLOR);
        tick_y_position -=
            (MIDDLE_LINE_TICK_HEIGHT + MIDDLE_LINE_TICK_GAP) * g_back_buffer.aspect_ratio;
    }

    // NOTE(leo): Rendering one more tick just to fill any gap at the bottom.
    draw_rectangle(0.0f,
                   tick_y_position,
                   MIDDLE_LINE_TICK_WIDTH,
                   MIDDLE_LINE_TICK_HEIGHT,
                   ENTITIES_COLOR);
}

INTERNAL void
render_entity(Entity *entity, Color color)
{
    draw_rectangle(entity->position.x,
                   entity->position.y,
                   entity->width,
                   entity->height,
                   color);
}

INTERNAL void
update_and_render_paddles(Entity *left_paddle,
                          Entity *right_paddle,
                          f32     last_frame_time_seconds)
{
#define MAX_VELOCITY_Y 2.3f
#define ACCELERATION   9.0f

    Entity *paddles[]   = {left_paddle, right_paddle};
    int     up_keys[]   = {KEY_W, KEY_UP};
    int     down_keys[] = {KEY_S, KEY_DOWN};

    for(size_t i = 0; i < STATIC_ARRAY_LENGTH(paddles); ++i)
    {
        Entity *paddle   = paddles[i];
        int     up_key   = up_keys[i];
        int     down_key = down_keys[i];

        if(g_is_key_down[up_key] && g_is_key_down[down_key])
        {
            paddle->velocity.y = 0.0f;
        }
        else if(g_is_key_down[up_key])
        {
            if(paddle->velocity.y < MAX_VELOCITY_Y)
            {
                paddle->velocity.y += ACCELERATION * last_frame_time_seconds;
            }
        }
        else if(g_is_key_down[down_key])
        {
            if(paddle->velocity.y > -MAX_VELOCITY_Y)
            {
                paddle->velocity.y -= ACCELERATION * last_frame_time_seconds;
            }
        }
        else
        {
            paddle->velocity.y = 0.0f;
        }

        v2 paddle_frame_velocity =
            v2_scalar_multiply(paddle->velocity, last_frame_time_seconds);
        paddle->position = v2_add(paddle->position, paddle_frame_velocity);

        if(paddle->position.y >= PADDLE_AT_TOP)
        {
            paddle->position.y = PADDLE_AT_TOP;
            paddle->velocity.y = 0.0f;
        }
        else if(paddle->position.y <= PADDLE_AT_BOTTOM)
        {
            paddle->position.y = PADDLE_AT_BOTTOM;
            paddle->velocity.y = 0.0f;
        }

        render_entity(paddle, ENTITIES_COLOR);
    }

#undef MAX_VELOCITY_Y
#undef ACCELERATION
}

INTERNAL void
set_winner(GameState *game_state, Winner winner)
{
    memset(&game_state->ball.velocity, 0, sizeof(game_state->ball.velocity));

    set_random_ball_y_position(game_state);

    game_state->winner = winner;

#define BALL_RESTART_POSITION_X_PADDING 0.07f;
    if(winner == WINNER_LEFT)
    {
        game_state->ball.position.x = -BALL_RESTART_POSITION_X_PADDING;
        game_state->left_points++;
    }
    else if(winner == WINNER_RIGHT)
    {
        game_state->ball.position.x = BALL_RESTART_POSITION_X_PADDING;
        game_state->right_points++;
    }
#undef BALL_RESTART_POSITION_X_PADDING

    game_state->match_started = false;
}

INTERNAL void
update_and_render_ball(GameState *game_state, f32 last_frame_time_seconds)
{
    g_collision_detected = false;

    v2 ball_frame_velocity =
        v2_scalar_multiply(game_state->ball.velocity, last_frame_time_seconds);
    game_state->ball.position = v2_add(game_state->ball.position, ball_frame_velocity);

    if(game_state->ball.position.x > SCREEN_RIGHT)
    {
        g_collision_detected = true;
        g_sound_to_play      = SOUND_POINT;

        set_winner(game_state, WINNER_LEFT);
    }
    else if(game_state->ball.position.x < SCREEN_LEFT)
    {
        g_collision_detected = true;
        g_sound_to_play      = SOUND_POINT;

        set_winner(game_state, WINNER_RIGHT);
    }
    else if(game_state->ball.position.y >= BALL_AT_TOP)
    {
        g_collision_detected = true;
        g_sound_to_play      = SOUND_WALL;

        game_state->ball.position.y = BALL_AT_TOP;
        game_state->ball.velocity.y = -game_state->ball.velocity.y;
    }
    else if(game_state->ball.position.y <= BALL_AT_BOTTOM)
    {
        g_collision_detected = true;
        g_sound_to_play      = SOUND_WALL;

        game_state->ball.position.y = BALL_AT_BOTTOM;
        game_state->ball.velocity.y = -game_state->ball.velocity.y;
    }
    else
    {
        f32 ball_left   = game_state->ball.position.x - (game_state->ball.width / 2.0f);
        f32 ball_right  = game_state->ball.position.x + (game_state->ball.width / 2.0f);
        f32 ball_top    = game_state->ball.position.y + HALF_HEIGHT(game_state->ball.height);
        f32 ball_bottom = game_state->ball.position.y - HALF_HEIGHT(game_state->ball.height);

        f32 left_paddle_left =
            game_state->left_paddle.position.x - (game_state->left_paddle.width / 2.0f);
        f32 left_paddle_right =
            game_state->left_paddle.position.x + (game_state->left_paddle.width / 2.0f);
        f32 left_paddle_top =
            game_state->left_paddle.position.y + HALF_HEIGHT(game_state->left_paddle.height);
        f32 left_paddle_bottom =
            game_state->left_paddle.position.y - HALF_HEIGHT(game_state->left_paddle.height);

        if((ball_left <= left_paddle_right && ball_right >= left_paddle_left)
           && (ball_bottom <= left_paddle_top && ball_top >= left_paddle_bottom)
           && (game_state->ball.velocity.x < 0.0f))
        {
            game_state->ball.position.x =
                LEFT_PADDLE_POSITION_X + (PADDLE_WIDTH / 2.0f) + (BALL_SCALE / 2.0f);

            game_state->ball.velocity.x = -game_state->ball.velocity.x;
            game_state->ball.velocity.y += game_state->left_paddle.velocity.y;

            g_collision_detected = true;

            g_sound_to_play = SOUND_PADDLE;
        }
        else
        {
            f32 right_paddle_left =
                game_state->right_paddle.position.x - (game_state->right_paddle.width / 2.0f);
            f32 right_paddle_right =
                game_state->right_paddle.position.x + (game_state->right_paddle.width / 2.0f);
            f32 right_paddle_top = game_state->right_paddle.position.y
                                 + HALF_HEIGHT(game_state->right_paddle.height);
            f32 right_paddle_bottom = game_state->right_paddle.position.y
                                    - HALF_HEIGHT(game_state->right_paddle.height);

            if((ball_left <= right_paddle_right && ball_right >= right_paddle_left)
               && (ball_bottom <= right_paddle_top && ball_top >= right_paddle_bottom)
               && (game_state->ball.velocity.x > 0.0f))
            {
                game_state->ball.position.x =
                    RIGHT_PADDLE_POSITION_X - (PADDLE_WIDTH / 2.0f) - (BALL_SCALE / 2.0f);

                game_state->ball.velocity.x = -game_state->ball.velocity.x;
                game_state->ball.velocity.y += game_state->right_paddle.velocity.y;

                g_collision_detected = true;

                g_sound_to_play = SOUND_PADDLE;
            }
        }
    }

    if(game_state->match_started)
    {
        render_entity(&game_state->ball, ENTITIES_COLOR);
    }
}

INTERNAL void
render_scoreboard(GameState *game_state)
{
#define TILE_SCALE  0.0124f
#define TOP_TILE_Y  0.9f
#define TILES_COUNT 20
#define DIGITS_GAP  2.0f

#define RIGHT_SCREEN_FIRST_TILE_X                                                            \
    ((SCREEN_RIGHT / 2.0f) - (TILE_SCALE * 2.0f) + (TILE_SCALE / 2.0f))
#define LEFT_SCREEN_FIRST_TILE_X (RIGHT_SCREEN_FIRST_TILE_X + SCREEN_LEFT) // -1.0f

    // clang-format off
    int digits_tilemaps[][TILES_COUNT] =
    {
        {
            1, 1, 1, 1,
            1,       1,
            1,       1,
            1, 0, 0, 1, // 0
            1,       1,
            1,       1,
            1, 1, 1, 1
        },
        {
            0, 0, 0, 1,
            0,       1,
            0,       1,
            0, 0, 0, 1, // 1
            0,       1,
            0,       1,
            0, 0, 0, 1
        },
        {
            1, 1, 1, 1,
            0,       1,
            0,       1,
            1, 1, 1, 1, // 2
            1,       0,
            1,       0,
            1, 1, 1, 1
        },
        {
            1, 1, 1, 1,
            0,       1,
            0,       1,
            1, 1, 1, 1, // 3
            0,       1,
            0,       1,
            1, 1, 1, 1
        },
        {
            1, 0, 0, 1,
            1,       1,
            1,       1,
            1, 1, 1, 1, // 4
            0,       1,
            0,       1,
            0, 0, 0, 1
        },
        {
            1, 1, 1, 1,
            1,       0,
            1,       0,
            1, 1, 1, 1, // 5
            0,       1,
            0,       1,
            1, 1, 1, 1
        },
        {
            1, 0, 0, 0,
            1,       0,
            1,       0,
            1, 1, 1, 1, // 6
            1,       1,
            1,       1,
            1, 1, 1, 1
        },
        {
            1, 1, 1, 1,
            0,       1,
            0,       1,
            0, 0, 0, 1, // 7
            0,       1,
            0,       1,
            0, 0, 0, 1
        },
        {
            1, 1, 1, 1,
            1,       1,
            1,       1,
            1, 1, 1, 1, // 8
            1,       1,
            1,       1,
            1, 1, 1, 1
        },
        {
            1, 1, 1, 1,
            1,       1,
            1,       1,
            1, 1, 1, 1, // 9
            0,       1,
            0,       1,
            0, 0, 0, 1
        }
    };
    // clang-format on

    Color possible_colors[] = {BACKGROUND_COLOR, ENTITIES_COLOR};

    for(int i = 0; i < 2; ++i)
    {
        u32 points = i == 0 ? game_state->left_points : game_state->right_points;

        u32 temp_points  = points;
        u32 digits_count = temp_points == 0 ? 1 : 0;
        while(temp_points)
        {
            digits_count++;
            temp_points /= 10;
        }

        f32 x = i == 0 ? LEFT_SCREEN_FIRST_TILE_X : RIGHT_SCREEN_FIRST_TILE_X;
        x -= ((f32)(digits_count - 1) * (TILE_SCALE * DIGITS_GAP));
        f32 y = TOP_TILE_Y;

        for(u32 j = digits_count; j > 0; --j)
        {
            u32 ten_raised_to_j = 10;
            for(u32 foo = 1; foo < j; ++foo)
            {
                ten_raised_to_j *= 10;
            }

            u32 digit = (points % ten_raised_to_j) / (ten_raised_to_j / 10);

            PixelRect pixel_rect = {0};

            for(size_t k = 0; k < STATIC_ARRAY_LENGTH(digits_tilemaps[0]); ++k)
            {
                Color color_to_use = possible_colors[digits_tilemaps[digit][k]];

                if(k == 0)
                {
                    pixel_rect = draw_rectangle(x, y, TILE_SCALE, TILE_SCALE, color_to_use);
                }
                else
                {
                    draw_rectangle_in_pixels(pixel_rect.x,
                                             pixel_rect.y,
                                             pixel_rect.width,
                                             pixel_rect.height,
                                             color_to_use);
                }

                pixel_rect.x += ((k >= 4 && k <= 7) || (k >= 12 && k <= 15))
                                  ? pixel_rect.width * 3
                                  : pixel_rect.width;

                if(k == 3 || k == 11)
                {
                    pixel_rect.x -= pixel_rect.width * 4;
                    pixel_rect.y += pixel_rect.height;
                }
                else if(k == 5 || k == 7 || k == 13 || k == 15)
                {
                    pixel_rect.x -= pixel_rect.width * 6;
                    pixel_rect.y += pixel_rect.height;
                }
            }

            x += TILE_SCALE * DIGITS_GAP * 3.0f;
        }
    }

#undef TILE_SCALE
#undef TOP_TILE_Y
#undef TILES_COUNT
#undef DIGITS_GAP

#undef RIGHT_SCREEN_FIRST_TILE_X
#undef LEFT_SCREEN_FIRST_TILE_X
}

INTERNAL void
game_update_and_render(GameState *game_state, f32 last_frame_time_seconds)
{
    // TODO(leo): Since it is a waste of time to clear the back buffer every frame, let's find
    // a way to render only the things that changed with the background color before updating
    // and rendering with the actual color.
    clear_back_buffer(BACKGROUND_COLOR);

    if(!game_state->match_started && g_is_key_down[KEY_ENTER])
    {
        game_state->match_started = true;

        game_state->ball.velocity.x = (0.65f * random_f32_0_1()) + 0.65f; // 0.65 <= x < 1.3

#define SEN_75DEG 0.96592582628906828675f

        // NOTE(leo): Generating a velocity vector that is, at maximum, 75 degrees from the X
        // axis.

        game_state->ball.velocity.y =
            game_state->ball.velocity.x * SEN_75DEG * random_f32_0_1();

#undef SEN_75DEG

        // TODO(leo): Decide whether the ball goes up or down randomly.
        game_state->ball.velocity.y = pcg32_boundedrand_r(&g_rng, 2)
                                        ? game_state->ball.velocity.y
                                        : -game_state->ball.velocity.y;

        if(game_state->left_points == 0 && game_state->right_points == 0)
        {
            // TODO(leo): Decides which player starts with the ball ramdomly.
            game_state->ball.velocity.x = pcg32_boundedrand_r(&g_rng, 2)
                                            ? game_state->ball.velocity.x
                                            : -game_state->ball.velocity.x;
        }
        else if(game_state->winner == WINNER_RIGHT)
        {
            game_state->ball.velocity.x = -game_state->ball.velocity.x;
        }
    }

    render_middle_line();

    update_and_render_paddles(&game_state->left_paddle,
                              &game_state->right_paddle,
                              last_frame_time_seconds);

    update_and_render_ball(game_state, last_frame_time_seconds);

    render_scoreboard(game_state);
}

INTERNAL void
game_send_audio(void)
{
    if(g_collision_detected)
    {
        switch(g_sound_to_play)
        {
            case SOUND_POINT:
            {
                g_samples_to_play   = g_point_sound.samples;
                g_bytes_remaining   = g_point_sound.sample_count * BYTES_PER_SAMPLE;
                g_next_byte_to_copy = 0;

                break;
            }
            case SOUND_WALL:
            {
                g_samples_to_play   = g_ball_hit_wall_sound.samples;
                g_bytes_remaining   = g_ball_hit_wall_sound.sample_count * BYTES_PER_SAMPLE;
                g_next_byte_to_copy = 0;

                break;
            }
            case SOUND_PADDLE:
            {
                g_samples_to_play   = g_ball_hit_paddle_sound.samples;
                g_bytes_remaining   = g_ball_hit_paddle_sound.sample_count * BYTES_PER_SAMPLE;
                g_next_byte_to_copy = 0;

                break;
            }
        }
    }
}
