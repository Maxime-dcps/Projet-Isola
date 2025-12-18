//
// Created by Max on 18/12/2025.
//
#include "matchmaking.h"

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

    c1->state = IN_GAME;
    c2->state = IN_GAME;

    // TODO: Init Game

    // Notify clients match found
    uint8_t match_data = 1;
    send_packet(c1, S_MATCH_FOUND, &match_data, sizeof(uint8_t));
    send_packet(c2, S_MATCH_FOUND, &match_data, sizeof(uint8_t));
}