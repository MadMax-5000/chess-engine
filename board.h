#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>

// Represents the color of a piece or an empty square
typedef enum { NO_COLOR, WHITE, BLACK } PieceColor;
typedef enum { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING } PieceType;

typedef struct {
    PieceType type;
    PieceColor color;
    bool has_moved;
} Piece;

// --- NEW: Move Structure ---
typedef struct {
    int from_r, from_c;
    int to_r, to_c;
    Piece piece_moved;         // What piece was moved
    Piece piece_captured;      // What piece was at to_sq (EMPTY if none)
    PieceType promotion_to;    // Type of piece pawn promoted to (EMPTY if no promotion)
    bool was_castling_kingside;
    bool was_castling_queenside;
    bool was_en_passant;
    int captured_ep_pawn_r;   // If EP, original row of captured pawn
    int captured_ep_pawn_c;   // If EP, original col of captured pawn
    // For restoring state before this move:
    int prev_en_passant_target_r;
    int prev_en_passant_target_c;
    int prev_halfmove_clock;
    // We might also need to store castling rights before the move if not derivable
    // from piece.has_moved easily for undo. For now, has_moved is primary.
} Move;

#define MAX_MOVES_IN_GAME 500 // Arbitrary limit for history, can be dynamic

extern Piece game_board[8][8];
extern PieceColor current_player_turn;
extern int en_passant_target_r;
extern int en_passant_target_c;
extern int halfmove_clock;

// --- NEW: Move History ---
extern Move move_history[MAX_MOVES_IN_GAME];
extern int current_move_number; // Index of the *next* move to be stored (also total moves made)

void init_board(); // Will also need to init move history
const char* get_piece_type_string(PieceType type);
const char* get_piece_color_string(PieceColor color);
void move_piece_on_board(int from_r, int from_c, int to_r, int to_c);
void switch_player_turn();
void clear_en_passant_target();
void set_en_passant_target(int r, int c);

// --- NEW: Move History Functions ---
void record_move(int fr, int fc, int tr, int tc, Piece moved, Piece captured, PieceType promo,
                 bool cast_k, bool cast_q, bool ep, int ep_cap_r, int ep_cap_c,
                 int prev_ep_r, int prev_ep_c, int prev_hm_clock);
bool undo_last_move(); // Returns true if undo was successful

#endif // BOARD_H