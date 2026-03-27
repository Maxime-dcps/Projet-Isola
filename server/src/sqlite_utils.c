//
//Created by Max on 11/12/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <openssl/sha.h>

#include "sqlite_utils.h"
#include "config.h"

//Variable for the database connection
sqlite3 *db_conn = NULL;

sqlite3* init_database() {
    if (db_conn != NULL) {
        //Database already initialized
        return db_conn;
    }

    //Create de database file, open if already existing
    int status = sqlite3_open(SQLITE_DB_NAME, &db_conn);
    if (status != SQLITE_OK) {
        printf("SQLITE ERROR: cannot open database\n");
        return NULL;
    }

    //SQL statement to create the users table
    const char *sql_create_table =
        "CREATE TABLE IF NOT EXISTS users ("
            "username TEXT PRIMARY KEY NOT NULL,"
            "password_hash BLOB NOT NULL,"
            "salt BLOB NOT NULL,"
            "wins INTEGER DEFAULT 0,"
            "losses INTEGER DEFAULT 0,"
            "forfeits INTEGER DEFAULT 0"
        ");";

    //Execute the query
    char *err_msg = 0;
    status = sqlite3_exec(db_conn, sql_create_table, 0, 0, &err_msg);

    if (status != SQLITE_OK) {
        printf("SQLITE ERROR: SQL error creating table : %s\n", sqlite3_errmsg(db_conn));
        sqlite3_free(err_msg);
        sqlite3_close(db_conn);
        db_conn = NULL;
        return NULL;
    }

    printf("INFO: SQLite database initialized successfully: %s\n", SQLITE_DB_NAME);
    return db_conn;
}

int get_user(sqlite3 *db, const char *username, User *user) {
    sqlite3_stmt *stmt; //Create a new statement

    const char *sql_select =
        "SELECT password_hash, salt, wins, losses, forfeits FROM users WHERE username = ?;";

    //Prepare the statement
    if (sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to prepare SELECT statement : %s\n", sqlite3_errmsg(db));
        return -1;
    }

    //Bind the parameter to avoid SQL injection
    if (sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to bind username for SELECT : %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); //Clean the statement
        return -1;
    }

    //Execute the query and check results
    int status = sqlite3_step(stmt);

    if (status == SQLITE_ROW) {
        //User found

        //Col 0: password_hash (BLOB)
        const uint8_t *hash_ptr = sqlite3_column_blob(stmt, 0);
        int hash_len = sqlite3_column_bytes(stmt, 0);

        //Ensure hash length matches expected length
        if (hash_len != AUTH_HASH_LEN) {
            fprintf(stderr, "SECURITY WARNING: Hash length mismatch for user %s\n", username);
            sqlite3_finalize(stmt);
            return -1; //Error
        }

        //Col 1: salt (BLOB)
        const uint8_t *salt_ptr = sqlite3_column_blob(stmt, 1);
        int salt_len = sqlite3_column_bytes(stmt, 1);

        //Ensure salt length matches expected length
        if (salt_len != SALT_LEN) {
            fprintf(stderr, "SECURITY WARNING: Salt length mismatch for user %s\n", username);
            sqlite3_finalize(stmt);
            return -1; //Error
        }

        //Copy data to the User structure
        strncpy(user->username, username, MAX_USERNAME_LEN); //Copying the value not the pointer
        memcpy(user->password_hash, hash_ptr, AUTH_HASH_LEN);
        memcpy(user->salt, salt_ptr, SALT_LEN);
        user->wins = (uint16_t)sqlite3_column_int(stmt, 2);
        user->losses = (uint16_t)sqlite3_column_int(stmt, 3);
        user->forfeits = (uint16_t)sqlite3_column_int(stmt, 4);

        sqlite3_finalize(stmt);
        return 0; //User found

    } else if (status == SQLITE_DONE) {
        //No row returned
        sqlite3_finalize(stmt);
        return 1; //User not found

    } else {
        //General error
        printf("SQLITE ERROR: execution error during SELECT : %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1; //Error
    }
}

int create_user(sqlite3 *db, const char *username, const uint8_t *auth_hash, const uint8_t *salt) {
    sqlite3_stmt *stmt;
    //Insert a new user
    const char *sql_insert = "INSERT INTO users (username, password_hash, salt) VALUES (?, ?, ?);";

    //Prepare the statement
    if (sqlite3_prepare_v2(db, sql_insert, -1, &stmt, 0) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to prepare INSERT statement : %s\n", sqlite3_errmsg(db));
        return -1;
    }

    //Bind parameter 1 (username)
    if (sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to bind username for INSERT : %s\n", sqlite3_errmsg(db));

        sqlite3_finalize(stmt);
        return -1;
    }
    //Bind parameter 2 (password_hash)
    if (sqlite3_bind_blob(stmt, 2, auth_hash, AUTH_HASH_LEN, SQLITE_STATIC) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to bind hash for INSERT : %s\n", sqlite3_errmsg(db));

        sqlite3_finalize(stmt);
        return -1;
    }
    //Bind parameter 3 (salt)
    if (sqlite3_bind_blob(stmt, 3, salt, SALT_LEN, SQLITE_STATIC) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to bind salt for INSERT : %s\n", sqlite3_errmsg(db));

        sqlite3_finalize(stmt);
        return -1;
    }

    //Execute the query
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("SQLITE ERROR: execution error during INSERT : %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    //Cleanup
    sqlite3_finalize(stmt);
    return 0; //User created
}

int update_user_stats(sqlite3 *db, const char *username, int win_increment, int loss_increment, int forfeit_increment) {
    sqlite3_stmt *stmt;
    const char *sql_update = 
        "UPDATE users SET wins = wins + ?, losses = losses + ?, forfeits = forfeits + ? WHERE username = ?;";

    if (sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to prepare UPDATE statement : %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // Bind parameters
    sqlite3_bind_int(stmt, 1, win_increment);
    sqlite3_bind_int(stmt, 2, loss_increment);
    sqlite3_bind_int(stmt, 3, forfeit_increment);
    sqlite3_bind_text(stmt, 4, username, -1, SQLITE_STATIC);

    int status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (status != SQLITE_DONE) {
        printf("SQLITE ERROR: execution error during UPDATE : %s\n", sqlite3_errmsg(db));
        return -1;
    }

    printf("DB: Updated stats for %s (W+%d, L+%d, F+%d)\n", username, win_increment, loss_increment, forfeit_increment);
    return 0;
}

int get_all_users(sqlite3 *db, User *users, int max_users) {
    sqlite3_stmt *stmt;
    const char *sql_select = "SELECT username, wins, losses, forfeits FROM users ORDER BY wins DESC;";

    if (sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to prepare SELECT ALL statement : %s\n", sqlite3_errmsg(db));
        return -1;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_users) {
        const char *username = (const char *)sqlite3_column_text(stmt, 0);
        strncpy(users[count].username, username, MAX_USERNAME_LEN);
        users[count].wins = (uint16_t)sqlite3_column_int(stmt, 1);
        users[count].losses = (uint16_t)sqlite3_column_int(stmt, 2);
        users[count].forfeits = (uint16_t)sqlite3_column_int(stmt, 3);
        count++;
    }

    sqlite3_finalize(stmt);
    return count;
}

int change_user_password(sqlite3 *db, const char *username, const char *old_password, const char *new_password) {
    // First, verify the old password
    User user;
    int result = get_user(db, username, &user);
    
    if (result != 0) {
        printf("SQLITE ERROR: User not found for password change\n");
        return -1;
    }

    // Hash old password with stored salt and compare
    uint8_t old_hash[AUTH_HASH_LEN];
    hash_password_with_salt(old_password, user.salt, old_hash);
    
    if (memcmp(user.password_hash, old_hash, AUTH_HASH_LEN) != 0) {
        printf("AUTH: Old password mismatch for user %s\n", username);
        return 1;  // Wrong old password
    }

    // Generate new salt and hash new password
    uint8_t new_salt[SALT_LEN];
    uint8_t new_hash[AUTH_HASH_LEN];
    generate_salt(new_salt);
    hash_password_with_salt(new_password, new_salt, new_hash);

    // Update password and salt
    sqlite3_stmt *stmt;
    const char *sql_update = "UPDATE users SET password_hash = ?, salt = ? WHERE username = ?;";

    if (sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to prepare password UPDATE statement : %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_blob(stmt, 1, new_hash, AUTH_HASH_LEN, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, new_salt, SALT_LEN, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, username, -1, SQLITE_STATIC);

    int status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (status != SQLITE_DONE) {
        printf("SQLITE ERROR: execution error during password UPDATE : %s\n", sqlite3_errmsg(db));
        return -1;
    }

    printf("AUTH: Password changed successfully for user %s\n", username);
    return 0;  // Success
}

//Generate a random salt using /dev/urandom
void generate_salt(uint8_t *salt) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        // Fallback: use time-based pseudo-random if /dev/urandom unavailable
        srand((unsigned int)time(NULL));
        for (int i = 0; i < SALT_LEN; i++) {
            salt[i] = (uint8_t)(rand() % 256);
        }
        return;
    }
    read(fd, salt, SALT_LEN);
    close(fd);
}

//Hash password with salt using SHA-256: hash = SHA256(salt + password)
void hash_password_with_salt(const char *password, const uint8_t *salt, uint8_t *hash_out) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, salt, SALT_LEN);  // Salt first
    SHA256_Update(&ctx, password, strlen(password));  // Then password
    SHA256_Final(hash_out, &ctx);
}
