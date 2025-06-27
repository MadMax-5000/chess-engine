#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include "board.h"
#include "sdl_graphics.h"
#include "rules.h"
#include "ai.h"

typedef enum {
    GAME_STATE_PLAYING, GAME_STATE_CHECKMATE_WHITE_WINS, GAME_STATE_CHECKMATE_BLACK_WINS,
    GAME_STATE_STALEMATE, GAME_STATE_DRAW_INSUFFICIENT_MATERIAL, GAME_STATE_DRAW_50_MOVE_RULE,
    GAME_STATE_DRAW_THREEFOLD_REPETITION
} GameState;

GameState current_game_state = GAME_STATE_PLAYING;
SDL_Rect play_again_button_rect;

PieceColor human_player_color = WHITE;
PieceColor ai_player_color = BLACK;

void check_game_over_conditions() {
    if (current_game_state != GAME_STATE_PLAYING) return;

    bool no_legal_moves = !has_any_legal_moves(game_board, current_player_turn);
    if (no_legal_moves) {
        if (is_king_in_check(game_board, current_player_turn)) {
            current_game_state = (current_player_turn == WHITE) ? GAME_STATE_CHECKMATE_BLACK_WINS : GAME_STATE_CHECKMATE_WHITE_WINS;
        } else {
            current_game_state = GAME_STATE_STALEMATE;
        }
    } else if (is_draw_by_insufficient_material(game_board)) {
        current_game_state = GAME_STATE_DRAW_INSUFFICIENT_MATERIAL;
    } else if (halfmove_clock >= 100) {
        current_game_state = GAME_STATE_DRAW_50_MOVE_RULE;
    }

    if (current_game_state != GAME_STATE_PLAYING) {
        printf("Game Over! Result: %d (Current turn was for: %s)\n", current_game_state, current_player_turn == WHITE ? "White" : "Black");
    }
}

void init_game_elements() {
    init_board();
    current_game_state = GAME_STATE_PLAYING;

    human_player_color = WHITE;
    ai_player_color = BLACK;
    printf("Human plays as %s, AI plays as %s.\n",
           human_player_color == WHITE ? "White" : "Black",
           ai_player_color == WHITE ? "White" : "Black");

    int button_w = 180, button_h = 50;
    play_again_button_rect = (SDL_Rect){(SCREEN_WIDTH - button_w) / 2, SCREEN_HEIGHT / 2 + 60, button_w, button_h};
}

void execute_the_move(int from_r, int from_c, int to_r, int to_c, PieceType promotion_piece_type) {
    Piece piece_to_move_snapshot = game_board[from_r][from_c];
    Piece piece_at_dest_snapshot = game_board[to_r][to_c];
    int prev_ep_r = en_passant_target_r;
    int prev_ep_c = en_passant_target_c;
    int prev_hm_clk = halfmove_clock;

    bool is_castling_kingside = false, is_castling_queenside = false;
    if (piece_to_move_snapshot.type == KING && abs(to_c - from_c) == 2) {
        if (to_c > from_c) is_castling_kingside = true; else is_castling_queenside = true;
    }
    bool is_ep_capture = false; int ep_cap_r = -1, ep_cap_c = -1;
    if (piece_to_move_snapshot.type == PAWN && to_r == prev_ep_r && to_c == prev_ep_c && piece_at_dest_snapshot.type == EMPTY) {
        is_ep_capture = true; ep_cap_r = from_r; ep_cap_c = to_c;
    }

    record_move(from_r, from_c, to_r, to_c, piece_to_move_snapshot, piece_at_dest_snapshot,
                promotion_piece_type, is_castling_kingside, is_castling_queenside,
                is_ep_capture, ep_cap_r, ep_cap_c, prev_ep_r, prev_ep_c, prev_hm_clk);

    if (piece_to_move_snapshot.type == PAWN || piece_at_dest_snapshot.type != EMPTY) halfmove_clock = 0;
    else halfmove_clock++;

    move_piece_on_board(from_r, from_c, to_r, to_c);

    if (is_castling_kingside) move_piece_on_board(from_r, 7, from_r, 5);
    else if (is_castling_queenside) move_piece_on_board(from_r, 0, from_r, 3);
    if (is_ep_capture) game_board[ep_cap_r][ep_cap_c].type = EMPTY;

    if (promotion_piece_type != EMPTY) {
        game_board[to_r][to_c].type = promotion_piece_type;
        if(current_move_number > 0) move_history[current_move_number-1].promotion_to = promotion_piece_type;
    }

    clear_en_passant_target();
    if (piece_to_move_snapshot.type == PAWN && abs(to_r - from_r) == 2) {
        set_en_passant_target((piece_to_move_snapshot.color == WHITE) ? to_r + 1 : to_r - 1, to_c);
    }
}

int main(int argc, char* args[]) {
    (void)argc; (void)args;

    if (!init_sdl_graphics() || !load_media()) {
        printf("Initialization or media loading failed.\n");
        return 1;
    }
    ai_init_random();
    init_game_elements();

    int quit = 0; SDL_Event e;
    int piece_is_selected = 0;
    int selected_piece_r = -1, selected_piece_c = -1;
    bool button_hovered = false;
    SDL_Point mouse_point = {0,0};

    printf("Game started. Human (%s) vs AI (%s). Press 'U' to Undo.\n",
           human_player_color == WHITE ? "White" : "Black",
           ai_player_color == WHITE ? "White" : "Black");

    while (!quit) {
        button_hovered = false;

        if (current_game_state == GAME_STATE_PLAYING && current_player_turn == ai_player_color) {
            AIMove ai_chosen_move;
 int ai_time_limit_ms = 2000;
             if (ai_select_move(game_board, ai_player_color, &ai_chosen_move, ai_time_limit_ms)) {
                printf("AI %s moves: [%d,%d] to [%d,%d]",
                       ai_player_color == WHITE ? "White" : "Black",
                       ai_chosen_move.from_r, ai_chosen_move.from_c,
                       ai_chosen_move.to_r, ai_chosen_move.to_c);
                if (ai_chosen_move.promotion_to != EMPTY) {
                    printf(" promoting to %s", get_piece_type_string(ai_chosen_move.promotion_to));
                }
                printf("\n");

                execute_the_move(ai_chosen_move.from_r, ai_chosen_move.from_c,
                               ai_chosen_move.to_r, ai_chosen_move.to_c,
                               ai_chosen_move.promotion_to);

                switch_player_turn();
                printf("Turn: Human (%s)", human_player_color == WHITE ? "W":"B");
                if (is_king_in_check(game_board, current_player_turn)) printf(" - Human is in CHECK!");
                printf(" | Moves: %d | HM Clock: %d\n", current_move_number, halfmove_clock);
                check_game_over_conditions();
            } else {
                printf("AI has no moves. Game should be over. State: %d\n", current_game_state);
            }
        }

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = 1;
            else if (e.type == SDL_MOUSEMOTION) {
                mouse_point.x = e.motion.x; mouse_point.y = e.motion.y;
                if (current_game_state != GAME_STATE_PLAYING && SDL_PointInRect(&mouse_point, &play_again_button_rect)) {
                    button_hovered = true;
                }
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_u) {
                    if (current_move_number > 0) {
                        if (undo_last_move()) {
                            printf("Undo successful. Player to move: %s\n", current_player_turn == WHITE ? "W" : "B");
                            piece_is_selected = 0; selected_piece_r = -1; selected_piece_c = -1;
                            current_game_state = GAME_STATE_PLAYING;
                            check_game_over_conditions();
                        }
                    } else {
                        printf("No moves to undo.\n");
                    }
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                mouse_point.x = e.button.x; mouse_point.y = e.button.y;

                if (current_game_state != GAME_STATE_PLAYING) {
                    if (SDL_PointInRect(&mouse_point, &play_again_button_rect)) {
                        init_game_elements();
                        piece_is_selected = 0; selected_piece_r = -1; selected_piece_c = -1;
                        printf("Game restarted. Human (%s) vs AI (%s).\n",
                               human_player_color == WHITE ? "White" : "Black",
                               ai_player_color == WHITE ? "White" : "Black");
                    }
                    continue;
                }

                if (current_player_turn == human_player_color) {
                    int clicked_c = mouse_point.x / SQUARE_SIZE;
                    int clicked_r = mouse_point.y / SQUARE_SIZE;

                    if (is_square_on_board(clicked_r, clicked_c)) {
                        if (!piece_is_selected) {
                            if (game_board[clicked_r][clicked_c].type != EMPTY &&
                                game_board[clicked_r][clicked_c].color == human_player_color) {
                                piece_is_selected = 1;
                                selected_piece_r = clicked_r; selected_piece_c = clicked_c;
                            }
                        } else {
                            int dest_r = clicked_r; int dest_c = clicked_c;
                            bool move_made_by_human = false;

                            if (selected_piece_r == dest_r && selected_piece_c == dest_c) {
                                piece_is_selected = 0; selected_piece_r = -1; selected_piece_c = -1;
                            } else {
                                Piece piece_to_move = game_board[selected_piece_r][selected_piece_c];
                                Piece target_piece = game_board[dest_r][dest_c];

                                if (target_piece.type != EMPTY && target_piece.color == human_player_color) {
                                    selected_piece_r = dest_r; selected_piece_c = dest_c;
                                } else {
                                    PieceType human_promo_choice = EMPTY;
                                    if (piece_to_move.type == PAWN &&
                                        ((piece_to_move.color == WHITE && dest_r == 0) ||
                                         (piece_to_move.color == BLACK && dest_r == 7))) {
                                        if(is_move_legal(game_board, selected_piece_r, selected_piece_c, dest_r, dest_c, human_player_color)){
                                            human_promo_choice = QUEEN;
                                            printf("Human pawn promoting to Queen.\n");
                                        }
                                    }

                                    if (is_move_legal(game_board, selected_piece_r, selected_piece_c, dest_r, dest_c, human_player_color)) {
                                        execute_the_move(selected_piece_r, selected_piece_c, dest_r, dest_c, human_promo_choice);
                                        move_made_by_human = true;
                                    } else {
                                        printf("Human: Illegal move attempt.\n");
                                    }
                                    piece_is_selected = 0; selected_piece_r = -1; selected_piece_c = -1;
                                }
                            }

                            if (move_made_by_human) {
                                switch_player_turn();
                                printf("Turn: AI (%s)", ai_player_color == WHITE ? "W":"B");
                                if (is_king_in_check(game_board, current_player_turn)) printf(" - AI is in CHECK!");
                                printf(" | Moves:%d | HM Clock: %d\n", current_move_number, halfmove_clock);
                                check_game_over_conditions();
                            }
                        }
                    }
                }
            }
        }

        // --- Rendering ---
        SDL_SetRenderDrawColor(g_renderer, 0x33, 0x33, 0x33, 0xFF);
        SDL_RenderClear(g_renderer);
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND); // Enable blending for transparency

        render_board_squares();
        if (piece_is_selected && selected_piece_r != -1 && current_game_state == GAME_STATE_PLAYING && current_player_turn == human_player_color) {
            render_square_highlight(selected_piece_r, selected_piece_c, 255, 255, 0, 100);
        }
        render_pieces();

        if (current_game_state != GAME_STATE_PLAYING) {
            // Draw semi-transparent background
            SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 128); // Semi-transparent black
            SDL_Rect bg_rect = { (SCREEN_WIDTH - 400) / 2, (SCREEN_HEIGHT - 200) / 2, 400, 200 };
            SDL_RenderFillRect(g_renderer, &bg_rect);

            // Game over message
            const char* msg = "";
            switch (current_game_state) {
                case GAME_STATE_CHECKMATE_WHITE_WINS: 
                    msg = human_player_color == WHITE ? "Checkmate! You Win!" : "Checkmate! AI Wins."; 
                    break;
                case GAME_STATE_CHECKMATE_BLACK_WINS: 
                    msg = human_player_color == BLACK ? "Checkmate! You Win!" : "Checkmate! AI Wins."; 
                    break;
                case GAME_STATE_STALEMATE:            
                    msg = "Stalemate! It's a Draw."; 
                    break;
                case GAME_STATE_DRAW_INSUFFICIENT_MATERIAL: 
                    msg = "Draw: Insufficient Material."; 
                    break;
                case GAME_STATE_DRAW_50_MOVE_RULE:    
                    msg = "Draw: 50-Move Rule."; 
                    break;
                default: 
                    msg = "Game Over!"; 
                    break;
            }
            render_text(msg, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 30, (SDL_Color){255, 255, 255, 255}, true);
            render_button("Play Again", play_again_button_rect, (SDL_Color){80, 80, 150, 255}, (SDL_Color){255, 255, 255, 255}, (SDL_Color){100, 100, 180, 255}, button_hovered);
        }
        SDL_RenderPresent(g_renderer);

        if (current_game_state != GAME_STATE_PLAYING) {
            SDL_Delay(10);
        }
    }

    close_sdl_graphics();
    return 0;
}