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

    GameState clone_game_state(GameState *game_state);
    int get_legal_moves(GameState *game_state, Move* valid_moves);
    void apply_move(GameState *game_state, Move *move);

#endif