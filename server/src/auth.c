//
//Created by Max on 12/12/2025.
//

#include "auth.h"

void handle_auth_challenge(Client *client, const CAuthChallenge *challenge) {
    User user_profile;
    SAuthResponse response;

    //Double check of the username integrity
    char username_safe[MAX_USERNAME_LEN + 1];
    strncpy(username_safe, challenge->username, MAX_USERNAME_LEN);
    username_safe[MAX_USERNAME_LEN] = '\0';

    //Extract password safely
    char password_safe[MAX_PASSWORD_LEN + 1];
    strncpy(password_safe, challenge->password, MAX_PASSWORD_LEN);
    password_safe[MAX_PASSWORD_LEN] = '\0';

    int status = get_user(db_conn, username_safe, &user_profile);
    
    //Database error
    if (status < 0) {
        printf("AUTH ERROR: DB lookup failed for user: %s\n", challenge->username);
        response.status = S_STATUS_FAIL;
        //Sending FAIL to the client

    } else if (status == 1) {
        //User not found -> create a new account
        process_user_creation(client, username_safe, password_safe, &response);

    } else { 
        //User Found -> authenticate
        process_user_authentication(client, &user_profile, password_safe, &response);
    }

    //Send response to the client
    send_packet(client, S_AUTH_RESPONSE, &response, sizeof(SAuthResponse));

}

void process_user_creation(Client *client, char username_safe[17], const char *password, SAuthResponse *response) {
    //Generate salt and hash password
    uint8_t salt[SALT_LEN];
    uint8_t password_hash[AUTH_HASH_LEN];
    
    generate_salt(salt);
    hash_password_with_salt(password, salt, password_hash);
    
    if (create_user(db_conn, username_safe, password_hash, salt) == 0) {
        //Creation Success: Initialize profile data
        strncpy(client->username, username_safe, MAX_USERNAME_LEN);
        client->state = AUTHENTICATED;

        response->status = S_STATUS_CREATED;
        response->wins = 0; //Htons not needed here because of the symmetry
        response->losses = 0;
        response->forfeits = 0;
        printf("AUTH INFO: New user created and authenticated: %s\n", username_safe);

    } else {
        printf("AUTH ERROR: Failed to create user: %s\n", username_safe);
        response->status = S_STATUS_FAIL;
    }
}

void process_user_authentication(Client *client, User *user_profile, const char *password, SAuthResponse *response) {
    //Hash the received password with the stored salt
    uint8_t computed_hash[AUTH_HASH_LEN];
    hash_password_with_salt(password, user_profile->salt, computed_hash);
    
    //Compare the computed hash with the stored hash
    if (memcmp(computed_hash, user_profile->password_hash, AUTH_HASH_LEN) == 0) {
        //Authentication Success
        strncpy(client->username, user_profile->username, MAX_USERNAME_LEN);
        client->state = AUTHENTICATED;

        response->status = S_STATUS_SUCCESS;
        response->wins = htons(user_profile->wins);
        response->losses = htons(user_profile->losses);
        response->forfeits = htons(user_profile->forfeits);
        printf("AUTH INFO: User authenticated: %s\n", client->username);

    } else {
        printf("AUTH ERROR: Password doesn't match for user: %s\n", user_profile->username);
        response->status = S_STATUS_FAIL;
    }
}
