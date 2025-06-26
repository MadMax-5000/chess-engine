#ifndef RULES_H
#define RULES_H

#include "board.h"
#include <stdbool.h>

bool is_square_on_board(int r, int c);

// NEW: Checks if the square (r, c) is attacked by any piece of attacker_color
bool is_square_attacked(const Piece board[8][8], int r, int c, PieceColor attacker_color);

// NEW: Checks if the player of 'player_color' has any legal moves on the current board
bool has_any_legal_moves(const Piece board[8][8], PieceColor player_color);

// NEW: Checks for draw by insufficient material
bool is_draw_by_insufficient_material(const Piece board[8][8]);

// NEW: Checks if the king of 'king_color' is currently in check on the given board
bool is_king_in_check(const Piece board[8][8], PieceColor king_color);

bool is_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor player_turn);

// Piece-specific validation functions - existing ones will be updated
bool is_pawn_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color);
bool is_rook_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color);
bool is_knight_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color);
bool is_bishop_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color);
bool is_queen_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color);
bool is_king_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color, bool check_castling_safety_and_normal_move);
#endif // RULES_H