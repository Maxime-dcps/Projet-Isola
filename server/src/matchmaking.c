//
// Created by Max on 18/12/2025.
//
#include "matchmaking.h"
#include "server.h"
#include "game.h"
#include "network_core.h"

//Simple pointer to the waiting player
Client *waiting_client = NULL;

void handle_play_request(Client *client) {
    //Check the client status
    if (client->state != AUTHENTICATED) return;

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

void start_match(Client *c1, Client *c2) {
    printf("MATCHMAKING: Starting match between %s and %s!\n", c1->username, c2->username);

    Game *game = create_game(c1, c2);
    if (!game) {
        printf("ERROR: Failed to create game instance.\n");
        return;
    }

    c1->state = IN_GAME;
    c2->state = IN_GAME;

    uint8_t match_data = 1;
    /*
     * Flag for player turn
     * 1 -> It's your turn
     * 0 -> It's your opponent turn
     */
    send_packet(c1, S_MATCH_FOUND, &match_data, sizeof(uint8_t));
    send_packet(c2, S_MATCH_FOUND, &match_data, sizeof(uint8_t));

    update_game_state(c1, c2, game);
}

void update_game_state(Client *c1, Client *c2, Game *game) {
    // Prepare packet for Player 1
    SGameState state1;
    memcpy(state1.board, game->board, BOARD_DATA_SIZE);
    state1.turn_player_id = (game->current_turn == 1); //If not your turn = 0
    send_packet(c1, S_GAME_STATE, &state1, sizeof(SGameState));

    // Prepare packet for Player 2
    SGameState state2;
    memcpy(state2.board, game->board, BOARD_DATA_SIZE);
    state2.turn_player_id = (game->current_turn == 2); //If your turn = 1
    send_packet(c2, S_GAME_STATE, &state2, sizeof(SGameState));
}