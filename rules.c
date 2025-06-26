#include "rules.h"
#include <stdlib.h> // For abs()
#include <stdio.h>  // For debugging prints (optional)
#include <string.h> // For memcpy

// Helper: Check if square is on board
bool is_square_on_board(int r, int c) {
    return r >= 0 && r < 8 && c >= 0 && c < 8;
}

// Helper: Check path for sliding pieces (Rook, Bishop, Queen)
static bool is_path_clear(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c) {
    int dr_step = (to_r > from_r) ? 1 : (to_r < from_r) ? -1 : 0;
    int dc_step = (to_c > from_c) ? 1 : (to_c < from_c) ? -1 : 0;

    int r = from_r + dr_step;
    int c = from_c + dc_step;

    while (r != to_r || c != to_c) {
        if (!is_square_on_board(r, c)) return false;
        if (board[r][c].type != EMPTY) {
            return false; // Path is blocked
        }
        r += dr_step;
        c += dc_step;
    }
    return true;
}

// Checks if the square (target_r, target_c) is attacked by any piece of attacker_color
bool is_square_attacked(const Piece board[8][8], int target_r, int target_c, PieceColor attacker_color) {
    for (int r_scan = 0; r_scan < 8; ++r_scan) {
        for (int c_scan = 0; c_scan < 8; ++c_scan) {
            Piece p = board[r_scan][c_scan];
            if (p.type != EMPTY && p.color == attacker_color) {
                // Check if this piece p at (r_scan, c_scan) attacks (target_r, target_c)
                switch (p.type) {
                    case PAWN:
                        if (attacker_color == WHITE) {
                            if ((r_scan - 1 == target_r && c_scan - 1 == target_c) || (r_scan - 1 == target_r && c_scan + 1 == target_c)) {
                                return true;
                            }
                        } else { // BLACK attacker
                            if ((r_scan + 1 == target_r && c_scan - 1 == target_c) || (r_scan + 1 == target_r && c_scan + 1 == target_c)) {
                                return true;
                            }
                        }
                        break;
                    case KNIGHT:
                        {
                            int dr_abs = abs(target_r - r_scan);
                            int dc_abs = abs(target_c - c_scan);
                            if ((dr_abs == 2 && dc_abs == 1) || (dr_abs == 1 && dc_abs == 2)) {
                                return true;
                            }
                        }
                        break;
                    case BISHOP:
                        if (abs(target_r - r_scan) == abs(target_c - c_scan)) { // Diagonal
                            if (is_path_clear(board, r_scan, c_scan, target_r, target_c)) {
                                return true;
                            }
                        }
                        break;
                    case ROOK:
                        if (target_r == r_scan || target_c == c_scan) { // Horizontal or Vertical
                            if (is_path_clear(board, r_scan, c_scan, target_r, target_c)) {
                                return true;
                            }
                        }
                        break;
                    case QUEEN:
                        if (abs(target_r - r_scan) == abs(target_c - c_scan) || target_r == r_scan || target_c == c_scan) {
                            if (is_path_clear(board, r_scan, c_scan, target_r, target_c)) {
                                return true;
                            }
                        }
                        break;
                    case KING:
                        {
                            int dr_abs = abs(target_r - r_scan);
                            int dc_abs = abs(target_c - c_scan);
                            if (dr_abs <= 1 && dc_abs <= 1) { // King attacks adjacent squares
                                return true;
                            }
                        }
                        break;
                    case EMPTY: break; // Should not happen due to p.type != EMPTY check
                }
            }
        }
    }
    return false;
}

// Helper function to find the king of a specific color
// Stores king's location in *king_r_ptr, *king_c_ptr
// Returns true if found, false otherwise (should always be found in a valid game)
static bool find_king_location(const Piece board[8][8], PieceColor king_color, int* king_r_ptr, int* king_c_ptr) {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board[r][c].type == KING && board[r][c].color == king_color) {
                *king_r_ptr = r;
                *king_c_ptr = c;
                return true;
            }
        }
    }
    *king_r_ptr = -1; // Should not happen
    *king_c_ptr = -1;
    // fprintf(stderr, "CRITICAL ERROR: King of color %d not found on board!\n", king_color);
    return false;
}

// NEW: Checks if the king of 'king_color' is currently in check on the given board
bool is_king_in_check(const Piece board[8][8], PieceColor king_color) {
    int king_r, king_c;
    if (!find_king_location(board, king_color, &king_r, &king_c)) {
        // This implies a severe issue with the board state (e.g., king captured before game end)
        // fprintf(stderr, "Error: Could not find king of color %d to check for check.\n", king_color);
        return true; // Treat as "in check" to prevent further illegal states if king is missing.
    }
    PieceColor attacker_color = (king_color == WHITE) ? BLACK : WHITE;
    return is_square_attacked(board, king_r, king_c, attacker_color);
}

// NEW: Checks if the player of 'player_color' has any legal moves on the current board
bool has_any_legal_moves(const Piece board[8][8], PieceColor player_color) {
    // Iterate over all squares on the board
    for (int r_from = 0; r_from < 8; ++r_from) {
        for (int c_from = 0; c_from < 8; ++c_from) {
            // If there's a piece on the square and it belongs to the player
            if (board[r_from][c_from].type != EMPTY && board[r_from][c_from].color == player_color) {
                // Try to move this piece to every other square on the board
                for (int r_to = 0; r_to < 8; ++r_to) {
                    for (int c_to = 0; c_to < 8; ++c_to) {
                        // Store current en_passant_target_r/c because is_move_legal might modify it temporarily
                        // or rely on it. For has_any_legal_moves, we assume the current EP state is correct.
                        int current_ep_r = en_passant_target_r;
                        int current_ep_c = en_passant_target_c;

                        if (is_move_legal(board, r_from, c_from, r_to, c_to, player_color)) {
                            // Restore EP state if it was changed by is_move_legal for this test
                            // Though is_move_legal itself should be careful about global state changes
                            // when used in a "testing" context like this.
                            // For now, we assume is_move_legal is robust enough for this.
                            en_passant_target_r = current_ep_r;
                            en_passant_target_c = current_ep_c;
                            return true; // Found at least one legal move
                        }
                        // Restore EP state if is_move_legal modified it.
                        en_passant_target_r = current_ep_r;
                        en_passant_target_c = current_ep_c;
                    }
                }
            }
        }
    }
    return false; // No legal moves found for any piece
}


// NEW: Checks for draw by insufficient material
bool is_draw_by_insufficient_material(const Piece board[8][8]) {
    int white_knights = 0;
    int white_bishops = 0;
    int white_rooks = 0;
    int white_queens = 0;
    int white_pawns = 0;
    int white_bishops_light_sq = 0; // Bishops on light squares for white
    int white_bishops_dark_sq = 0;  // Bishops on dark squares for white

    int black_knights = 0;
    int black_bishops = 0;
    int black_rooks = 0;
    int black_queens = 0;
    int black_pawns = 0;
    int black_bishops_light_sq = 0; // Bishops on light squares for black
    int black_bishops_dark_sq = 0;  // Bishops on dark squares for black

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board[r][c];
            if (p.type == EMPTY || p.type == KING) continue; // Kings are assumed

            if (p.color == WHITE) {
                if (p.type == PAWN) white_pawns++;
                else if (p.type == KNIGHT) white_knights++;
                else if (p.type == BISHOP) {
                    white_bishops++;
                    if ((r + c) % 2 != 0) white_bishops_light_sq++;// Assuming (0,0) is dark, standard chess light/dark
                    else white_bishops_dark_sq++;
                }
                else if (p.type == ROOK) white_rooks++;
                else if (p.type == QUEEN) white_queens++;
            } else if (p.color == BLACK) {
                if (p.type == PAWN) black_pawns++;
                else if (p.type == KNIGHT) black_knights++;
                else if (p.type == BISHOP) {
                    black_bishops++;
                    if ((r + c) % 2 != 0) black_bishops_light_sq++;
                    else black_bishops_dark_sq++;
                }
                else if (p.type == ROOK) black_rooks++;
                else if (p.type == QUEEN) black_queens++;
            }
        }
    }

    // If any player has a pawn, rook, or queen, it's not insufficient material (yet)
    if (white_pawns > 0 || white_rooks > 0 || white_queens > 0 ||
        black_pawns > 0 || black_rooks > 0 || black_queens > 0) {
        return false;
    }

    // Only kings left
    if (white_knights == 0 && white_bishops == 0 && black_knights == 0 && black_bishops == 0) {
        return true; // K vs K
    }

    // King and Knight vs King
    if ((white_knights == 1 && white_bishops == 0 && black_knights == 0 && black_bishops == 0) ||
        (black_knights == 1 && black_bishops == 0 && white_knights == 0 && white_bishops == 0)) {
        return true; // K+N vs K
    }

    // King and Bishop vs King
    if ((white_bishops == 1 && white_knights == 0 && black_knights == 0 && black_bishops == 0) ||
        (black_bishops == 1 && black_knights == 0 && white_knights == 0 && white_bishops == 0)) {
        return true; // K+B vs K
    }

    // King and Bishop vs King and Bishop (if bishops are on same colored squares)
    if (white_knights == 0 && black_knights == 0 && white_bishops == 1 && black_bishops == 1) {
        // Both bishops on light squares OR both bishops on dark squares
        if ((white_bishops_light_sq == 1 && black_bishops_light_sq == 1) ||
            (white_bishops_dark_sq == 1 && black_bishops_dark_sq == 1)) {
            return true; // K+B vs K+B (bishops on same color)
        }
    }
    
    // Note: K+N+N vs K is generally a win, but K+B+N vs K is a win.
    // This function covers common cases. More complex endgames (like two knights vs pawn)
    // are not typically handled by "insufficient material" rules directly but might lead to 50-move or stalemate.
    // FIDE rules state K+N+N vs K is a draw if the opponent plays correctly, but it's not an *automatic* draw by insufficient material upon reaching the position.
    // The most common auto-draws are K vs K, K+N vs K, K+B vs K, K+B vs K+B (same color).

    return false; // Otherwise, assume sufficient material (or leads to other draws/checkmate)
}


// --- Piece-specific pseudo-legal move checkers ---
// These now primarily check if the move follows the piece's basic movement pattern.
// The check for leaving the king in check is handled by the main is_move_legal function.

bool is_pawn_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color) {
    int dr = to_r - from_r;
    int dc = to_c - from_c;

    if (piece_color == WHITE) {
        // Move forward one square
        if (dc == 0 && dr == -1 && board[to_r][to_c].type == EMPTY) return true;
        // Move forward two squares (initial move)
        if (dc == 0 && dr == -2 && from_r == 6 && board[to_r][to_c].type == EMPTY && board[from_r - 1][from_c].type == EMPTY) return true;
        // Capture
        if (abs(dc) == 1 && dr == -1 && board[to_r][to_c].type != EMPTY && board[to_r][to_c].color == BLACK) return true;
        // En Passant Capture for White
        if (abs(dc) == 1 && dr == -1 &&
            to_r == en_passant_target_r && to_c == en_passant_target_c &&
            from_r == 3 ) { // White pawn must be on rank 3 (0-indexed) to perform EP
            // We also need to ensure the pawn to be captured by EP exists, which is implicitly handled
            // if en_passant_target_r/c are correctly set after a black 2-square move.
            // The target square for EP must be empty.
            if (board[to_r][to_c].type == EMPTY) return true;
        }
    } else { // BLACK piece
        // Move forward one square
        if (dc == 0 && dr == 1 && board[to_r][to_c].type == EMPTY) return true;
        // Move forward two squares (initial move)
        if (dc == 0 && dr == 2 && from_r == 1 && board[to_r][to_c].type == EMPTY && board[from_r + 1][from_c].type == EMPTY) return true;
        // Capture
        if (abs(dc) == 1 && dr == 1 && board[to_r][to_c].type != EMPTY && board[to_r][to_c].color == WHITE) return true;
        // En Passant Capture for Black
        if (abs(dc) == 1 && dr == 1 &&
            to_r == en_passant_target_r && to_c == en_passant_target_c &&
            from_r == 4 ) { // Black pawn must be on rank 4
            if (board[to_r][to_c].type == EMPTY) return true;
        }
    }
    return false;
}

bool is_rook_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color) {
    (void)piece_color; // piece_color not strictly needed here due to checks in is_move_legal
    if (from_r != to_r && from_c != to_c) { // Must be horizontal or vertical
        return false;
    }
    return is_path_clear(board, from_r, from_c, to_r, to_c);
}

bool is_knight_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color) {
    (void)board; (void)piece_color;
    int dr_abs = abs(to_r - from_r);
    int dc_abs = abs(to_c - from_c);
    return (dr_abs == 2 && dc_abs == 1) || (dr_abs == 1 && dc_abs == 2);
}

bool is_bishop_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color) {
    (void)piece_color;
    if (abs(to_r - from_r) != abs(to_c - from_c)) { // Must be diagonal
        return false;
    }
    return is_path_clear(board, from_r, from_c, to_r, to_c);
}

bool is_queen_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color) {
    (void)piece_color;
    bool is_straight = (from_r == to_r || from_c == to_c);
    bool is_diagonal = (abs(to_r - from_r) == abs(to_c - from_c));

    if (!is_straight && !is_diagonal) {
        return false;
    }
    return is_path_clear(board, from_r, from_c, to_r, to_c);
}

// For king, castling safety checks (not in check, not through check, not to check) are included here.
// General self-check for normal king moves is handled by the simulation in the main is_move_legal.
bool is_king_move_legal(const Piece board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor piece_color, bool check_castling_safety_and_normal_move) {
    int dr_signed = to_r - from_r;
    int dc_signed = to_c - from_c;
    int dr_abs = abs(dr_signed);
    int dc_abs = abs(dc_signed);

    // Standard one-square move
    if (dr_abs <= 1 && dc_abs <= 1 && (dr_abs + dc_abs > 0) ) { // Ensure it actually moves
        return true; // The self-check is done by simulation in the calling function
    }

    // Castling logic (only if called for primary move validation, not recursively for attack checks)
    if (check_castling_safety_and_normal_move && !board[from_r][from_c].has_moved && dr_signed == 0) {
        // King must be on its original rank and e-file for standard castling
        int expected_king_r = (piece_color == WHITE) ? 7 : 0;
        if (from_r != expected_king_r || from_c != 4) return false;

        PieceColor opponent_color = (piece_color == WHITE) ? BLACK : WHITE;

        // King-side castling (O-O): King e1->g1 (or e8->g8)
        if (dc_signed == 2 && to_c == 6) {
            // Check rook: h1/h8, same row, not moved
            if (board[from_r][7].type == ROOK && board[from_r][7].color == piece_color && !board[from_r][7].has_moved) {
                // Path clear: f1/f8, g1/g8
                if (board[from_r][5].type == EMPTY && board[from_r][6].type == EMPTY) {
                    // King not in check, not through attacked square, not to attacked square
                    if (!is_square_attacked(board, from_r, 4, opponent_color) &&      // e-file (current)
                        !is_square_attacked(board, from_r, 5, opponent_color) &&  // f-file (pass-through)
                        !is_square_attacked(board, from_r, 6, opponent_color)) {  // g-file (landing)
                        return true;
                    }
                }
            }
        }
        // Queen-side castling (O-O-O): King e1->c1 (or e8->c8)
        else if (dc_signed == -2 && to_c == 2) {
             // Check rook: a1/a8, same row, not moved
            if (board[from_r][0].type == ROOK && board[from_r][0].color == piece_color && !board[from_r][0].has_moved) {
                // Path clear: d1/d8, c1/c8, b1/b8
                if (board[from_r][3].type == EMPTY &&
                    board[from_r][2].type == EMPTY &&
                    board[from_r][1].type == EMPTY) {
                    // King not in check, not through attacked square, not to attacked square
                    if (!is_square_attacked(board, from_r, 4, opponent_color) &&      // e-file (current)
                        !is_square_attacked(board, from_r, 3, opponent_color) &&  // d-file (pass-through)
                        !is_square_attacked(board, from_r, 2, opponent_color)) {  // c-file (landing)
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// MODIFIED: Main move legality function
// Combines pseudo-legal checks (piece movement rules) with self-check prevention.
bool is_move_legal(const Piece original_board[8][8], int from_r, int from_c, int to_r, int to_c, PieceColor player_turn) {
    // Step 0: Basic pre-checks
    if (!is_square_on_board(from_r, from_c) || !is_square_on_board(to_r, to_c)) return false;

    Piece moving_piece_original = original_board[from_r][from_c];
    Piece target_piece_original = original_board[to_r][to_c]; // Piece at destination on original board

    if (moving_piece_original.type == EMPTY) return false;
    if (moving_piece_original.color != player_turn) return false;
    // Cannot capture own piece (target square has a piece of the same color)
    if (target_piece_original.type != EMPTY && target_piece_original.color == player_turn) return false;
    if (from_r == to_r && from_c == to_c) return false; // Cannot move to the same square

    // Step 1: Basic piece movement rule validation (pseudo-legality)
    bool pseudo_legal = false;
    switch (moving_piece_original.type) {
        case PAWN:   pseudo_legal = is_pawn_move_legal(original_board, from_r, from_c, to_r, to_c, player_turn); break;
        case ROOK:   pseudo_legal = is_rook_move_legal(original_board, from_r, from_c, to_r, to_c, player_turn); break;
        case KNIGHT: pseudo_legal = is_knight_move_legal(original_board, from_r, from_c, to_r, to_c, player_turn); break;
        case BISHOP: pseudo_legal = is_bishop_move_legal(original_board, from_r, from_c, to_r, to_c, player_turn); break;
        case QUEEN:  pseudo_legal = is_queen_move_legal(original_board, from_r, from_c, to_r, to_c, player_turn); break;
        case KING:   pseudo_legal = is_king_move_legal(original_board, from_r, from_c, to_r, to_c, player_turn, true); break;
        default:     return false; // Unknown piece type
    }

    if (!pseudo_legal) {
        // printf("DEBUG: Move [%d,%d]->[%d,%d] failed pseudo-legal check for %s.\n", from_r, from_c, to_r, to_c, get_piece_type_string(moving_piece_original.type));
        return false; // Fails basic movement rules or castling conditions
    }

    // Step 2: Simulate the move on a temporary board and check if it leaves the player's king in check.
    Piece temp_board[8][8];
    memcpy(temp_board, original_board, sizeof(Piece[8][8]));

    // --- Simulate Special Move Aspects on Temp Board (if applicable) ---
    // En Passant: If the pseudo-legal move was an EP capture, remove the captured pawn on the temp_board.
    if (moving_piece_original.type == PAWN && to_c != from_c && temp_board[to_r][to_c].type == EMPTY) { // Diagonal move to an empty square
        if (to_r == en_passant_target_r && to_c == en_passant_target_c) { // And it's the EP target square
            int captured_pawn_actual_r = (player_turn == WHITE) ? to_r + 1 : to_r - 1; // Row of the pawn being captured
            if (is_square_on_board(captured_pawn_actual_r, to_c) &&
                temp_board[captured_pawn_actual_r][to_c].type == PAWN &&
                temp_board[captured_pawn_actual_r][to_c].color != player_turn) {
                // Remove the captured pawn from the temporary board
                temp_board[captured_pawn_actual_r][to_c].type = EMPTY;
                temp_board[captured_pawn_actual_r][to_c].color = NO_COLOR;
                // temp_board[captured_pawn_actual_r][to_c].has_moved does not matter for empty
            }
        }
    }

    // --- Perform the main piece move on the temporary board ---
    temp_board[to_r][to_c] = temp_board[from_r][from_c]; // Move the piece
    temp_board[to_r][to_c].has_moved = true;             // Update has_moved on temp board
    temp_board[from_r][from_c].type = EMPTY;             // Empty original square
    temp_board[from_r][from_c].color = NO_COLOR;
    // temp_board[from_r][from_c].has_moved = false; // Not strictly necessary for empty

    // Castling: If the pseudo-legal move was castling, also move the rook on the temp_board.
    if (moving_piece_original.type == KING && abs(to_c - from_c) == 2) { // King moved two squares horizontally
        int rook_original_col, rook_dest_col;
        // from_r is the king's row
        if (to_c > from_c) { // King-side castling (e.g., e1g1 or e8g8), rook from h-file to f-file
            rook_original_col = 7;
            rook_dest_col = 5; // to_c - 1
        } else { // Queen-side castling (e.g., e1c1 or e8c8), rook from a-file to d-file
            rook_original_col = 0;
            rook_dest_col = 3; // to_c + 1
        }
        // Important: use original_board to get the rook's data before it was potentially overwritten
        // if rook_original_col was the same as from_r/from_c (not possible for king) or to_r/to_c (not for rook in castle).
        // The rook on temp_board at original_board[from_r][rook_original_col]
        if(original_board[from_r][rook_original_col].type == ROOK && original_board[from_r][rook_original_col].color == player_turn){
            temp_board[from_r][rook_dest_col] = original_board[from_r][rook_original_col]; // Move rook data
            temp_board[from_r][rook_dest_col].has_moved = true;
            temp_board[from_r][rook_original_col].type = EMPTY;
            temp_board[from_r][rook_original_col].color = NO_COLOR;
        } else {
             // This would be an inconsistency if castling was deemed pseudo-legal
            // printf("DEBUG: Castling simulation error - rook not found at expected original position [%d][%d]\n", from_r, rook_original_col);
        }
    }
    
    // Step 3: Check if the current player's king is in check on the temporary board
    if (is_king_in_check(temp_board, player_turn)) {
        // printf("DEBUG: Move from %d,%d to %d,%d for %s results in self-check.\n", from_r, from_c, to_r, to_c, player_turn == WHITE ? "W" : "B");
        return false; // Move is illegal because it leaves/puts king in check
    }

    return true; // Move is fully legal
}

