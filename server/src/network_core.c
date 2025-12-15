#include "network_core.h"

//Global variables
Client *client_list_head = NULL;
int server_socket = -1;

//TODO: void handle_client_activity(Client *current_client);

void start_session(int socket_fd) {
    //Allocate memory for the new client node
    Client *new_client = (Client *)malloc(sizeof(Client));
    if (new_client == NULL) {
        printf("Error allocating memory for new client\n");
        close(socket_fd);
        return;
    }

    //Initialize the client fields
    *new_client = (Client){0}; //Clear all fields
    new_client->socket_fd = socket_fd;
    new_client->state = CONNECTED;
    new_client->game_id = -1;

    //Add the new client at the beginning of the linked list
    new_client->next = client_list_head;
    client_list_head = new_client;

    printf("INFO: New connection accepted. FD: %d.", socket_fd);
}

void end_session(Client *client_to_remove) {
    if (client_to_remove == NULL) return;

    //Find the client in the list to unlink it
    Client **ptr = &client_list_head;
    while (*ptr != NULL && *ptr != client_to_remove) {
        ptr = &(*ptr)->next;
    }

    if (*ptr == client_to_remove) {
        //Unlink the node
        *ptr = client_to_remove->next;
    }

    printf("INFO: Closing connection. FD: %d. Username: %s\n", client_to_remove->socket_fd, client_to_remove->username);

    //Close the socket and free memory
    close(client_to_remove->socket_fd);
    free(client_to_remove);

    //TODO: Handle give up and SQLite updates if state was IN_GAME
}

Client* find_client_by_fd(int fd) {
    Client *current = client_list_head;
    while (current != NULL) {
        if (current->socket_fd == fd) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void accept_new_connection() {
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    //Accept new connection
    int new_fd = accept(server_socket, (struct sockaddr *)&cli_addr, &clilen);
    if (new_fd < 0) {
        if (errno != EWOULDBLOCK) {
             printf("Error accepting new connection\n");
        }
        return;
    }

    //Init the session
    start_session(new_fd);
}

int start_server_loop() {
    //Server socket initialization and setup
    struct sockaddr_in srv_addr;
    int optval = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Error creating server listening socket\n");
        return -1;
    }

    //Set option SO_REUSEADDR to allow direct reconnection on socket when client closed
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(optval)) < 0) {
        printf("Error setting socket options\n");
        close(server_socket);
        return -1;
    }

    //Bind to the port 55555
    srv_addr = (struct sockaddr_in){0};
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //Listening on every interface
    srv_addr.sin_port = htons(SERVER_PORT); //htons and htonl to resolve endianness

    if (bind(server_socket, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        printf("Error binding socket\n");
        close(server_socket);
        return -1;
    }

    //Listen for incoming connections
    if (listen(server_socket, 10) < 0) {
        printf("Error listening on socket\n");
        close(server_socket);
        return -1;
    }

    printf("INFO: Server listening on port %d...\n", SERVER_PORT);

    //Main loop
    while (1) {
        fd_set read_fds; //Init fd_set to check for activity
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds); //Add server socket

        int max_fd = server_socket;

        //Add every active client sockets to the set
        Client *current_client = client_list_head;
        while (current_client != NULL) {
            FD_SET(current_client->socket_fd, &read_fds);
            if (current_client->socket_fd > max_fd) {
                max_fd = current_client->socket_fd;
            }
            current_client = current_client->next;
        }

        //Wait for activity on any socket (select is looping until activity)
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno != EINTR) {
                printf("Error while waiting for activity\n");
                break;
            }
            //Else interrupted by a signal, retry
        }

        //Check for new connection
        if (FD_ISSET(server_socket, &read_fds)) {
            accept_new_connection();
        }

        //Check for activity on existing clients
        current_client = client_list_head;
        while (current_client != NULL) {
            Client *next_client = current_client->next; //Get next pointer BEFORE processing

            if (FD_ISSET(current_client->socket_fd, &read_fds)) {
                //If process_client_data returns -1, the client disconnected
                if (process_client_data(current_client) < 0) {
                    end_session(current_client);
                    //The current_client now invalid/freed
                }
            }
            //Move to the next client
            current_client = next_client;
        }
    }

    //Cleanup server socket
    close(server_socket);
    return 0;
}

int process_client_data(Client *client) {
    //Read data into buffer
    ssize_t bytes_read = read(client->socket_fd, client->recv_buffer + client->recv_len, sizeof(client->recv_buffer) - client->recv_len);

    if (bytes_read <= 0) {
        if (bytes_read < 0 && errno == EWOULDBLOCK) {
            //No data available right now -> leave the processing
            return 0;
        }
        //Disconnection
        return -1;
    }

    client->recv_len += bytes_read;

    //Check and process complete packets in the buffer
    while (client->recv_len >= HEADER_SIZE) {
        //Read Header
        Header *header = (Header *)client->recv_buffer;
        //Must be converted
        uint16_t required_len = ntohs(header->length); //Convert length to host byte order

        uint16_t total_packet_size = HEADER_SIZE + required_len;

        //Check if the full packet is available
        if (client->recv_len < total_packet_size) {
            //Packet is incomplete, wait for more data
            break;
        }

        //If the full packet is available, process it
        printf("DEBUG: Received complete packet. ID: %d, Length: %d\n", header->command_id, required_len);

        fsm_process_packet(client, header->command_id, client->recv_buffer + HEADER_SIZE, required_len);

        //Shift the remaining data in the buffer to avoid deleting the next packet
        memmove(client->recv_buffer,
                client->recv_buffer + total_packet_size,
                client->recv_len - total_packet_size);
        client->recv_len -= total_packet_size;
    }

    return 1;
}

ssize_t send_packet(Client *client, CommandID command_id, const void *data_body, uint16_t body_len) {
    //Allocate space for the full packet (header + body)
    size_t total_len = HEADER_SIZE + body_len;
    uint8_t *packet_buffer = (uint8_t *)malloc(total_len);
    if (packet_buffer == NULL) {
        printf("Error allocating space for the buffer\n");
        return -1;
    }

    //Build the header
    Header *header = (Header *)packet_buffer;
    header->command_id = (uint8_t)command_id;
    header->reserved = 0;
    header->length = htons(body_len);

    //Copy the Body
    if (data_body != NULL && body_len > 0) {
        memcpy(packet_buffer + HEADER_SIZE, data_body, body_len);
    }

    //Send the packet in one time
    ssize_t bytes_sent = write(client->socket_fd, packet_buffer, total_len);

    if (bytes_sent < 0) {
        printf("Error sending packet\n");
    }

    free(packet_buffer);
    return bytes_sent; //Number of bytes successfuly sent
}

void fsm_process_packet(Client *client, CommandID command_id, const uint8_t *packet_body, uint16_t body_len) {
    // Check command validity based on the client's current state (FSM)

    if (!validate_body_size(command_id, body_len)) {
        printf("SECURITY ALERT: Invalid body size (%u) for command_id (%d) from FD: %d. Expected: %zu\n", body_len, command_id, client->socket_fd, sizeof(CAuthChallenge));
        client->state = DISCONNECTED;
    }

    switch (client->state) {
        case CONNECTED:
            if (command_id == C_AUTH_CHALLENGE) {
                // The client is connected and attempts to authenticate

                CAuthChallenge challenge;
                memcpy(&challenge, packet_body, sizeof(CAuthChallenge));

                handle_auth_challenge(client, &challenge);
            } else {
                // Invalid command for this state: disconnect the client
                printf("FSM WARNING: Client %d sent invalid command %d in state CONNECTED\n", client->socket_fd, command_id);
                client->state = DISCONNECTED; // Flag for immediate removal by main loop
            }
            break;

        case AUTHENTICATED:
            // Client is in the lobby, expecting C_PLAY_REQUEST or C_DISCONNECT
            if (command_id == C_PLAY_REQUEST) {
                // TODO: Implement matchmaking logic
                printf("FSM INFO: User %s requesting match.\n", client->username);
            } else if (command_id == C_DISCONNECT) {
                client->state = DISCONNECTED;
            } else {
                printf("FSM WARNING: User %s sent invalid command %d in state AUTHENTICATED\n", client->username, command_id);
            }
            break;

        case IN_QUEUE:
            //TODO : Implement IN_QUEUE logic
            break;

        case IN_GAME:
            //TODO : Implement IN_GAME logic
            break;

        case DISCONNECTED:
            //Ignore, this client will be removed on the next iteration
            printf("FSM WARNING: User %s try to send command %d in state DISCONNECTED\n", client->username, command_id);
            break;

        default:
            printf("FSM WARNING: User %s try to send command %d in unattended state\n", client->username, command_id);
            break;
    }
}

int validate_body_size(CommandID command_id, uint16_t body_len) {
    int expected_len;
    switch (command_id) {
        case C_AUTH_CHALLENGE:
            expected_len = sizeof(CAuthChallenge);
            break;
        case C_MOVE_PAWN:
            expected_len = sizeof(CMovePawn);
            break;
        case C_BLOCK_TILE:
            expected_len = sizeof(CBlockTile);
            break;
        default:
            return -1;
    }
    return (body_len == expected_len);
}