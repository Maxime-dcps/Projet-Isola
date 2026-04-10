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

    new_game->game_mode = (p2 != NULL) ? PLAYER_VS_PLAYER : PLAYER_VS_AI;

    new_game->player1 = p1;
    new_game->player2 = p2;
    new_game->game_state.current_turn = 1; //TODO: Random player starts. Carefull with order ! OK for AI to start ?

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
            game->game_state.board[i][j] = 0;
        }
    }

    //Setting players pos
    game->game_state.pos1.row = 2;
    game->game_state.pos1.col = 0;

    game->game_state.pos2.row = 3;
    game->game_state.pos2.col = 7;

    game->game_state.board[game->game_state.pos1.row][game->game_state.pos1.col] = 1;
    game->game_state.board[game->game_state.pos2.row][game->game_state.pos2.col] = 2;
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

int is_valid_move(GameState *game_state, int player_id, int new_row, int new_col) {
    Position current = (player_id == 1) ? game_state->pos1 : game_state->pos2;

    // Check boundaries
    if (!is_in_boundaries(new_row, new_col)) return 0; 

    // Check if it's an adjacent tile
    int dr = abs(new_row - current.row); //Absolute value =>
    int dc = abs(new_col - current.col);
    if (dr > 1 || dc > 1 || (dr == 0 && dc == 0)) return 0;

    // Check if the tile is empty (0)
    if (game_state->board[new_row][new_col] != 0) return 0;

    return 1;
}

void handle_move_request(Client *client, const uint8_t *body) {
    Game *game = (Game *)client->current_game;
    if (!game) return;

    //Is the player turn
    int player_id = (client == game->player1) ? 1 : 2;

    if (game->game_state.current_turn != player_id || game->phase != PHASE_MOVE) {
        return;
    }

    CMovePawn *move_pkt = (CMovePawn *)body;
    uint8_t data = move_pkt->to_pos_encoded;

    //Extract Row (RRR) and Column (CCC) using bit shifting and masking
    //RRR: Shift right by 5 to get the 3 top bits
    int new_row = (data >> 5) & 0x07;
    //CCC: Shift right by 2 to get the middle 3 bits, then mask
    int new_col = (data >> 2) & 0x07;

    if (is_valid_move(&game->game_state, player_id, new_row, new_col)) {
        //Update positions on board
        Position *old_pos = (player_id == 1) ? &game->game_state.pos1 : &game->game_state.pos2;
        game->game_state.board[old_pos->row][old_pos->col] = 0; //Clear old tile
        //Update player position
        old_pos->row = new_row;
        old_pos->col = new_col;
        game->game_state.board[new_row][new_col] = player_id; //Set new tile

        //Switch to BLOCK phase
        game->phase = PHASE_BLOCK;

        printf("GAME: %s moved successfully. Waiting for block.\n", client->username);

        update_game_state(game);
    }
}

void handle_block_request(Client *client, const uint8_t *body) {
    Game *game = (Game *)client->current_game;
    if (!game) return;

    int player_id = (client == game->player1) ? 1 : 2;

    //Verify turn and phase
    if (game->game_state.current_turn != player_id || game->phase != PHASE_BLOCK) {
        return;
    }

    CBlockTile *block_pkt = (CBlockTile *)body;
    uint8_t data = block_pkt->block_pos_encoded;
    int row = (data >> 5) & 0x07;
    int col = (data >> 2) & 0x07;

    if((row == 2 && col == 0) || (row == 3 && col == 7)) return;

    // Validation: tile must be empty (0)
    if (row < ROW && col < COLUMN && row >= 0 && col >= 0 && game->game_state.board[row][col] == 0) {
        //Set tile to destroyed
        game->game_state.board[row][col] = 3;

        //Switch player and phase
        int next_player = (game->game_state.current_turn == 1) ? 2 : 1;
        game->game_state.current_turn = next_player;
        game->phase = PHASE_MOVE;

        printf("GAME: %s blocked tile [%d, %d]. Next turn: Player %d\n", client->username, row, col, game->game_state.current_turn);

        // Check if any player is blocked 
        finalize_turn(game);
    }
}

int is_in_boundaries(int row, int col) 
{
    return (row >= 0 && row < ROW && col >= 0 && col < COLUMN);

}

int tile_is_free(uint8_t board[ROW][COLUMN], int row, int col)
{
    return is_in_boundaries(row, col) && board[row][col] == 0;
}

// Check if a player has at least one valid move
int check_player_blocked(GameState *game_state, int player_id) {
    Position pos = (player_id == 1) ? game_state->pos1 : game_state->pos2;

    // Check all 8 adjacent tiles
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue; // Skip current position

            int new_row = pos.row + dr;
            int new_col = pos.col + dc;

            // Check boundaries
            if (!is_in_boundaries(new_row, new_col)) continue;

            // If any adjacent tile is empty (0), player can move
            if (game_state->board[new_row][new_col] == 0) {
                return 0; // Not blocked
            }
        }
    }
    return 1; // Blocked - no valid moves
}

// End the game, update stats, notify clients, cleanup
void end_game(Game *game, Client *winner, Client *loser, int is_forfeit) {
    if (!game) return;

    // apply_game_result(winner, );

    // Update database stats
    if (is_forfeit) {
        if (winner) update_user_stats(db_conn, winner->username, 1, 0, 0); // Win
        if (loser)  update_user_stats(db_conn, loser->username,  0, 0, 1); // Forfeit
    } else {
        if (winner) update_user_stats(db_conn, winner->username, 1, 0, 0); // Win
        if (loser)  update_user_stats(db_conn, loser->username,  0, 1, 0); // Loss
    }

    // Send S_GAME_OVER to winner (if human)
    if (winner) {
        User winner_stats;
        get_user(db_conn, winner->username, &winner_stats);

        // Prepare and send S_GAME_OVER to winner
        SGameOver winner_pkt;
        winner_pkt.result = is_forfeit ? GAME_RESULT_FORFEIT_WIN : GAME_RESULT_WIN;
        winner_pkt.wins = htons(winner_stats.wins);
        winner_pkt.losses = htons(winner_stats.losses);
        winner_pkt.forfeits = htons(winner_stats.forfeits);
        send_packet(winner, S_GAME_OVER, &winner_pkt, sizeof(SGameOver));
    }

    // Prepare and send S_GAME_OVER to loser
    if (loser) {
        User loser_stats;
        get_user(db_conn, loser->username, &loser_stats);

        SGameOver loser_pkt;
        loser_pkt.result = is_forfeit ? GAME_RESULT_FORFEIT_LOSS : GAME_RESULT_LOSS;
        loser_pkt.wins = htons(loser_stats.wins);
        loser_pkt.losses = htons(loser_stats.losses);
        loser_pkt.forfeits = htons(loser_stats.forfeits);
        send_packet(loser, S_GAME_OVER, &loser_pkt, sizeof(SGameOver));
    }

    const char *winner_name = (winner != NULL) ? winner->username : AI_NAME;
    const char *loser_name  = (loser  != NULL) ? loser->username  : AI_NAME;
    printf("GAME: Game ended. Winner: %s, Loser: %s (forfeit: %d)\n", winner_name, loser_name, is_forfeit);

    // Reset client states
    if (winner) {
        winner->state = AUTHENTICATED;
        winner->current_game = NULL;
    }
    if (loser) {
        loser->state = AUTHENTICATED;
        loser->current_game = NULL;
    }

    // Remove game from list and free memory
    remove_game(game);
}

// Handle forfeit when a player disconnects during a game
void handle_forfeit(Client *disconnecting_client) {
    if (!disconnecting_client || !disconnecting_client->current_game) return;

    Game *game = (Game *)disconnecting_client->current_game;
    
    Client *winner = (disconnecting_client == game->player1) ? game->player2 : game->player1;
    Client *loser = disconnecting_client;

    // Update stats 
    if (winner) update_user_stats(db_conn, winner->username, 1, 0, 0); // Win
    if (loser)  update_user_stats(db_conn, loser->username,  0, 0, 1); // Forfeit

    // Notify only winner
    if (winner) {
        User winner_stats;
        get_user(db_conn, winner->username, &winner_stats);

        SGameOver winner_pkt;
        winner_pkt.result = GAME_RESULT_FORFEIT_WIN;
        winner_pkt.wins = htons(winner_stats.wins);
        winner_pkt.losses = htons(winner_stats.losses);
        winner_pkt.forfeits = htons(winner_stats.forfeits);
        send_packet(winner, S_GAME_OVER, &winner_pkt, sizeof(SGameOver));

        winner->state = AUTHENTICATED;
        winner->current_game = NULL;
    }

    // Clear loser's game pointer (will be freed in end_session)
    loser->current_game = NULL;

    // Remove game from list and free memory
    remove_game(game);
}

void finalize_turn(Game *game) {
    if (check_player_blocked(&game->game_state, 1)) {
        Client *winner = game->player2;
        Client *loser = game->player1;

        const char *winner_name = (winner != NULL) ? winner->username : AI_NAME;
        const char *loser_name  = (loser  != NULL) ? loser->username  : AI_NAME;

        printf("GAME: %s is blocked! %s wins!\n", loser_name, winner_name);
        end_game(game, winner, loser, 0);
        return;
    }

    if (check_player_blocked(&game->game_state, 2)) {
        Client *winner = game->player1;
        Client *loser = game->player2;
        const char *winner_name = (winner != NULL) ? winner->username : AI_NAME;
        const char *loser_name  = (loser  != NULL) ? loser->username  : AI_NAME;

        printf("GAME: %s is blocked! %s wins!\n", loser_name, winner_name);
        end_game(game, winner, loser, 0);
        return;
    }

    // If no winner, just update game state for next turn
    update_game_state(game);
}