#ifndef AI_H
#define AI_H

#include "board.h"
#include "rules.h"

typedef struct {
    int from_r, from_c;
    int to_r, to_c;
    PieceType promotion_to;
    int score;
} AIMove;

void ai_init_random();
int ai_evaluate_board(const Piece board[8][8], PieceColor player_to_evaluate_for);
bool ai_select_move(const Piece board[8][8], PieceColor ai_player_color, AIMove* chosen_move, int time_limit_ms); // Added time limit

#endif // AI_H