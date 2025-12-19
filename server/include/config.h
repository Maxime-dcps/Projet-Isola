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
    #define BOARD_DATA_SIZE 48   //6x8 * 1 byte/tile = 48 bytes

    typedef enum {
        //Client -> server
        C_AUTH_CHALLENGE = 1,
        C_PLAY_REQUEST = 2,
        C_MOVE_PAWN = 3,
        C_BLOCK_TILE = 4,
        C_DISCONNECT = 5,
        C_GET_PLAYER_LIST = 6,
        C_CHANGE_PASSWORD = 7,

        //Server -> client
        S_AUTH_RESPONSE = 10,
        S_LOBBY_LIST = 11,
        S_WAITING_OPPONENT = 12,
        S_MATCH_FOUND = 13,
        S_GAME_STATE = 14,
        S_YOUR_TURN = 15,
        S_INVALID_MOVE = 16,
        S_GAME_OVER = 17,
        S_OPPONENT_DISCONNECTED = 18,
        S_PLAYER_LIST = 19,
        S_CHANGE_PASSWORD_RESPONSE = 20,

        S_STATUS_FAIL = 50,
        S_STATUS_SUCCESS = 51,
        S_STATUS_CREATED = 52

    } CommandID;

#endif