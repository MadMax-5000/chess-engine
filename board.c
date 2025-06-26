#include "board.h"
#include <stddef.h>
#include "rules.h" 
#include <stdio.h> // For debug prints
#include <string.h> // For memcpy for potential future board state saving

Piece game_board[8][8];
PieceColor current_player_turn;
int en_passant_target_r = -1;
int en_passant_target_c = -1;
int halfmove_clock = 0;

// --- NEW: Move History Definition ---
Move move_history[MAX_MOVES_IN_GAME];
int current_move_number = 0; // Number of moves made, index for next move

// (get_piece_type_string, get_piece_color_string remain same)
const char* get_piece_type_string(PieceType type) {
    switch (type) {
        case PAWN:   return "pawn"; case KNIGHT: return "knight"; case BISHOP: return "bishop";
        case ROOK:   return "rook"; case QUEEN:  return "queen";  case KING:   return "king";
        default:     return NULL;
    }
}
const char* get_piece_color_string(PieceColor color) {
    switch (color) {
        case WHITE: return "w"; case BLACK: return "b"; default: return NULL;
    }
}

void init_board() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            game_board[r][c] = (Piece){EMPTY, NO_COLOR, false};
        }
    }
    // ... (Standard piece setup with has_moved = false) ...
    for (int c = 0; c < 8; ++c) {
        game_board[1][c] = (Piece){PAWN, BLACK, false};
        game_board[6][c] = (Piece){PAWN, WHITE, false};
    }
    game_board[0][0] = (Piece){ROOK, BLACK, false}; game_board[0][7] = (Piece){ROOK, BLACK, false};
    game_board[7][0] = (Piece){ROOK, WHITE, false}; game_board[7][7] = (Piece){ROOK, WHITE, false};
    game_board[0][1] = (Piece){KNIGHT, BLACK, false}; game_board[0][6] = (Piece){KNIGHT, BLACK, false};
    game_board[7][1] = (Piece){KNIGHT, WHITE, false}; game_board[7][6] = (Piece){KNIGHT, WHITE, false};
    game_board[0][2] = (Piece){BISHOP, BLACK, false}; game_board[0][5] = (Piece){BISHOP, BLACK, false};
    game_board[7][2] = (Piece){BISHOP, WHITE, false}; game_board[7][5] = (Piece){BISHOP, WHITE, false};
    game_board[0][3] = (Piece){QUEEN, BLACK, false}; game_board[7][3] = (Piece){QUEEN, WHITE, false};
    game_board[0][4] = (Piece){KING, BLACK, false}; game_board[7][4] = (Piece){KING, WHITE, false};

    current_player_turn = WHITE;
    clear_en_passant_target();
    halfmove_clock = 0;
    current_move_number = 0; // Reset move history
}

void move_piece_on_board(int from_r, int from_c, int to_r, int to_c) {
    if (!is_square_on_board(from_r, from_c) || !is_square_on_board(to_r, to_c)) return;
    if (from_r == to_r && from_c == to_c) return;

    game_board[to_r][to_c] = game_board[from_r][from_c];
    game_board[to_r][to_c].has_moved = true; // This is the key part for `has_moved`

    game_board[from_r][from_c].type = EMPTY;
    game_board[from_r][from_c].color = NO_COLOR;
    game_board[from_r][from_c].has_moved = false; // Or true, doesn't matter for empty
}

void switch_player_turn() {
    current_player_turn = (current_player_turn == WHITE) ? BLACK : WHITE;
}
void clear_en_passant_target() {
    en_passant_target_r = -1; en_passant_target_c = -1;
}
void set_en_passant_target(int r, int c) {
    en_passant_target_r = r; en_passant_target_c = c;
}

// --- NEW: Move History Functions ---
void record_move(int fr, int fc, int tr, int tc, Piece moved, Piece captured, PieceType promo,
                 bool cast_k, bool cast_q, bool ep, int ep_cap_r, int ep_cap_c,
                 int prev_ep_r, int prev_ep_c, int prev_hm_clock) {
    if (current_move_number >= MAX_MOVES_IN_GAME) {
        printf("Warning: Max move history reached.\n");
        return;
    }
    Move* m = &move_history[current_move_number];
    m->from_r = fr; m->from_c = fc;
    m->to_r = tr; m->to_c = tc;
    m->piece_moved = moved;         // This should be the state *before* its .has_moved was set to true by move_piece_on_board
    m->piece_captured = captured;
    m->promotion_to = promo;
    m->was_castling_kingside = cast_k;
    m->was_castling_queenside = cast_q;
    m->was_en_passant = ep;
    m->captured_ep_pawn_r = ep_cap_r;
    m->captured_ep_pawn_c = ep_cap_c;
    m->prev_en_passant_target_r = prev_ep_r;
    m->prev_en_passant_target_c = prev_ep_c;
    m->prev_halfmove_clock = prev_hm_clock;

    current_move_number++;
}

bool undo_last_move() {
    if (current_move_number == 0) {
        printf("No moves to undo.\n");
        return false;
    }

    current_move_number--; // Go to the last move made
    Move* last_m = &move_history[current_move_number];

    // 1. Restore the piece that moved to its original square
    // The `piece_moved` in history already has its original `has_moved` status.
    game_board[last_m->from_r][last_m->from_c] = last_m->piece_moved;

    // 2. Restore the captured piece (if any) to the destination square
    // Or clear the destination square if it was an empty move
    game_board[last_m->to_r][last_m->to_c] = last_m->piece_captured;

    // 3. Handle pawn promotion undo (demote)
    if (last_m->promotion_to != EMPTY) {
        game_board[last_m->from_r][last_m->from_c].type = PAWN; // It was a pawn that moved to promote
    }

    // 4. Handle castling undo
    if (last_m->was_castling_kingside) {
        // King is already at from_r, from_c (e.g., e1)
        // Rook was at (from_r, to_c-1) (e.g., f1), needs to go back to (from_r, 7) (h1)
        // The rook's piece_moved data is in last_m->piece_moved, which is the KING.
        // We need the original rook.
        Piece rook_to_move_back = game_board[last_m->from_r][last_m->to_c - 1]; // Rook on f1/f8
        rook_to_move_back.has_moved = false; // Reset its moved status
        game_board[last_m->from_r][7] = rook_to_move_back;
        game_board[last_m->from_r][last_m->to_c - 1] = (Piece){EMPTY, NO_COLOR, false}; // Clear f1/f8
    } else if (last_m->was_castling_queenside) {
        // King at from_r, from_c (e.g., e1)
        // Rook was at (from_r, to_c+1) (e.g., d1), needs to go back to (from_r, 0) (a1)
        Piece rook_to_move_back = game_board[last_m->from_r][last_m->to_c + 1]; // Rook on d1/d8
        rook_to_move_back.has_moved = false;
        game_board[last_m->from_r][0] = rook_to_move_back;
        game_board[last_m->from_r][last_m->to_c + 1] = (Piece){EMPTY, NO_COLOR, false}; // Clear d1/d8
    }

    // 5. Handle en passant undo
    if (last_m->was_en_passant) {
        // The moving pawn is at from_r, from_c.
        // The captured pawn was at captured_ep_pawn_r, captured_ep_pawn_c.
        // It was captured by piece_moved (a pawn). We need to restore it.
        // The color of the captured EP pawn is the opponent's color.
        PieceColor captured_pawn_color = (last_m->piece_moved.color == WHITE) ? BLACK : WHITE;
        game_board[last_m->captured_ep_pawn_r][last_m->captured_ep_pawn_c] = (Piece){PAWN, captured_pawn_color, true}; // Assume it had moved
        // The destination square to_r, to_c should already be empty or contain what was there before the EP capture
        // which is handled by restoring piece_captured (which should be EMPTY for EP).
    }

    // 6. Restore game state variables
    halfmove_clock = last_m->prev_halfmove_clock;
    en_passant_target_r = last_m->prev_en_passant_target_r;
    en_passant_target_c = last_m->prev_en_passant_target_c;

    // 7. Switch player turn back
    switch_player_turn(); // This switches to the player who made the undone move

    printf("Move undone. Player to move: %s\n", current_player_turn == WHITE ? "W" : "B");
    return true;
}