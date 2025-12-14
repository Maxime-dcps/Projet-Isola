//
// Created by Max on 12/12/2025.
//

#ifndef PROJETRESEAUX_AUTH_H
#define PROJETRESEAUX_AUTH_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "sqlite_utils.h" //For db_conn and get_user()
#include "network_core.h" //For send_packet
#include "server.h" //For Client struct
#include "protocol.h" //For SAuthResponse

//Handles the C_AUTH_CHALLENGE packet
void handle_auth_challenge(Client *client, const CAuthChallenge *challenge);
//Creates user if user not found
void process_user_creation(Client *client, char username_safe[17], const uint8_t challenge_hash[32], SAuthResponse *response);
//Authenticates user if user found
void process_user_authentication(Client *client, User *user_profile, const uint8_t challenge_hash[32], SAuthResponse *response);
#endif //PROJETRESEAUX_AUTH_H