#ifndef SDL_GRAPHICS_H
#define SDL_GRAPHICS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // Include SDL_ttf header
#include "board.h"       // For Piece definition

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480
#define SQUARE_SIZE (SCREEN_WIDTH / 8)

// Global SDL variables (managed by sdl_graphics.c)
extern SDL_Window* g_window;
extern SDL_Renderer* g_renderer;
extern TTF_Font* g_font; // Global font

// Texture array for pieces
extern SDL_Texture* g_piece_textures[KING + 1][BLACK + 1];

// Initializes SDL, creates window and renderer, initializes SDL_ttf
int init_sdl_graphics();

// Loads all piece textures and the font
int load_media();

// Renders the chessboard squares
void render_board_squares();

// Renders a highlight on a specific square
void render_square_highlight(int r, int c, Uint8 R, Uint8 G, Uint8 B, Uint8 A);

// Renders the pieces on the board
void render_pieces();

// Renders text at a given position with a given color
void render_text(const char* text, int x, int y, SDL_Color color, bool centered);

// Renders a button with text
// Returns true if the button is hovered (optional, not used for click yet)
bool render_button(const char* text, SDL_Rect button_rect, SDL_Color bg_color, SDL_Color text_color, SDL_Color hover_bg_color, bool is_hovered);


// Cleans up SDL resources, font, and SDL_ttf
void close_sdl_graphics();

#endif // SDL_GRAPHICS_H