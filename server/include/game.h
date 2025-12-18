//
// Created by Max on 18/12/2025.
//

#ifndef PROJETRESEAUX_GAME_H
#define PROJETRESEAUX_GAME_H

#include "network_core.h"

#define ROW 6
#define COLUMN 8

typedef struct {
    int row;
    int col;
} PlayerPos;

typedef struct Game {
    uint8_t board[ROW][COLUMN];
    /*
     * 0 -> free
     * 1 -> player 1
     * 2 -> player 3
     * 3 -> destroyed
     */
    Client *player1;
    Client *player2;
    PlayerPos pos1;
    PlayerPos pos2;
    int current_turn; //1 or 2
    struct Game *next; //Pointer to the next game
} Game;

extern Game *games_head; //Head of the linked list

void init_game_board(Game *game);
Game* create_game(Client *p1, Client *p2);
void remove_game(Game *game);

#endif