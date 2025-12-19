//
// Created by Max on 07/12/2025.
//

#ifndef PROJETRESEAUX_PROTOCOL_H
#define PROJETRESEAUX_PROTOCOL_H

#include "game.h"

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
        uint8_t board[ROW][COLUMN];
        /*Each tile has 4 states possible (ok, player 1, player 2, destroyed)
         * 0 -> free
         * 1 -> player 1
         * 2 -> player 3
         * 3 -> destroyed
         */
        uint8_t turn_player_id;         //Flag to know if it's your turn (1 -> your turn)
    } SGameState;

    //S_AUTH_RESPONSE
    typedef struct SAuthResponse{
        uint8_t status;
        uint16_t wins;
        uint16_t losses;
        uint16_t forfeits;
    } SAuthResponse;

    //S_GAME_OVER
    #define GAME_RESULT_WIN 1
    #define GAME_RESULT_LOSS 2
    #define GAME_RESULT_FORFEIT_WIN 3   // Opponent disconnected, you win
    #define GAME_RESULT_FORFEIT_LOSS 4  // You disconnected (forfeit)

    typedef struct SGameOver {
        uint8_t result;     // GAME_RESULT_WIN, GAME_RESULT_LOSS, etc.
        uint8_t padding;    // Padding for alignment
        uint16_t wins;
        uint16_t losses;
        uint16_t forfeits;
    } SGameOver;

    //S_PLAYER_LIST - Entry for each player
    #define MAX_PLAYERS_IN_LIST 50
    
    typedef struct PlayerEntry {
        char username[MAX_USERNAME_LEN];
        uint16_t wins;
        uint16_t losses;
        uint16_t forfeits;
        uint8_t is_online;  // 1 if online, 0 if offline
        uint8_t padding;    // Padding for alignment
    } PlayerEntry;

    //C_CHANGE_PASSWORD
    typedef struct CChangePassword {
        uint8_t old_password_hash[AUTH_HASH_LEN];
        uint8_t new_password_hash[AUTH_HASH_LEN];
    } CChangePassword;

    //S_CHANGE_PASSWORD_RESPONSE
    #define PASSWORD_CHANGE_SUCCESS 1
    #define PASSWORD_CHANGE_WRONG_OLD 2
    #define PASSWORD_CHANGE_ERROR 3

    typedef struct SChangePasswordResponse {
        uint8_t status;  // PASSWORD_CHANGE_SUCCESS, PASSWORD_CHANGE_WRONG_OLD, PASSWORD_CHANGE_ERROR
    } SChangePasswordResponse;

#endif