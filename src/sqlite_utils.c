//
//Created by Max on 11/12/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/sqlite_utils.h"
#include "../include/config.h"

//Variable for the database connection
static sqlite3 *db_conn = NULL;

sqlite3* init_database(void) {
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
        "SELECT password_hash, wins, losses, forfeits FROM users WHERE username = ?;";

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

        //Copy data to the User structure
        strncpy(user->username, username, MAX_USERNAME_LEN); //Copying the value not the pointer
        memcpy(user->password_hash, hash_ptr, AUTH_HASH_LEN);
        user->wins = (uint16_t)sqlite3_column_int(stmt, 1);
        user->losses = (uint16_t)sqlite3_column_int(stmt, 2);
        user->forfeits = (uint16_t)sqlite3_column_int(stmt, 3);

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

int create_user(sqlite3 *db, const char *username, const uint8_t *auth_hash) {
    sqlite3_stmt *stmt;
    //Insert a new user
    const char *sql_insert = "INSERT INTO users (username, password_hash) VALUES (?, ?);";

    //Prepare the statement
    if (sqlite3_prepare_v2(db, sql_insert, -1, &stmt, 0) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to prepare INSERT statement : %s\n", sqlite3_errmsg(db));
        return -1;
    }

    //Bind parameter 1
    if (sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to bind username for INSERT : %s\n", sqlite3_errmsg(db));

        sqlite3_finalize(stmt);
        return -1;
    }
    //Bind parameter 2
    if (sqlite3_bind_blob(stmt, 2, auth_hash, AUTH_HASH_LEN, SQLITE_STATIC) != SQLITE_OK) {
        printf("SQLITE ERROR: failed to bind hash for INSERT : %s\n", sqlite3_errmsg(db));

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
    //TODO: Implement UPDATE
    return -1;
}
