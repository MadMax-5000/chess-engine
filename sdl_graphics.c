#include "sdl_graphics.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>

// Define global SDL variables
SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;
TTF_Font* g_font = NULL; // Define global font
SDL_Texture* g_piece_textures[KING + 1][BLACK + 1];

int init_sdl_graphics() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    g_window = SDL_CreateWindow("C Chess Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (g_window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (g_renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 0;
    }

    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        printf("SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyRenderer(g_renderer);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 0;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_DestroyRenderer(g_renderer);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 0;
    }
    
    for (int i = 0; i <= KING; ++i) {
        for (int j = 0; j <= BLACK; ++j) {
            g_piece_textures[i][j] = NULL;
        }
    }

    printf("SDL Graphics Initialized Successfully.\n");
    return 1;
}

// Helper function to load font (called by load_media)
int load_font(const char* path, int size) {
    g_font = TTF_OpenFont(path, size);
    if (g_font == NULL) {
        printf("Failed to load font %s! TTF_Error: %s\n", path, TTF_GetError());
        return 0;
    }
    printf("Font '%s' loaded successfully.\n", path);
    return 1;
}

int load_media() {
    char filepath[100];
    bool textures_loaded_ok = true;

    for (PieceType type = PAWN; type <= KING; ++type) {
        for (PieceColor color = WHITE; color <= BLACK; ++color) {
            const char* type_str = get_piece_type_string(type);
            const char* color_str = get_piece_color_string(color);

            if (type_str && color_str) {
                snprintf(filepath, sizeof(filepath), "images/%s-%s.svg", type_str, color_str);
                SDL_Surface* loaded_surface = IMG_Load(filepath);
                if (loaded_surface == NULL) {
                    printf("Unable to load image %s! SDL_image Error: %s\n", filepath, IMG_GetError());
                    textures_loaded_ok = false;
                } else {
                    g_piece_textures[type][color] = SDL_CreateTextureFromSurface(g_renderer, loaded_surface);
                    if (g_piece_textures[type][color] == NULL) {
                        printf("Unable to create texture from %s! SDL Error: %s\n", filepath, SDL_GetError());
                        textures_loaded_ok = false;
                    }
                    SDL_FreeSurface(loaded_surface);
                }
            }
        }
    }

    // Load font - IMPORTANT: Make sure "font.ttf" exists in your execution directory or specify a correct path.
    if (!load_font("font.ttf", 24)) { // Using font size 24
        // You might want to make this a fatal error or have a fallback.
        printf("Continuing without a font. Text will not be rendered.\n");
        // return 0; // Optionally fail hard if font is critical
    }


    if (!textures_loaded_ok && g_font == NULL) {
         printf("Failed to load any media (textures and font).\n");
         return 0;
    }
    printf("Media loading process finished.\n");
    return 1;
}


void render_board_squares() {
    SDL_Rect square_rect;
    square_rect.w = SQUARE_SIZE;
    square_rect.h = SQUARE_SIZE;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            square_rect.x = c * SQUARE_SIZE;
            square_rect.y = r * SQUARE_SIZE;
            if ((r + c) % 2 == 0) {
                SDL_SetRenderDrawColor(g_renderer, 238, 238, 210, 255);
            } else {
                SDL_SetRenderDrawColor(g_renderer, 118, 150, 86, 255);
            }
            SDL_RenderFillRect(g_renderer, &square_rect);
        }
    }
}

void render_pieces() {
    SDL_Rect dest_rect;
    dest_rect.w = SQUARE_SIZE;
    dest_rect.h = SQUARE_SIZE;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece current_piece = game_board[r][c];
            if (current_piece.type != EMPTY) {
                SDL_Texture* piece_texture = g_piece_textures[current_piece.type][current_piece.color];
                if (piece_texture) {
                    dest_rect.x = c * SQUARE_SIZE;
                    dest_rect.y = r * SQUARE_SIZE;
                    SDL_RenderCopy(g_renderer, piece_texture, NULL, &dest_rect);
                }
            }
        }
    }
}

void render_square_highlight(int r, int c, Uint8 R, Uint8 G, Uint8 B, Uint8 A) {
    if (r < 0 || r >= 8 || c < 0 || c >= 8) return;
    SDL_Rect highlight_rect = {c * SQUARE_SIZE, r * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE};
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, R, G, B, A);
    SDL_RenderFillRect(g_renderer, &highlight_rect);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
}

void render_text(const char* text, int x, int y, SDL_Color color, bool centered) {
    if (g_font == NULL) return; // Cannot render text without a font

    SDL_Surface* text_surface = TTF_RenderText_Blended(g_font, text, color); // Blended for smoother text
    if (text_surface == NULL) {
        printf("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(g_renderer, text_surface);
    if (text_texture == NULL) {
        printf("Unable to create texture from rendered text! SDL_Error: %s\n", SDL_GetError());
        SDL_FreeSurface(text_surface);
        return;
    }

    SDL_Rect render_quad = {x, y, text_surface->w, text_surface->h};
    if (centered) {
        render_quad.x = x - text_surface->w / 2;
        render_quad.y = y - text_surface->h / 2;
    }


    SDL_RenderCopy(g_renderer, text_texture, NULL, &render_quad);

    SDL_FreeSurface(text_surface);
    SDL_DestroyTexture(text_texture);
}

bool render_button(const char* text, SDL_Rect button_rect, SDL_Color bg_color, SDL_Color text_color, SDL_Color hover_bg_color, bool is_hovered) {
    SDL_Color current_bg_color = is_hovered ? hover_bg_color : bg_color;

    // Draw button background
    SDL_SetRenderDrawColor(g_renderer, current_bg_color.r, current_bg_color.g, current_bg_color.b, current_bg_color.a);
    SDL_RenderFillRect(g_renderer, &button_rect);

    // Draw button border (optional)
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255); // Black border
    SDL_RenderDrawRect(g_renderer, &button_rect);


    // Render text centered on button
    if (g_font && text) {
        int text_w, text_h;
        if (TTF_SizeText(g_font, text, &text_w, &text_h) == 0) {
            int text_x = button_rect.x + (button_rect.w - text_w) / 2;
            int text_y = button_rect.y + (button_rect.h - text_h) / 2;
            render_text(text, text_x, text_y, text_color, false); // Not centered again, already calculated
        } else {
            printf("TTF_SizeText Error: %s\n", TTF_GetError());
        }
    }
    return is_hovered; // Could be used for more complex hover effects
}


void close_sdl_graphics() {
    for (int type = PAWN; type <= KING; ++type) {
        for (int color = WHITE; color <= BLACK; ++color) {
            if (g_piece_textures[type][color] != NULL) {
                SDL_DestroyTexture(g_piece_textures[type][color]);
                g_piece_textures[type][color] = NULL;
            }
        }
    }

    if (g_font) {
        TTF_CloseFont(g_font);
        g_font = NULL;
    }

    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }

    TTF_Quit(); // Quit SDL_ttf subsystem
    IMG_Quit();
    SDL_Quit();
    printf("SDL Graphics closed.\n");
}