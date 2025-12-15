//
// Created by Max on 07/12/2025.
//

#ifndef PROJETRESEAUX_CONFIG_H
#define PROJETRESEAUX_CONFIG_H

    #define SERVER_PORT 55555
    #define SQLITE_DB_NAME "isola_profiles.db"

    #define MAX_USERNAME_LEN 16
    #define AUTH_HASH_LEN 32
    #define HEADER_SIZE 4        //Message header size (ID 1, reserved 1, length of content 2)
    #define BOARD_DATA_SIZE 12   //6x8 * 2 bits/tile = 12 bytes

    typedef enum {
        //Client -> server
        C_AUTH_CHALLENGE = 1,
        C_PLAY_REQUEST = 2,
        C_MOVE_PAWN = 3,
        C_BLOCK_TILE = 4,
        C_DISCONNECT = 5,

        //Server -> client
        S_AUTH_RESPONSE = 6,
        S_LOBBY_LIST = 7,
        S_WAITING_OPPONENT = 8,
        S_MATCH_FOUND = 9,
        S_GAME_STATE = 10,
        S_YOUR_TURN = 11,
        S_INVALID_MOVE = 12,
        S_GAME_OVER = 13,
        S_OPPONENT_DISCONNECTED = 14,

        S_STATUS_FAIL = 15,
        S_STATUS_SUCCESS = 16,
        S_STATUS_CREATED = 17

    } CommandID;

#endif