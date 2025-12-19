//
// Created by Max on 18/12/2025.
//
#include "game.h"
#include "server.h"
#include "network_core.h"
#include "sqlite_utils.h"
#include "protocol.h"

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

    if (game->current_turn != player_id || game->phase != PHASE_MOVE) {
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

    if((row == 2 && col == 0) || (row == 3 && col == 7)) return;

    // Validation: tile must be empty (0)
    if (row < ROW && col < COLUMN && row >= 0 && col >= 0 && game->board[row][col] == 0) {
        //Set tile to destroyed
        game->board[row][col] = 3;

        //Switch player and phase
        int next_player = (game->current_turn == 1) ? 2 : 1;
        game->current_turn = next_player;
        game->phase = PHASE_MOVE;

        printf("GAME: %s blocked tile [%d, %d]. Next turn: Player %d\n", client->username, row, col, game->current_turn);

        // Check if next player is blocked (victory condition)
        if (check_player_blocked(game, next_player)) {
            Client *winner = (next_player == 1) ? game->player2 : game->player1;
            Client *loser = (next_player == 1) ? game->player1 : game->player2;
            printf("GAME: Player %d is blocked! %s wins!\n", next_player, winner->username);
            end_game(game, winner, loser, 0);
        } else {
            update_game_state(game->player1, game->player2, game);
        }
    }
}

// Check if a player has at least one valid move
int check_player_blocked(Game *game, int player_id) {
    PlayerPos pos = (player_id == 1) ? game->pos1 : game->pos2;

    // Check all 8 adjacent tiles
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue; // Skip current position

            int new_row = pos.row + dr;
            int new_col = pos.col + dc;

            // Check boundaries
            if (new_row < 0 || new_row >= ROW || new_col < 0 || new_col >= COLUMN) continue;

            // If any adjacent tile is empty (0), player can move
            if (game->board[new_row][new_col] == 0) {
                return 0; // Not blocked
            }
        }
    }
    return 1; // Blocked - no valid moves
}

// End the game, update stats, notify clients, cleanup
void end_game(Game *game, Client *winner, Client *loser, int is_forfeit) {
    if (!game || !winner || !loser) return;

    // Update database stats
    if (is_forfeit) {
        // Loser gets a forfeit
        update_user_stats(db_conn, winner->username, 1, 0, 0); // Win
        update_user_stats(db_conn, loser->username, 0, 0, 1);  // Forfeit
    } else {
        // Normal game end
        update_user_stats(db_conn, winner->username, 1, 0, 0); // Win
        update_user_stats(db_conn, loser->username, 0, 1, 0);  // Loss
    }

    // Get updated stats for both players
    User winner_stats, loser_stats;
    get_user(db_conn, winner->username, &winner_stats);
    get_user(db_conn, loser->username, &loser_stats);

    // Prepare and send S_GAME_OVER to winner
    SGameOver winner_pkt;
    winner_pkt.result = is_forfeit ? GAME_RESULT_FORFEIT_WIN : GAME_RESULT_WIN;
    winner_pkt.wins = htons(winner_stats.wins);
    winner_pkt.losses = htons(winner_stats.losses);
    winner_pkt.forfeits = htons(winner_stats.forfeits);
    send_packet(winner, S_GAME_OVER, &winner_pkt, sizeof(SGameOver));

    // Prepare and send S_GAME_OVER to loser
    SGameOver loser_pkt;
    loser_pkt.result = is_forfeit ? GAME_RESULT_FORFEIT_LOSS : GAME_RESULT_LOSS;
    loser_pkt.wins = htons(loser_stats.wins);
    loser_pkt.losses = htons(loser_stats.losses);
    loser_pkt.forfeits = htons(loser_stats.forfeits);
    send_packet(loser, S_GAME_OVER, &loser_pkt, sizeof(SGameOver));

    printf("GAME: Game ended. Winner: %s, Loser: %s (forfeit: %d)\n", 
           winner->username, loser->username, is_forfeit);

    // Reset client states
    winner->state = AUTHENTICATED;
    winner->current_game = NULL;
    loser->state = AUTHENTICATED;
    loser->current_game = NULL;

    // Remove game from list and free memory
    remove_game(game);
}

// Handle forfeit when a player disconnects during a game
void handle_forfeit(Client *disconnecting_client) {
    if (!disconnecting_client || !disconnecting_client->current_game) return;

    Game *game = (Game *)disconnecting_client->current_game;
    
    Client *winner = (disconnecting_client == game->player1) ? game->player2 : game->player1;
    Client *loser = disconnecting_client;

    // Update stats - only for the winner since loser is disconnecting
    update_user_stats(db_conn, winner->username, 1, 0, 0); // Win
    update_user_stats(db_conn, loser->username, 0, 0, 1);  // Forfeit

    // Get updated stats and notify winner
    User winner_stats;
    get_user(db_conn, winner->username, &winner_stats);

    SGameOver winner_pkt;
    winner_pkt.result = GAME_RESULT_FORFEIT_WIN;
    winner_pkt.wins = htons(winner_stats.wins);
    winner_pkt.losses = htons(winner_stats.losses);
    winner_pkt.forfeits = htons(winner_stats.forfeits);
    send_packet(winner, S_GAME_OVER, &winner_pkt, sizeof(SGameOver));

    printf("GAME: %s forfeited. %s wins by forfeit.\n", loser->username, winner->username);

    // Reset winner state
    winner->state = AUTHENTICATED;
    winner->current_game = NULL;

    // Clear loser's game pointer (will be freed in end_session)
    loser->current_game = NULL;

    // Remove game from list and free memory
    remove_game(game);
}