//
// Created by Max on 18/12/2025.
//

#ifndef PROJETRESEAUX_GAME_H
#define PROJETRESEAUX_GAME_H

#include <stdint.h>
#include <stdlib.h>

// Forward declaration to avoid circular dependency
typedef struct Client Client;

#define ROW 6
#define COLUMN 8
#define MAX_LEGAL_MOVES 384 // 8 possible moves * 48 possible blocks 
#define DEFAULT_AI_DEPTH 40

typedef struct {
    int row;
    int col;
} Position;

typedef enum {
    PHASE_MOVE,
    PHASE_BLOCK
} GamePhase;

typedef struct GameState{
    uint8_t board[ROW][COLUMN];
    /*
     * 0 -> free
     * 1 -> player 1
     * 2 -> player 3
     * 3 -> destroyed
     */
    Position pos1;
    Position pos2;
    int current_turn; //1 or 2
} GameState;

typedef struct Game {
    GameState game_state;
    Client *player1;
    Client *player2;
    GamePhase phase;  //Added to track Move vs Block
    int game_mode;
    struct Game *next; //Pointer to the next game
} Game;

extern Game *games_head; //Head of the linked list

void init_game_board(Game *game);
Game* create_game(Client *p1, Client *p2);
void remove_game(Game *game);
int is_valid_move(GameState *game_state, int player_id, int new_row, int new_col);
void handle_move_request(Client *client, const uint8_t *body);
void handle_block_request(Client *client, const uint8_t *body);
int is_in_boundaries(int row, int col);
int tile_is_free(uint8_t board[ROW][COLUMN], int row, int col);

// End game management
int check_player_blocked(GameState *game_state, int player_id);
void end_game(Game *game, Client *winner, Client *loser, int is_forfeit);
void handle_forfeit(Client *disconnecting_client);
void finalize_turn(Game *game);

#endif