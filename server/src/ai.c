#include "ai.h"

GameState clone_game_state(GameState *game_state)
{
    GameState clone;
    memcpy(&clone, game_state, sizeof(GameState));
    return clone;
}

int get_legal_moves(GameState *game_state, Move* valid_moves)
{
    int count = 0;
    Position current = (game_state->current_turn == 1) ? game_state->pos1 : game_state->pos2;

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue; // Skip current position

            int move_row = current.row + dr;
            int move_col = current.col + dc;

            if(is_valid_move(game_state, game_state->current_turn, move_row, move_col)) {
                // For each valid move, we can block any free tile on the board
                for (int br = 0; br < ROW; br++) {
                    for (int bc = 0; bc < COLUMN; bc++) {
                        if (br == 2 && bc == 0 || br == 3 && bc == 7) continue; // Starting positions can't be blocked
                        if (br == move_row && bc == move_col) continue; // Can't block tile we just moved to 
                        if (game_state->board[br][bc] == 0 || br == current.row && bc == current.col) { // If tile is free
                            valid_moves[count].dest_row = move_row;
                            valid_moves[count].dest_col = move_col;
                            valid_moves[count].block_row = br;
                            valid_moves[count].block_col = bc;
                            count++;
                        }
                    }
                }
            }
        }
    }

    return count;
}

void apply_move(GameState *game_state, Move move)
{
    // Move player
    Position *current_pos = (game_state->current_turn == 1) ? &game_state->pos1 : &game_state->pos2;
    game_state->board[current_pos->row][current_pos->col] = 0; // Clear old position
    current_pos->row = move.dest_row;
    current_pos->col = move.dest_col;
    game_state->board[move.dest_row][move.dest_col] = game_state->current_turn; // Set new position

    // Block tile
    game_state->board[move.block_row][move.block_col] = 3; // Mark as destroyed

    // Switch turn
    game_state->current_turn = (game_state->current_turn == 1) ? 2 : 1;
}

int evaluate_board(GameState *game_state, int depth, int ai_id)
{
    int score = 0;
    int human_id = (ai_id == 2) ? 1 : 2;

    if(check_player_blocked(game_state, human_id)) return 1000 + depth;
    if(check_player_blocked(game_state, ai_id)) return -1000 - depth;

    int mobility = count_free_tiles(game_state, ai_id) - count_free_tiles(game_state, human_id);
    score += mobility;

    return score;
}

int count_free_tiles(GameState *game_state, int player_id)
{
    int count = 0;

    Position playerPos = (player_id == 1) ? game_state->pos1 : game_state->pos2;

    for (int dr = -1; dr <= 1; dr++)
    {
        for (int dc = -1; dc <= 1; dc++)
        {
            if (dr == 0 && dc == 0) continue;

            if (tile_is_free(game_state->board, playerPos.row + dr, playerPos.col + dc)) count++;
        }
    }

    return count;
}

int is_game_over(GameState *game_state)
{
    return (check_player_blocked(game_state, 1) || check_player_blocked(game_state, 2));
}

int max_value(GameState *game_state, int depth, AISearchContext *ctx, int alpha, int beta)
{
    // We reach a leaf
    if(depth == 0 || is_game_over(game_state)) 
        return evaluate_board(game_state, depth, ctx->ai_id);
    
    if (is_time_up(ctx)) {
        ctx->time_expired = 1;
        return evaluate_board(game_state, depth, ctx->ai_id);
    }

    Move valid_moves[MAX_LEGAL_MOVES];

    int moves_count = get_legal_moves(game_state, valid_moves);
    int best_val = -100000;
    int score;

    for(int i = 0; i < moves_count; i++)
    {
        GameState child_state = clone_game_state(game_state);
        apply_move(&child_state, valid_moves[i]);

        // Create a new branch
        score = min_value(&child_state, depth - 1, ctx, alpha, beta);

        if(ctx->time_expired) break;

        // Update best_val if this move is better
        if(score > best_val) best_val = score;
        if(best_val > alpha) alpha = best_val;
        if(alpha >= beta) break; // Prune branch
    }

    return best_val;
}

int min_value(GameState *game_state, int depth, AISearchContext *ctx, int alpha, int beta)
{
    // We reach a leaf
    if(depth == 0 || is_game_over(game_state)) 
        return evaluate_board(game_state, depth, ctx->ai_id);

    if (is_time_up(ctx)) {
        ctx->time_expired = 1;
        return evaluate_board(game_state, depth, ctx->ai_id);
    }
    
    Move valid_moves[MAX_LEGAL_MOVES];

    int moves_count = get_legal_moves(game_state, valid_moves);
    int best_val = 100000;
    int score;

    for(int i = 0; i < moves_count; i++)
    {
        GameState child_state = clone_game_state(game_state);
        apply_move(&child_state, valid_moves[i]);

        // Create a new branch
        score = max_value(&child_state, depth - 1, ctx, alpha, beta);

        if(ctx->time_expired) break;

        // Update best_val if this move is better
        if(score < best_val) best_val = score;
        if(best_val < beta) beta = best_val; 
        if(alpha >= beta) break; // Prune branch
    }

    return best_val;
}

Move search_at_depth(GameState *game_state, int depth, AISearchContext *ctx, int *final_val)
{
    Move best_move, valid_moves[MAX_LEGAL_MOVES];

    int moves_count = get_legal_moves(game_state, valid_moves);
    int score, best_val = -100000, alpha = -100000, beta = 100000;

    for(int i = 0; i < moves_count; i++)
    {
        GameState child_state = clone_game_state(game_state);
        apply_move(&child_state, valid_moves[i]);

        // Evaluate move using Minimax with alpha-beta pruning
        score = min_value(&child_state, depth - 1, ctx, alpha, beta);

        if(ctx->time_expired) break;

        // Update best_val and best move if this move is better
        if(score > best_val)
        {
            best_move = valid_moves[i];
            best_val = score;
        }

        // Update alpha for pruning
        if(best_val > alpha) alpha = best_val;
    }

    *final_val = best_val;
    return best_move;
}

Move get_best_move(GameState *game_state, int max_depth, int ai_id)
{
    // Initialize search context
    AISearchContext ctx;
    ctx.ai_id = ai_id;
    ctx.time_limit_ms = TIME_LIMIT_MS;
    ctx.time_expired = 0;
    clock_gettime(CLOCK_MONOTONIC, &ctx.start_time);
    
    Move valid_moves[MAX_LEGAL_MOVES];
    get_legal_moves(game_state, valid_moves);
    Move best_move_overall = valid_moves[0];

    int best_val;
    
    for (int depth = 1; depth <= max_depth; depth++)
    {
        Move candidate = search_at_depth(game_state, depth, &ctx, &best_val);
        
        if (!ctx.time_expired) {
            // Update if we completed the search within the time limit
            best_move_overall = candidate;
            printf("AI: depth %d complete\n", depth);
        } else {
            // we keep the best move from the last completed depth
            printf("AI: timeout at depth %d, using depth %d\n", depth, depth - 1);
            break;
        }

        if(best_val >= 1000) {
            // Found a winning move, no need to search deeper
            printf("AI: found winning move at depth %d\n", depth);
            break;
        }
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    printf("AI: selected move after searching up to depth %d in %ld seconds\n", max_depth, get_duration(ctx.start_time, now));

    return best_move_overall;
}

int is_time_up(AISearchContext *ctx) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long elapsed_ms = get_duration(ctx->start_time, now);
    return elapsed_ms >= ctx->time_limit_ms;
}

long get_duration(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
}