//
// Created by Max on 18/12/2025.
//
#include "game.h"

Game *games_head = NULL;

Game* create_game(Client *p1, Client *p2) {
    Game *new_game = (Game *)malloc(sizeof(Game));
    if (!new_game) return NULL;

    new_game->player1 = p1;
    new_game->player2 = p2;
    new_game->current_turn = 1; //TODO: Random player starts

    init_game_board(new_game);

    //Link into active games list
    new_game->next = games_head;
    games_head = new_game;

    return new_game;
}

void init_game_board(Game *game) {
    //Reset board with zeros
    for (int i = 0; i < ROW; i++) {
        for (int j = 0; j < COLUMN; j++) {
            game->board[i][j] = 0;
        }
    }

    //Setting players pos
    game->pos1.row = 2;
    game->pos1.col = 0;

    game->pos2.row = 3;
    game->pos2.col = 7;

    game->board[game->pos1.row][game->pos1.col] = 1;
    game->board[game->pos2.row][game->pos2.col] = 2;
}

void remove_game(Game *game_to_remove) {
    if (!game_to_remove) return;

    Game **ptr = &games_head;
    while (*ptr != NULL && *ptr != game_to_remove) {
        ptr = &(*ptr)->next;
    }

    if (*ptr == game_to_remove) {
        *ptr = game_to_remove->next;
        free(game_to_remove);
        printf("DEBUG: Game memory freed.\n");
    }
}

int is_valid_move(Game *game, int player_id, int new_row, int new_col) {
    PlayerPos current = (player_id == 1) ? game->pos1 : game->pos2;

    // Check boundaries
    if (new_row < 0 || new_row >= ROW || new_col < 0 || new_col >= COLUMN) return 0;

    // Check if it's an adjacent tile
    int dr = abs(new_row - current.row); //Absolute value =>
    int dc = abs(new_col - current.col);
    if (dr > 1 || dc > 1 || (dr == 0 && dc == 0)) return 0;

    // Check if the tile is empty (0)
    if (game->board[new_row][new_col] != 0) return 0;

    return 1;
}