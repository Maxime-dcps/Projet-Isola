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

void apply_move(GameState *game_state, Move *move)
{
    // Move player
    Position *current_pos = (game_state->current_turn == 1) ? &game_state->pos1 : &game_state->pos2;
    game_state->board[current_pos->row][current_pos->col] = 0; // Clear old position
    current_pos->row = move->dest_row;
    current_pos->col = move->dest_col;
    game_state->board[move->dest_row][move->dest_col] = game_state->current_turn; // Set new position

    // Block tile
    game_state->board[move->block_row][move->block_col] = 3; // Mark as destroyed

    // Switch turn
    game_state->current_turn = (game_state->current_turn == 1) ? 2 : 1;
}

int evaluate_board(GameState *game_state, int ai_id)
{
    int score = 0;
    int human_id = (ai_id == 2) ? 1 : 2;

    if(check_player_blocked(game_state, human_id)) return 1000;
    if(check_player_blocked(game_state, ai_id)) return -1000;

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