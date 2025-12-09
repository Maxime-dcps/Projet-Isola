//
// Created by Max on 07/12/2025.
//

#ifndef PROJETRESEAUX_SERVER_H
#define PROJETRESEAUX_SERVER_H

    //FSM states
    typedef enum {
        CONNECTED,
        AUTHENTICATED,
        IN_QUEUE,
        IN_GAME,
        DISCONNECTED
    } ClientState;

    //Struct containing the session info
    typedef struct {
        struct Client *next;            //Pointer to the next client in the list

        int socket_fd;                  //Socket's file descriptor
        ClientState state;              //Current state for the FSM
        char username[MAX_USERNAME_LEN];
        int game_id;                    //Game ID if playing, else -1
        uint8_t recv_buffer[512];       //Buffer before parsing
        int recv_len;

    } Client;


#endif