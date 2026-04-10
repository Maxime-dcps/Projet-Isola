//
// Created by Max on 18/12/2025.
//
#include "matchmaking.h"
#include "server.h"
#include "game.h"
#include "network_core.h"
#include "ai.h"

//Simple pointer to the waiting player
Client *waiting_client = NULL;

void handle_play_request(Client *client, const uint8_t *body) {
    //Check the client status
    if (client->state != AUTHENTICATED) return;

    // Extract game mode safely
    CPlayRequest play_rqst_pkt;
    memcpy(&play_rqst_pkt, body, sizeof(CPlayRequest));
    uint8_t game_mode = play_rqst_pkt.game_mode;

    if(game_mode != PLAYER_VS_AI && game_mode != PLAYER_VS_PLAYER) game_mode = PLAYER_VS_PLAYER;

    if(game_mode == PLAYER_VS_PLAYER)
    {
        printf("MATCHMAKING: requesting to play versus a player\n");
        
        if (waiting_client == NULL) {
            //The queue is empty, client is added to the waiting room
            waiting_client = client;
            client->state = IN_QUEUE;
    
            printf("MATCHMAKING: %s is now waiting for an opponent.\n", client->username);
    
            uint8_t status = 1;
            send_packet(client, S_WAITING_OPPONENT, &status, sizeof(uint8_t));
        }
        else {
            //Someone was waiting -> start the match
            Client *p1 = waiting_client;
            Client *p2 = client;
    
            waiting_client = NULL;
    
            start_match(p1, p2);
        }
    }
    else if(game_mode == PLAYER_VS_AI)
    {
        printf("MATCHMAKING: requesting to play versus the AI\n");

        Client *player = client;

        start_match(player, NULL); // Will initiate the AI_flag
    }

}

void start_match(Client *c1, Client *c2) {
    const char *p2_name = (c2 != NULL) ? c2->username : AI_NAME;
    printf("MATCHMAKING: Starting match between %s and %s!\n", c1->username, p2_name);

    Game *game = create_game(c1, c2);
    if (!game) {
        printf("ERROR: Failed to create game instance.\n");
        return;
    }

    c1->state = IN_GAME;
    c1->current_game = game;
    if (game->game_mode == PLAYER_VS_PLAYER)
    {
        // c2 is NULL if PLAYER_VS_AI
        c2->state = IN_GAME;
        c2->current_game = game;
    }

    uint8_t match_data_p1 = 1;  // Player 1 = RED
    uint8_t match_data_p2 = 2;  // Player 2 = BLUE
    /*
     * Flag for player ID
     * 1 -> You are Player 1 (RED)
     * 2 -> You are Player 2 (BLUE)
     */
    send_packet(c1, S_MATCH_FOUND, &match_data_p1, sizeof(uint8_t));
    if (game->game_mode == PLAYER_VS_PLAYER) 
    {
        // Only send packet if c2 is a player
        send_packet(c2, S_MATCH_FOUND, &match_data_p2, sizeof(uint8_t));
    }

    update_game_state(game);
}

void update_game_state(Game *game) {
    // Prepare packet for Player 1
    SGameState state1;
    memcpy(state1.board, game->game_state.board, BOARD_DATA_SIZE);
    // 0 = not your turn, 1 = your turn (MOVE phase), 2 = your turn (BLOCK phase)
    if (game->game_state.current_turn == 1) {
        state1.turn_player_id = (game->phase == PHASE_MOVE) ? 1 : 2;
    } else {
        state1.turn_player_id = 0;
    }
    send_packet(game->player1, S_GAME_STATE, &state1, sizeof(SGameState));

    if (game->game_mode == PLAYER_VS_PLAYER) 
    {
        // Prepare packet for Player 2 if not the AI
        SGameState state2;
        memcpy(state2.board, game->game_state.board, BOARD_DATA_SIZE);
        // 0 = not your turn, 1 = your turn (MOVE phase), 2 = your turn (BLOCK phase)
        if (game->game_state.current_turn == 2) {
            state2.turn_player_id = (game->phase == PHASE_MOVE) ? 1 : 2;
        } else {
            state2.turn_player_id = 0;
        }
        send_packet(game->player2, S_GAME_STATE, &state2, sizeof(SGameState));
    }
    else if (game->game_mode == PLAYER_VS_AI)
    {
        // If it's AI's turn
        if(game->game_state.current_turn == 2) 
        {
            // TODO: call Mr. Spock play function.
            Move ai_moves[MAX_LEGAL_MOVES];
            int count = get_legal_moves(&game->game_state, ai_moves);
            printf("DEBUG: AI has %d legal moves available.\n", count);
            // Reduce AI depth to speed up the process or to lower difficulty
            Move best_move = get_best_move(&game->game_state, DEFAULT_AI_DEPTH, 2); 
            printf("DEBUG: AI chose move - Dest: [%d, %d], Block: [%d, %d]\n", best_move.dest_row, best_move.dest_col, best_move.block_row, best_move.block_col);

            apply_move(&game->game_state, best_move);

            //Switch player and phase
            game->game_state.current_turn = 1;
            game->phase = PHASE_MOVE;

            // Check if any player is blocked after AI move
            finalize_turn(game);
        }
    }
}