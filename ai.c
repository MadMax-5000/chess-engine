#include "ai.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <SDL2/SDL_timer.h> // For SDL_GetTicks()

// --- Piece Values ---
#define PAWN_VALUE   100
#define KNIGHT_VALUE 320
#define BISHOP_VALUE 330
#define ROOK_VALUE   500
#define QUEEN_VALUE  900
#define KING_VALUE   20000 // For checkmate evaluation

// --- Piece-Square Tables (PSTs) ---
// These tables give a bonus or penalty for a piece being on a specific square.
// They are defined from White's perspective; for Black, the row index is mirrored.
const int pawn_pst_white[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5,  5, 10, 25, 25, 10,  5,  5},
    {0,  0,  0, 20, 20,  0,  0,  0},
    {5, -5,-10,  0,  0,-10, -5,  5},
    {5, 10, 10,-20,-20, 10, 10,  5},
    {0,  0,  0,  0,  0,  0,  0,  0}
};

const int knight_pst_white[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

const int bishop_pst_white[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

const int rook_pst_white[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {5, 10, 10, 10, 10, 10, 10,  5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {0,  0,  0,  5,  5,  0,  0,  0}
};

const int queen_pst_white[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    {-5,  0,  5,  5,  5,  5,  0, -5},
    {0,  0,  5,  5,  5,  5,  0, -5},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};

const int king_pst_white_midgame[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    { 20, 20,  0,  0,  0,  0, 20, 20},
    { 20, 30, 10,  0,  0, 10, 30, 20}
};

// --- Killer Moves ---
#define MAX_SEARCH_PLY 30 // Max search depth for storing killer moves
AIMove killer_moves[MAX_SEARCH_PLY][2]; // [ply][killer_slot]
int nodes_searched; // Global counter for performance tracking

void ai_init_random() {
    srand(time(NULL));
    // Initialize killer move table
    for (int i = 0; i < MAX_SEARCH_PLY; ++i) {
        killer_moves[i][0].from_r = -1; // Mark as invalid
        killer_moves[i][1].from_r = -1;
    }
}

int ai_evaluate_board(const Piece board[8][8], PieceColor player_to_evaluate_for) {
    int material_score = 0;
    int positional_score = 0;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board[r][c];
            if (p.type != EMPTY) {
                int piece_val = 0;
                int pst_val_for_piece = 0;
                int r_for_pst = (p.color == WHITE) ? r : (7 - r);

                switch (p.type) {
                    case PAWN:   piece_val = PAWN_VALUE;   pst_val_for_piece = pawn_pst_white[r_for_pst][c]; break;
                    case KNIGHT: piece_val = KNIGHT_VALUE; pst_val_for_piece = knight_pst_white[r_for_pst][c]; break;
                    case BISHOP: piece_val = BISHOP_VALUE; pst_val_for_piece = bishop_pst_white[r_for_pst][c]; break;
                    case ROOK:   piece_val = ROOK_VALUE;   pst_val_for_piece = rook_pst_white[r_for_pst][c]; break;
                    case QUEEN:  piece_val = QUEEN_VALUE;  pst_val_for_piece = queen_pst_white[r_for_pst][c]; break;
                    case KING:   pst_val_for_piece = king_pst_white_midgame[r_for_pst][c]; break;
                    default: break;
                }
                if (p.color == player_to_evaluate_for) {
                    if (p.type != KING) material_score += piece_val;
                    positional_score += pst_val_for_piece;
                } else {
                    if (p.type != KING) material_score -= piece_val;
                    positional_score -= pst_val_for_piece;
                }
            }
        }
    }
    int final_score = material_score + positional_score;
    
    PieceColor opponent_color = (player_to_evaluate_for == WHITE) ? BLACK : WHITE;
    bool player_has_moves = has_any_legal_moves(board, player_to_evaluate_for);
    if (!player_has_moves) {
        return is_king_in_check(board, player_to_evaluate_for) ? -KING_VALUE : 0; // Checkmate or Stalemate
    }
    bool opponent_has_moves = has_any_legal_moves(board, opponent_color);
    if (!opponent_has_moves && is_king_in_check(board, opponent_color)) {
        return KING_VALUE; // Opponent is checkmated
    }
    return final_score;
}

static int find_all_legal_ai_moves(const Piece board[8][8], PieceColor player_color, AIMove legal_moves[], int max_moves_capacity) {
    int count = 0;
    for (int r_from = 0; r_from < 8; ++r_from) {
        for (int c_from = 0; c_from < 8; ++c_from) {
            if (board[r_from][c_from].type != EMPTY && board[r_from][c_from].color == player_color) {
                for (int r_to = 0; r_to < 8; ++r_to) {
                    for (int c_to = 0; c_to < 8; ++c_to) {
                        int original_ep_r = en_passant_target_r; int original_ep_c = en_passant_target_c;
                        if (is_move_legal(board, r_from, c_from, r_to, c_to, player_color)) {
                            if (count < max_moves_capacity) {
                                legal_moves[count].from_r = r_from; legal_moves[count].from_c = c_from;
                                legal_moves[count].to_r = r_to;     legal_moves[count].to_c = c_to;
                                legal_moves[count].promotion_to = EMPTY; legal_moves[count].score = 0;
                                if (board[r_from][c_from].type == PAWN && ((player_color == WHITE && r_to == 0) || (player_color == BLACK && r_to == 7))) {
                                    legal_moves[count].promotion_to = QUEEN;
                                }
                                count++;
                            } else { en_passant_target_r = original_ep_r; en_passant_target_c = original_ep_c; return count; }
                        }
                        en_passant_target_r = original_ep_r; en_passant_target_c = original_ep_c;
                    }
                }
            }
        }
    }
    return count;
}

typedef struct {
    Piece captured_piece;
    Piece original_mover_piece;
    int old_ep_r, old_ep_c;
    bool castled_k, castled_q;
    Piece actual_ep_captured_pawn;
    int actual_ep_captured_pawn_r, actual_ep_captured_pawn_c;
} TempMoveInfo;

static TempMoveInfo make_temporary_move(Piece board_copy[8][8], const AIMove* move, PieceColor moving_player_color) {
    TempMoveInfo info;
    info.original_mover_piece = board_copy[move->from_r][move->from_c];
    info.captured_piece = board_copy[move->to_r][move->to_c];
    info.old_ep_r = en_passant_target_r; info.old_ep_c = en_passant_target_c;
    info.castled_k = false; info.castled_q = false;
    info.actual_ep_captured_pawn.type = EMPTY;

    Piece piece_to_move_on_copy = board_copy[move->from_r][move->from_c];
    board_copy[move->to_r][move->to_c] = piece_to_move_on_copy;
    board_copy[move->to_r][move->to_c].has_moved = true;
    board_copy[move->from_r][move->from_c].type = EMPTY;

    if (move->promotion_to != EMPTY) board_copy[move->to_r][move->to_c].type = move->promotion_to;

    if (piece_to_move_on_copy.type == PAWN && move->from_c != move->to_c && info.captured_piece.type == EMPTY && move->to_r == info.old_ep_r && move->to_c == info.old_ep_c) {
        info.actual_ep_captured_pawn_r = (moving_player_color == WHITE) ? move->to_r + 1 : move->to_r - 1;
        info.actual_ep_captured_pawn_c = move->to_c;
        if (is_square_on_board(info.actual_ep_captured_pawn_r, info.actual_ep_captured_pawn_c) && board_copy[info.actual_ep_captured_pawn_r][info.actual_ep_captured_pawn_c].type == PAWN) {
            info.actual_ep_captured_pawn = board_copy[info.actual_ep_captured_pawn_r][info.actual_ep_captured_pawn_c];
            board_copy[info.actual_ep_captured_pawn_r][info.actual_ep_captured_pawn_c].type = EMPTY;
        } else { info.actual_ep_captured_pawn.type = EMPTY; }
    }
    if (piece_to_move_on_copy.type == KING && abs(move->to_c - move->from_c) == 2) {
        int rook_orig_c, rook_dest_c;
        if (move->to_c > move->from_c) { rook_orig_c = 7; rook_dest_c = 5; info.castled_k = true; }
        else { rook_orig_c = 0; rook_dest_c = 3; info.castled_q = true; }
        board_copy[move->from_r][rook_dest_c] = board_copy[move->from_r][rook_orig_c];
        board_copy[move->from_r][rook_dest_c].has_moved = true;
        board_copy[move->from_r][rook_orig_c].type = EMPTY;
    }
    clear_en_passant_target();
    if (piece_to_move_on_copy.type == PAWN && abs(move->to_r - move->from_r) == 2) {
        set_en_passant_target((moving_player_color == WHITE) ? move->to_r + 1 : move->to_r - 1, move->to_c);
    }
    return info;
}

static void undo_temporary_move(Piece board_copy[8][8], const AIMove* move, TempMoveInfo info) {
    board_copy[move->from_r][move->from_c] = info.original_mover_piece;
    board_copy[move->to_r][move->to_c] = info.captured_piece;
    if (info.actual_ep_captured_pawn.type != EMPTY) {
        board_copy[info.actual_ep_captured_pawn_r][info.actual_ep_captured_pawn_c] = info.actual_ep_captured_pawn;
        if (info.captured_piece.type == EMPTY) {
             board_copy[move->to_r][move->to_c].type = EMPTY;
        }
    }
    if (info.castled_k) {
        board_copy[move->from_r][7] = board_copy[move->from_r][5]; board_copy[move->from_r][7].has_moved = false;
        board_copy[move->from_r][5].type = EMPTY;
    } else if (info.castled_q) {
        board_copy[move->from_r][0] = board_copy[move->from_r][3]; board_copy[move->from_r][0].has_moved = false;
        board_copy[move->from_r][3].type = EMPTY;
    }
    en_passant_target_r = info.old_ep_r; en_passant_target_c = info.old_ep_c;
}

int score_move_for_ordering(const Piece board[8][8], const AIMove* move) {
    int score = 0;
    Piece attacker = board[move->from_r][move->from_c];
    Piece victim = board[move->to_r][move->to_c];

    if (victim.type != EMPTY) {
        score = 10 * victim.type - attacker.type;
    }
    if (move->promotion_to == QUEEN) score += QUEEN_VALUE;
    
    return score;
}

void order_moves(const Piece board[8][8], AIMove legal_moves[], int num_legal_moves, int ply) {
    int move_scores[256];
    for (int i = 0; i < num_legal_moves; i++) {
        move_scores[i] = score_move_for_ordering(board, &legal_moves[i]);
        if (ply < MAX_SEARCH_PLY) {
            bool is_killer1 = (legal_moves[i].from_r == killer_moves[ply][0].from_r && legal_moves[i].to_r == killer_moves[ply][0].to_r && legal_moves[i].from_c == killer_moves[ply][0].from_c && legal_moves[i].to_c == killer_moves[ply][0].to_c);
            bool is_killer2 = (legal_moves[i].from_r == killer_moves[ply][1].from_r && legal_moves[i].to_r == killer_moves[ply][1].to_r && legal_moves[i].from_c == killer_moves[ply][1].from_c && legal_moves[i].to_c == killer_moves[ply][1].to_c);
            if (is_killer1) move_scores[i] += 10000;
            else if (is_killer2) move_scores[i] += 5000;
        }
    }

    for (int i = 0; i < num_legal_moves - 1; i++) {
        for (int j = i + 1; j < num_legal_moves; j++) {
            if (move_scores[j] > move_scores[i]) {
                int temp_score = move_scores[i];
                move_scores[i] = move_scores[j];
                move_scores[j] = temp_score;

                AIMove temp_move = legal_moves[i];
                legal_moves[i] = legal_moves[j];
                legal_moves[j] = temp_move;
            }
        }
    }
}

void store_killer_move(const AIMove* move, int ply) {
    if (ply >= MAX_SEARCH_PLY) return;
    if (!(move->from_r == killer_moves[ply][0].from_r && move->to_r == killer_moves[ply][0].to_r && move->from_c == killer_moves[ply][0].from_c && move->to_c == killer_moves[ply][0].to_c)) {
        killer_moves[ply][1] = killer_moves[ply][0];
        killer_moves[ply][0] = *move;
    }
}

#define MAX_QUIESCENCE_DEPTH 4
static int quiescence_search(Piece current_board_sim[8][8], int alpha, int beta, bool is_maximizing_player, PieceColor ai_color_perspective, int q_depth, int current_ply) {
    nodes_searched++;
    int stand_pat_score = ai_evaluate_board(current_board_sim, ai_color_perspective);
    if (q_depth >= MAX_QUIESCENCE_DEPTH) return stand_pat_score;

    PieceColor player_this_turn = is_maximizing_player ? ai_color_perspective : (ai_color_perspective == WHITE ? BLACK : WHITE);
    bool in_check = is_king_in_check(current_board_sim, player_this_turn);

    if (!in_check) {
        if (is_maximizing_player) { if (stand_pat_score >= beta) return beta; if (stand_pat_score > alpha) alpha = stand_pat_score; }
        else { if (stand_pat_score <= alpha) return alpha; if (stand_pat_score < beta) beta = stand_pat_score; }
    }

    AIMove q_moves[128]; int num_q_moves = 0;
    if (in_check) {
        num_q_moves = find_all_legal_ai_moves(current_board_sim, player_this_turn, q_moves, 128);
    } else {
        for (int r_f=0;r_f<8;++r_f) for(int c_f=0;c_f<8;++c_f) if(current_board_sim[r_f][c_f].type!=EMPTY && current_board_sim[r_f][c_f].color==player_this_turn) for(int r_t=0;r_t<8;++r_t) for(int c_t=0;c_t<8;++c_t) {
            bool is_cap = (current_board_sim[r_t][c_t].type!=EMPTY && current_board_sim[r_t][c_t].color!=player_this_turn);
            bool is_promo = (current_board_sim[r_f][c_f].type==PAWN && ((player_this_turn==WHITE && r_t==0)||(player_this_turn==BLACK && r_t==7)));
            if (is_cap || is_promo) {
                int oer=en_passant_target_r,oec=en_passant_target_c; if(is_move_legal(current_board_sim,r_f,c_f,r_t,c_t,player_this_turn)){ if(num_q_moves<128) q_moves[num_q_moves++]=(AIMove){r_f,c_f,r_t,c_t,is_promo?QUEEN:EMPTY,0};} en_passant_target_r=oer;en_passant_target_c=oec;
            }
        }
    }
    if (num_q_moves == 0) return stand_pat_score;
    order_moves(current_board_sim, q_moves, num_q_moves, current_ply + q_depth);

    if (is_maximizing_player) {
        int best_val = in_check ? INT_MIN : stand_pat_score;
        for (int i=0;i<num_q_moves;++i) {
            Piece cpy[8][8]; memcpy(cpy,current_board_sim,sizeof(Piece[8][8])); TempMoveInfo info = make_temporary_move(cpy,&q_moves[i],player_this_turn);
            int score = quiescence_search(cpy,alpha,beta,false,ai_color_perspective,q_depth+1, current_ply);
            undo_temporary_move(cpy,&q_moves[i],info);
            best_val=(score>best_val)?score:best_val; alpha=(score>alpha)?score:alpha; if(alpha>=beta)break;
        } return best_val;
    } else {
        int best_val = in_check ? INT_MAX : stand_pat_score;
        for (int i=0;i<num_q_moves;++i) {
            Piece cpy[8][8]; memcpy(cpy,current_board_sim,sizeof(Piece[8][8])); TempMoveInfo info = make_temporary_move(cpy,&q_moves[i],player_this_turn);
            int score = quiescence_search(cpy,alpha,beta,true,ai_color_perspective,q_depth+1, current_ply);
            undo_temporary_move(cpy,&q_moves[i],info);
            best_val=(score<best_val)?score:best_val; beta=(score<beta)?score:beta; if(alpha>=beta)break;
        } return best_val;
    }
}

static int minimax_ids(Piece board_sim[8][8], int depth, int alpha, int beta, bool is_max, PieceColor ai_color, int ply, Uint32 start_time, int time_limit_ms) {
    nodes_searched++;
    if (SDL_GetTicks() - start_time > time_limit_ms && depth < 2) {
        return ai_evaluate_board(board_sim, ai_color);
    }
    if (depth == 0) {
        return quiescence_search(board_sim, alpha, beta, is_max, ai_color, 0, ply);
    }

    PieceColor turn = is_max ? ai_color : (ai_color == WHITE ? BLACK : WHITE);
    if (!has_any_legal_moves(board_sim, turn)) {
        return ai_evaluate_board(board_sim, ai_color);
    }

    AIMove legal_moves[256];
    int num_legal_moves = find_all_legal_ai_moves(board_sim, turn, legal_moves, 256);
    order_moves(board_sim, legal_moves, num_legal_moves, ply);

    if (is_max) {
        int max_eval = INT_MIN;
        for (int i=0;i<num_legal_moves;++i) {
            Piece cpy[8][8]; memcpy(cpy,board_sim,sizeof(Piece[8][8])); TempMoveInfo info = make_temporary_move(cpy,&legal_moves[i],turn);
            int eval = minimax_ids(cpy,depth-1,alpha,beta,false,ai_color,ply+1,start_time,time_limit_ms);
            undo_temporary_move(cpy,&legal_moves[i],info);
            if(eval>max_eval) max_eval=eval; if(eval>alpha) alpha=eval;
            if(beta<=alpha) { if(board_sim[legal_moves[i].to_r][legal_moves[i].to_c].type == EMPTY) store_killer_move(&legal_moves[i], ply); break; }
        } return max_eval;
    } else {
        int min_eval = INT_MAX;
        for (int i=0;i<num_legal_moves;++i) {
            Piece cpy[8][8]; memcpy(cpy,board_sim,sizeof(Piece[8][8])); TempMoveInfo info = make_temporary_move(cpy,&legal_moves[i],turn);
            int eval = minimax_ids(cpy,depth-1,alpha,beta,true,ai_color,ply+1,start_time,time_limit_ms);
            undo_temporary_move(cpy,&legal_moves[i],info);
            if(eval<min_eval) min_eval=eval; if(eval<beta) beta=eval;
            if(beta<=alpha) { if(board_sim[legal_moves[i].to_r][legal_moves[i].to_c].type == EMPTY) store_killer_move(&legal_moves[i], ply); break; }
        } return min_eval;
    }
}

bool ai_select_move(const Piece board[8][8], PieceColor ai_player_color, AIMove* best_overall_move, int time_limit_ms) {
    AIMove legal_root_moves[256];
    int num_legal_root_moves = find_all_legal_ai_moves(board, ai_player_color, legal_root_moves, 256);

    if (num_legal_root_moves == 0) return false;

    *best_overall_move = legal_root_moves[0];
    int best_overall_score = INT_MIN;

    Uint32 search_start_time = SDL_GetTicks();
    printf("AI (%s) thinking...\n", ai_player_color == WHITE ? "W":"B");

    for (int current_depth = 1; current_depth <= MAX_SEARCH_PLY; ++current_depth) {
        nodes_searched = 0;
        int current_iteration_best_score = INT_MIN;
        AIMove current_iteration_best_move = legal_root_moves[0];
        
        order_moves(board, legal_root_moves, num_legal_root_moves, 0); // Order root moves based on previous iteration's scores

        for (int i = 0; i < num_legal_root_moves; ++i) {
            Piece board_after_ai_move[8][8]; memcpy(board_after_ai_move, board, sizeof(Piece[8][8]));
            TempMoveInfo root_move_info = make_temporary_move(board_after_ai_move, &legal_root_moves[i], ai_player_color);

            int score = minimax_ids(board_after_ai_move, current_depth-1, INT_MIN, INT_MAX, false, ai_player_color, 1, search_start_time, time_limit_ms);
            legal_root_moves[i].score = score; // Update score for this move

            undo_temporary_move(board_after_ai_move, &legal_root_moves[i], root_move_info);

            if (score > current_iteration_best_score) {
                current_iteration_best_score = score;
                current_iteration_best_move = legal_root_moves[i];
            }

            if (SDL_GetTicks() - search_start_time > time_limit_ms && i > 0) {
                goto end_ids_loop;
            }
        }
        
        *best_overall_move = current_iteration_best_move;
        best_overall_score = current_iteration_best_score;

        printf("  Depth %d complete. Best move: [%d,%d]->[%d,%d] Score: %d. Nodes: %d. Time: %.2fs\n",
               current_depth, best_overall_move->from_r, best_overall_move->from_c,
               best_overall_move->to_r, best_overall_move->to_c, best_overall_score,
               nodes_searched, (float)(SDL_GetTicks() - search_start_time) / 1000.0f);

        if (best_overall_score >= (KING_VALUE - 100) || best_overall_score <= (-KING_VALUE + 100) ) {
            printf("  Mate found or unavoidable.\n"); break; 
        }
        if (SDL_GetTicks() - search_start_time > time_limit_ms) {
             printf("  Time limit reached.\n"); break; 
        }
    }

end_ids_loop:;
    printf("AI chose final move: [%d,%d] to [%d,%d]", best_overall_move->from_r, best_overall_move->from_c, best_overall_move->to_r, best_overall_move->to_c);
    if(best_overall_move->promotion_to != EMPTY) printf(" (promo Q)");
    printf(" with final eval score: %d\n", best_overall_score);
    return true;
}