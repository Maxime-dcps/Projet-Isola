//
// Created by Max on 09/12/2025.
//

#ifndef PROJETRESEAUX_NETWORK_CORE_H
#define PROJETRESEAUX_NETWORK_CORE_H

#include <sys/types.h>
#include "server.h"
#include "config.h"

    extern Client *client_list_head;
    extern int server_socket;       // Listener socket

    //Init the server listening socket
    int start_server_loop();
    //Allocate memory and add client to the list
    void start_session(int socket_fd);

    //Closes the socket, and removes a client node from the list
    void end_session(Client *client);

    //Accept a new client connection then initialize the session
    void accept_new_connection();

    //Return the pointer to a client by its file descriptor
    Client* find_client_by_fd(int fd);

    //Handle data reception and FSM processing for a client
    int process_client_data(Client *client);

    //Sends a binary packet to a specific client.
    ssize_t send_packet(Client *client, CommandID command_id, const void *data_body, uint16_t body_len);

    //TODO: void handle_client_activity(Client *current_client);
#endif