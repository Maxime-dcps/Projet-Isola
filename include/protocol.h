//
// Created by Max on 07/12/2025.
//

#ifndef PROJETRESEAUX_PROTOCOL_H
#define PROJETRESEAUX_PROTOCOL_H

    typedef struct {
        uint8_t  command_id;
        uint8_t  reserved;
        uint16_t length; //Length of the packet's body
    } Header;

    //Client -> server

    //C_AUTH_CHALLENGE
    typedef struct {
        char username[MAX_USERNAME_LEN];
        uint8_t auth_hash[AUTH_HASH_LEN];
    } CAuthChallenge;

    //C_MOVE_PAWN
    typedef struct {
        uint8_t to_pos_encoded; //LLLCCC00 (3 bits for the line, 3 bits for the column, 2 bits reserved)
    } CMovePawn;

    //C_BLOCK_TILE
    typedef struct {
        uint8_t block_pos_encoded; //LLLCCC00 (3 bits for the line, 3 bits for the column)
    } CBlockTile;

    //Serveur -> client

    //S_GAME_STATE
    typedef struct {
        uint8_t board[BOARD_DATA_SIZE];
        uint8_t turn_player_id;         //Flag to know if it's your turn
    } SGameState;

    //More to come

#endif