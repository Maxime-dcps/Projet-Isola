//
// Created by Max on 18/12/2025.
//

#ifndef PROJETRESEAUX_MATCHMAKING_H
#define PROJETRESEAUX_MATCHMAKING_H
    #include "network_core.h"
    #include "game.h"

    extern Client *waiting_client;
    //Handle C_PLAY_REQUEST
    void handle_play_request(Client *client);

    void start_match(Client *c1, Client *c2);
    void update_game_state(Client *c1, Client *c2, Game *game);

#endif