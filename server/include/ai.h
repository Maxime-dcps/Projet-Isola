//
// Created by Max on 09/04/2026.
//

#ifndef PROJETRESEAUX_AI_H
#define PROJETRESEAUX_AI_H

#include "game.h"
#include <string.h>

    typedef struct Move{
        uint8_t dest_row;
        uint8_t dest_col;
        uint8_t block_row;
        uint8_t block_col;
    } Move;

    // FIXME: needs to be able to compete multiple ai games at the same time
    // Multithreading will be needed
    GameState clone_game_state(GameState *game_state);
    int get_legal_moves(GameState *game_state, Move* valid_moves);
    void apply_move(GameState *game_state, Move move);
    int evaluate_board(GameState *game_state, int ai_id);
    int count_free_tiles(GameState *game_state, int player_id);
    int is_game_over(GameState *game_state);
    int max_value(GameState *game_state, int depth, int ai_id);
    int min_value(GameState *game_state, int depth, int ai_id);

#endif