#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <cassert>
#include <time.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_mixer.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int s8;
typedef int s16;
//typedef int s32;
typedef float f32;
typedef double f64;

//#include "colors.h"
typedef struct Color
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} Color;

inline Color
color(u8 r, u8 g, u8 b, u8 a)
{
    Color result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = a;
    return result;
}

const Color BASE_COLORS[] = {
    color(0x28, 0x28, 0x28, 0xFF),
    color(0x2D, 0x99, 0x99, 0xFF),
    color(0x99, 0x99, 0x2D, 0xFF),
    color(0x99, 0x2D, 0x99, 0xFF),
    color(0x2D, 0x99, 0x51, 0xFF),
    color(0x99, 0x2D, 0x2D, 0xFF),
    color(0x2D, 0x63, 0x99, 0xFF),
    color(0x99, 0x63, 0x2D, 0xFF)
};

const Color LIGHT_COLORS[] = {
    color(0x28, 0x28, 0x28, 0xFF),
    color(0x44, 0xE5, 0xE5, 0xFF),
    color(0xE5, 0xE5, 0x44, 0xFF),
    color(0xE5, 0x44, 0xE5, 0xFF),
    color(0x44, 0xE5, 0x7A, 0xFF),
    color(0xE5, 0x44, 0x44, 0xFF),
    color(0x44, 0x95, 0xE5, 0xFF),
    color(0xE5, 0x95, 0x44, 0xFF)
};

const Color DARK_COLORS[] = {
    color(0x28, 0x28, 0x28, 0xFF),
    color(0x1E, 0x66, 0x66, 0xFF),
    color(0x66, 0x66, 0x1E, 0xFF),
    color(0x66, 0x1E, 0x66, 0xFF),
    color(0x1E, 0x66, 0x36, 0xFF),
    color(0x66, 0x1E, 0x1E, 0xFF),
    color(0x1E, 0x42, 0x66, 0xFF),
    color(0x66, 0x42, 0x1E, 0xFF)
};
#define WIDTH 10
#define HEIGHT 22
#define VISIBLE_HEIGHT 20
#define GRID_SIZE 30 //lưới

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
//15 level
const u8 FRAMES_PER_DROP[] = {45,40,35,30,25,20,15,10,8,6,5,4,3,2,1}; // tốc độ của từng level =FRAMES_PER_DROP[]*TARGET_SECONDS_PER_FRAME 

const f32 TARGET_SECONDS_PER_FRAME = 1.f / 60.f;

struct Tetrino    //ma tran cua khoi gach
{
    const u8 *data;
    const int side;
};

inline Tetrino tetrino(const u8 *data, int side)
{
    return { data, side };
}

const u8 KHOI_I[] = {
    0, 0, 0, 0,
    1, 1, 1, 1,
    0, 0, 0, 0,
    0, 0, 0, 0
};

const u8 KHOI_O[] = {
    2, 2,
    2, 2
};

const u8 KHOI_T[] = {
    0, 0, 0,
    3, 3, 3,
    0, 3, 0
};

const u8 KHOI_S[] = {
    0, 4, 4,
    4, 4, 0,
    0, 0, 0
};

const u8 KHOI_Z[] = {
    5, 5, 0,
    0, 5, 5,
    0, 0, 0
};

const u8 KHOI_L[] = {
    6, 0, 0,
    6, 6, 6,
    0, 0, 0
};

const u8 KHOI_J[] = {
    0, 0, 7,
    7, 7, 7,
    0, 0, 0
};


const Tetrino TETRINOS[] = {
    tetrino(KHOI_I, 4),
    tetrino(KHOI_O, 2),
    tetrino(KHOI_T, 3),
    tetrino(KHOI_S, 3),
    tetrino(KHOI_Z, 3),
    tetrino(KHOI_L, 3),
    tetrino(KHOI_J, 3),
};

enum Game_Phase
{
    GAME_START,
    GAME_PLAY,
    GAME_LINE,
    GAME_GAMEOVER
};

struct Piece_State
{
    u8 tetrino_index;
    int offset_row;
    int offset_col;
    int rotation; //quay
};

struct Game_State
{
    u8 board[WIDTH * HEIGHT]; //bảng
    u8 lines[HEIGHT];
    int pending_line_count;
    Piece_State piece;
    Game_Phase phase;
    int start_level;
    int level;
    int line_count;
    int points;
    f32 next_drop_time;
    f32 highlight_end_time;
    f32 time;
};

struct Input_State
{
    u8 left;
    u8 right;
    u8 up;
    u8 down;
    u8 a;
    s8 dleft;
    s8 dright;
    s8 dup;
    s8 ddown;
    s8 da;
};

enum Text_Align
{
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,//căn vị trí văn bản
    TEXT_ALIGN_RIGHT
};
//ma trận khối gạch
inline u8 matrix_get(const u8 *values, int width, int row, int col)
{
    int index = row * width + col;
    return values[index];
}
//ma trận khi khối gạch hợp nhất
inline void matrix_set(u8 *values, int width, int row, int col, u8 value)
{
    int index = row * width + col;
    values[index] = value;
}

inline u8 tetrino_get(const Tetrino *tetrino, int row, int col, int rotation)
{//===========gia tri khi quay khoi gach
    int side = tetrino->side;
    switch (rotation)
    {
    case 0:
        return tetrino->data[row * side + col];
    case 1:
        return tetrino->data[(side - col - 1) * side + row];
    case 2:
        return tetrino->data[(side - row - 1) * side + (side - col - 1)];
    case 3:
        return tetrino->data[col * side + (side - row - 1)];
    }
    return 0;
}
//ktra hàng bị đầy
inline u8 check_row_filled(const u8 *values, int width, int row)
{
    for (int col = 0;col < width;++col)
    {
        if (!matrix_get(values, width, row, col))
        {
            return 0;
        }
    }
    return 1;
}
//kiểm tra hàng trống 
inline u8 check_row_empty(const u8 *values, int width, int row)
{
    for (int col = 0;col < width; ++col)
    {
        if (matrix_get(values, width, row, col))
        {
            return 0;
        }
    }
    return 1;
}

int find_lines(const u8 *values, int width, int height, u8 *lines_out)
{
    int count = 0;
    for (int row = 0;row < height;++row)
    {
        u8 filled = check_row_filled(values, width, row);
        lines_out[row] = filled;
        count += filled;
    }
    return count;
}
//xóa 1 hàng khi hàng đó đầy
void clear_lines(u8 *values, int width, int height, const u8 *lines)
{
    int src_row = height - 1;
    for (int dst_row = height - 1;dst_row >= 0;--dst_row)
    {
        while (src_row >= 0 && lines[src_row])
        {
            --src_row;
        }
        if (src_row < 0)
        {
            memset(values + dst_row * width, 0, width);
        }
        else
        {
            if (src_row != dst_row)
            {
                memcpy(values + dst_row * width,values + src_row * width,width);
            }
            --src_row;
        }
    }
}

//ko  để khối gạch vượt biên
bool check_piece_valid(const Piece_State *piece,
                  const u8 *board, int width, int height)
{
    const Tetrino *tetrino = TETRINOS + piece->tetrino_index;
    assert(tetrino);

    for (int row = 0;row < tetrino->side;++row)
    {
        for (int col = 0;col < tetrino->side;++col)
        {
            u8 value = tetrino_get(tetrino, row, col, piece->rotation);
            if (value > 0)
            {
                int board_row = piece->offset_row + row;
                int board_col = piece->offset_col + col;
                if (board_row < 0)
                {
                    return false;
                }
                if (board_row >= height)
                {
                    return false;
                }
                if (board_col < 0)
                {
                    return false;
                }
                if (board_col >= width)
                {
                    return false;
                }
                if (matrix_get(board, width, board_row, board_col))
                {
                    return false;
                }
            }
        }
    }
    return true;
}
// hợp nhất khối gạch
void merge_piece(Game_State *game)
{
    const Tetrino *tetrino = TETRINOS + game->piece.tetrino_index;
    for (int row = 0;row < tetrino->side;++row)
    {
        for (int col = 0;col < tetrino->side;++col)
        {
            u8 value = tetrino_get(tetrino, row, col, game->piece.rotation);
            if (value)
            {
                int board_row = game->piece.offset_row + row;
                int board_col = game->piece.offset_col + col;
                matrix_set(game->board, WIDTH, board_row, board_col, value);
            }
        }
    }
}
//các khối gạch rơi ngẫu nhiên
inline int random_int(int min, int max)
{
    int range = max - min;
    return min + rand() % range;
}
//level max là 15
inline f32 get_time_to_next_drop(int level)
{
    if (level > 15)
    {
        level = 15;
    }
    return FRAMES_PER_DROP[level] * TARGET_SECONDS_PER_FRAME; //level tăng thì tốc độ rơi tăng
}


void spawn_piece(Game_State *game)
{
    game->piece = {};
    game->piece.tetrino_index = (u8)random_int(0, ARRAY_COUNT(TETRINOS));
    game->piece.offset_col = WIDTH / 2;
    game->next_drop_time = game->time + get_time_to_next_drop(game->level);
}
// rơi <
inline bool soft_drop(Game_State *game)
{
    ++game->piece.offset_row;
    if (!check_piece_valid(&game->piece, game->board, WIDTH, HEIGHT))
    {
        --game->piece.offset_row;
        merge_piece(game);
        spawn_piece(game);
        return false;
    }
    game->next_drop_time = game->time + get_time_to_next_drop(game->level);
    return true;
}
// tính điểm
inline int compute_points(int level, int line_count)
{
    switch (line_count)
    {
    case 1:
        return 50 * (level + 1);
    case 2:
        return 100 * (level + 1);
    case 3:
        return 400 * (level + 1);
    case 4:
        return 1000 * (level + 1);
    }
    return 0;
}

inline int min(int x, int y)
{
    return x < y ? x : y;
}
inline int max(int x, int y)
{
    return x > y ? x : y;
}

inline int get_lines_for_next_level(int start_level, int level)
{
    int first_level_up_limit = min((start_level * 10 + 10),max(100, (start_level * 10 - 50)));

    if (level == start_level)
    {
        return first_level_up_limit;
    }
    int diff = level - start_level;
    return first_level_up_limit + diff * 10;
}
//bắt đầu vào  game
void game_start(Game_State *game, const Input_State *input) // s32
{
    if (input->dup > 0)
    {
        ++game->start_level;
    }

    if (input->ddown > 0 && game->start_level > 0)
    {
        --game->start_level;
    }

    if (input->da > 0)
    {
        memset(game->board, 0, WIDTH * HEIGHT);
        game->level = game->start_level;
        game->line_count = 0;
        game->points = 0;
        spawn_piece(game);
        game->phase = GAME_PLAY;
    }
}
//sau khi game over thì chơi lại
void update_game_gameover(Game_State *game, const Input_State *input)
{
    if (input->da > 0)
    {
        game->phase = GAME_START;
    }
}
//load lại hàng gạch sau khi xóa
void update_game_line(Game_State *game)
{
    if (game->time >= game->highlight_end_time)
    {
        clear_lines(game->board, WIDTH, HEIGHT, game->lines);
	 Mix_Chunk* destroy = NULL;
        destroy= Mix_LoadWAV("destroy.wav");
        Mix_PlayChannel(-1, destroy, 0);
        game->line_count += game->pending_line_count;
        game->points += compute_points(game->level, game->pending_line_count);

        int lines_for_next_level = get_lines_for_next_level(game->start_level, game->level);

        if (game->line_count >= lines_for_next_level)
        {
            ++game->level;
        }

        game->phase = GAME_PLAY;
    }
}
// di chuyển trong game
void game_play(Game_State *game , const Input_State *input)
{
    Piece_State piece = game->piece; 
    if (input->dleft > 0)  
    {
        --piece.offset_col;
    }
    if (input->dright> 0)   
    {
        ++piece.offset_col;
    }
    if (input->dup > 0)
    {
        piece.rotation = (piece.rotation + 1) % 4;
    }

    if (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))    //!!!!!
    {
        game->piece = piece;
    }

    if (input->ddown > 0)
    {
        soft_drop(game);
    }

    if (input->da > 0)
    {
        while(soft_drop(game));
    }

    while (game->time >= game->next_drop_time)
    {
        soft_drop(game);
    }

    game->pending_line_count = find_lines(game->board, WIDTH, HEIGHT, game->lines);
    if (game->pending_line_count > 0)
    {
        game->phase = GAME_LINE;
        game->highlight_end_time = game->time + 0.5f;
    }

    int game_over_row = 0;
    if (!check_row_empty(game->board, WIDTH, game_over_row))
    {
        game->phase = GAME_GAMEOVER;
    }
}

void update_game(Game_State *game , const Input_State *input)
{
    switch(game->phase)
    {
    case GAME_START:
        game_start(game, input);
        break;
    case GAME_PLAY:
        game_play(game, input);
        break;
    case GAME_LINE:
        update_game_line(game);
        break;
    case GAME_GAMEOVER:
        update_game_gameover(game, input);
        break;
    }
}
//tô màu 
void fill_rect(SDL_Renderer *renderer , int x , int y , int width, int height, Color color)
{
    SDL_Rect rect = {};
    rect.x = x;//vị trí của góc trên bên trái
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

//vẽ viền
void draw_rect(SDL_Renderer *renderer,
          int x, int y, int width, int height, Color color)
{
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rect);
}
// vẽ 1 ô
void draw_cell(SDL_Renderer *renderer,
          int row, int col, u8 value,
          int offset_x, int offset_y,
          bool outline = false)
{
    Color base_color = BASE_COLORS[value];
    Color light_color = LIGHT_COLORS[value];
    Color dark_color = DARK_COLORS[value];


    int edge = GRID_SIZE / 8;

    int x = col * GRID_SIZE + offset_x;
    int y = row * GRID_SIZE + offset_y;

    if (outline)
    {
        draw_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, base_color);
        return;
    }

    fill_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, dark_color);
    fill_rect(renderer, x + edge, y,
              GRID_SIZE - edge, GRID_SIZE - edge, light_color);
    fill_rect(renderer, x + edge, y + edge,
              GRID_SIZE - edge * 2, GRID_SIZE - edge * 2, base_color);
}
//vẽ các khối
void draw_piece(SDL_Renderer *renderer,
           const Piece_State *piece,
           int offset_x, int offset_y,
           bool outline = false)
{
    const Tetrino *tetrino = TETRINOS + piece->tetrino_index;
    for (int row = 0;row < tetrino->side;++row)
    {
        for (int col = 0;col < tetrino->side;++col)
        {
            u8 value = tetrino_get(tetrino, row, col, piece->rotation);
            if (value)
            {
                draw_cell(renderer,
                          row + piece->offset_row,
                          col + piece->offset_col,
                          value,
                          offset_x, offset_y,
                          outline);
            }
        }
    }
}

void draw_board(SDL_Renderer *renderer,
           const u8 *board, int width, int height,
           int offset_x, int offset_y)
{
    fill_rect(renderer, offset_x, offset_y,
              width * GRID_SIZE, height * GRID_SIZE,
              BASE_COLORS[0]);
    for (int row = 0;row < height;++row)
    {
        for (int col = 0;col < width;++col)
        {
            u8 value = matrix_get(board, width, row, col);
            if (value)
            {
                draw_cell(renderer, row, col, value, offset_x, offset_y);
            }
        }
    }
}

//khởi tạo các định dạng của chữ
void draw_string(SDL_Renderer *renderer,
                 TTF_Font *font,const char *text,
                 int x, int y,
                 Text_Align alignment,Color color)
{
    SDL_Color sdl_color = SDL_Color { color.r, color.g, color.b, color.a };
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, sdl_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect rect;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;
    switch (alignment)
    {
    case TEXT_ALIGN_LEFT:
        rect.x = x;
        break;
    case TEXT_ALIGN_CENTER:
        rect.x = x - surface->w / 2;
        break;
    case TEXT_ALIGN_RIGHT:
        rect.x = x - surface->w;
        break;
    }

    SDL_RenderCopy(renderer, texture, 0, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_game(const Game_State *game , SDL_Renderer *renderer , TTF_Font *font)
{
    char buffer[4096];

    Color highlight_color = color(0xFF, 0xFF, 0xFF, 0xFF); //màu khi 1 hàng đầy(trắng)

    int margin_y = 60;

    draw_board(renderer, game->board, WIDTH, HEIGHT, 0, margin_y);

    if (game->phase == GAME_PLAY)
    {
        draw_piece(renderer, &game->piece, 0, margin_y);

        Piece_State piece = game->piece;
        while (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))
        {
            piece.offset_row++;
        }
        --piece.offset_row;

        draw_piece(renderer, &piece, 0, margin_y, true); 

    }

    if (game->phase == GAME_LINE)
    {
        for (int row = 0;row < HEIGHT;++row)
        {
            if (game->lines[row])
            {
                int x = 0;
                int y = row * GRID_SIZE + margin_y;

                fill_rect(renderer, x, y,
                          WIDTH * GRID_SIZE, GRID_SIZE, highlight_color);
            }
        }
    }
    else if (game->phase == GAME_GAMEOVER)
    {


        int x = WIDTH * GRID_SIZE / 2;
        int y = (HEIGHT * GRID_SIZE + margin_y) / 2;
        draw_string(renderer, font, "GAME OVER ",
                    x, y, TEXT_ALIGN_CENTER, highlight_color);
        draw_string(renderer, font, "PLAY AGAIN!!!",
                    x, y+40, TEXT_ALIGN_CENTER, highlight_color);
        snprintf(buffer, sizeof(buffer), "HIGHT SCORE: %d", game->points);
        draw_string(renderer, font,buffer ,x, y-30, TEXT_ALIGN_CENTER, highlight_color);

    }
    else if (game->phase == GAME_START)
    {
        int x = WIDTH * GRID_SIZE / 2;
        int y = (HEIGHT * GRID_SIZE + margin_y) / 2;
        draw_string(renderer, font, "PRESS START",
                    x, y, TEXT_ALIGN_CENTER, highlight_color);

        snprintf(buffer, sizeof(buffer), "STARTING LEVEL: %d", game->start_level);
        draw_string(renderer, font, buffer,
                    x, y + 30, TEXT_ALIGN_CENTER, highlight_color);
    }

    fill_rect(renderer,
              0, margin_y,
              WIDTH * GRID_SIZE, (HEIGHT - VISIBLE_HEIGHT) * GRID_SIZE,
              color(0x00, 0x00, 0x00, 0x00));


    snprintf(buffer, sizeof(buffer), "LEVEL: %d", game->level);
    draw_string(renderer, font, buffer, 50, 5, TEXT_ALIGN_LEFT, highlight_color);

    snprintf(buffer, sizeof(buffer), "LINES: %d", game->line_count);
    draw_string(renderer, font, buffer, 50, 35, TEXT_ALIGN_LEFT, highlight_color);

    snprintf(buffer, sizeof(buffer), "POINTS: %d", game->points);
    draw_string(renderer, font, buffer, 50, 65, TEXT_ALIGN_LEFT, highlight_color);
}

int main(int argc, char* argv[])
{
    srand(time(NULL)); //để random sau mỗi lần chơi lại
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return 1;
    }

    if (TTF_Init() < 0)
    {
        return 2;
    }
    //tạo cửa sổ cho game
    SDL_Window *window = SDL_CreateWindow("GAME TETRIS - XẾP GẠCH",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,460,720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window,-1,
                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    //cài font chữ
    const char *font_name = "font__.ttf";
    TTF_Font *font = TTF_OpenFont(font_name, 24);
    //khởi tạo âm thanh
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    Mix_Chunk* chunk = NULL;
    chunk= Mix_LoadWAV("sound.wav");
    //Mix_PlayChannel(-1, chunk, -1);

    Game_State game = {};
    Input_State input = {};
    spawn_piece(&game);
    game.piece.tetrino_index = 2;
    //tạo vòng lặp để cửa sổ hiện ra liên tục
    bool quit = false;
    while (!quit)
    {
        game.time = SDL_GetTicks() / 1000.0f;

        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
           else if(e.type == SDL_KEYDOWN)
            {
				if (e.key.keysym.sym == SDLK_SPACE)
				{
					if (!Mix_Playing(-1))
						Mix_PlayChannel(-1, chunk, -1);
				}
            }

        }

        int key_count;
        const u8 *key_states = SDL_GetKeyboardState(&key_count);

        if (key_states[SDL_SCANCODE_ESCAPE])
        {
            quit = true;
        }
    //điều khiển
        Input_State prev_input = input;

        input.left = key_states[SDL_SCANCODE_LEFT];
        input.right = key_states[SDL_SCANCODE_RIGHT];
        input.up = key_states[SDL_SCANCODE_UP];
        input.down = key_states[SDL_SCANCODE_DOWN];
        input.a = key_states[SDL_SCANCODE_SPACE];

        input.dleft = input.left - prev_input.left;
        input.dright = input.right - prev_input.right;
        input.dup = input.up - prev_input.up;
        input.ddown = input.down - prev_input.down;
        input.da = input.a - prev_input.a;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        update_game(&game, &input);
        render_game(&game, renderer, font);

        SDL_RenderPresent(renderer);
    }
    //đóng cửa sổ và các chức năng
    Mix_CloseAudio();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    return 0;
}
