//
// Created by Max on 11/12/2025.
//

#ifndef PROJETRESEAUX_SQLITE_UTILS_H
#define PROJETRESEAUX_SQLITE_UTILS_H

    #include <sqlite3.h>
    #include <stdint.h>
    #include "config.h"

    typedef struct {
        char username[MAX_USERNAME_LEN];
        uint8_t password_hash[AUTH_HASH_LEN];
        uint8_t salt[SALT_LEN];
        uint16_t wins;
        uint16_t losses;
        uint16_t forfeits;
    } User;

    extern sqlite3 *db_conn;

    //Initializes the DB connection and creates the 'users' table if it does not exist yet
    sqlite3* init_database();

    //Return user profile by username (-1 = error, 0 = user found, 1 = user not found)
    int get_user(sqlite3 *db, const char *username, User *user);

    //Creates a new user profile with a given username, password hash and salt.
    int create_user(sqlite3 *db, const char *username, const uint8_t *auth_hash, const uint8_t *salt);

    //Updates user statistics after a game
    int update_user_stats(sqlite3 *db, const char *username, int win_increment, int loss_increment, int forfeit_increment);

    //Get all users from database, returns number of users found (-1 on error)
    int get_all_users(sqlite3 *db, User *users, int max_users);

    //Change user password (returns 0 on success, 1 if old password doesn't match, -1 on error)
    int change_user_password(sqlite3 *db, const char *username, const char *old_password, const char *new_password);

    //Utility functions for password hashing
    //Generate a random salt
    void generate_salt(uint8_t *salt);
    //Hash password with salt using SHA-256
    void hash_password_with_salt(const char *password, const uint8_t *salt, uint8_t *hash_out);
#endif