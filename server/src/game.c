//
// Created by Max on 18/12/2025.
//
#include "game.h"
#include "server.h"
#include "network_core.h"

Game *games_head = NULL;

Game* create_game(Client *p1, Client *p2) {
    Game *new_game = (Game *)malloc(sizeof(Game));
    if (!new_game) return NULL;

    new_game->player1 = p1;
    new_game->player2 = p2;
    new_game->current_turn = 1; //TODO: Random player starts

    new_game->phase = PHASE_MOVE;

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

void handle_move_request(Client *client, const uint8_t *body) {
    Game *game = (Game *)client->current_game;
    if (!game) return;

    //Is the player turn
    int player_id = (client == game->player1) ? 1 : 2;

    if (game->current_turn != player_id || game->phase != PHASE_BLOCK) {
        return;
    }

    CMovePawn *move_pkt = (CMovePawn *)body;
    uint8_t data = move_pkt->to_pos_encoded;

    //Extract Row (RRR) and Column (CCC) using bit shifting and masking
    //RRR: Shift right by 5 to get the 3 top bits
    int new_row = (data >> 5) & 0x07;
    //CCC: Shift right by 2 to get the middle 3 bits, then mask
    int new_col = (data >> 2) & 0x07;

    if (is_valid_move(game, player_id, new_row, new_col)) {
        //Update positions on board
        PlayerPos *old_pos = (player_id == 1) ? &game->pos1 : &game->pos2;
        game->board[old_pos->row][old_pos->col] = 0; //Clear old tile
        //Update player position
        old_pos->row = new_row;
        old_pos->col = new_col;
        game->board[new_row][new_col] = player_id; //Set new tile

        //Switch to BLOCK phase
        game->phase = PHASE_BLOCK;

        printf("GAME: %s moved successfully. Waiting for block.\n", client->username);

        update_game_state(game->player1, game->player2, game);
    }
}

void handle_block_request(Client *client, const uint8_t *body) {
    Game *game = (Game *)client->current_game;
    if (!game) return;

    int player_id = (client == game->player1) ? 1 : 2;

    //Verify turn and phase
    if (game->current_turn != player_id || game->phase != PHASE_BLOCK) {
        return;
    }

    CBlockTile *block_pkt = (CBlockTile *)body;
    uint8_t data = block_pkt->block_pos_encoded;
    int row = (data >> 5) & 0x07;
    int col = (data >> 2) & 0x07;

    // Validation: tile must be empty (0)
    if (row < ROW && col < COLUMN && row >= 0 && col >= 0 && game->board[row][col] == 0) {
        //Set tile to destroyed
        game->board[row][col] = 3;

        //Switch player and phase
        game->current_turn = (game->current_turn == 1) ? 2 : 1;
        game->phase = PHASE_MOVE;

        printf("GAME: %s blocked tile [%d, %d]. Next turn: Player %d\n", client->username, row, col, game->current_turn);

        update_game_state(game->player1, game->player2, game);
    }
}