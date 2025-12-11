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
    //TODO: Implement GET
    return 1;
}

int create_user(sqlite3 *db, const char *username, const uint8_t *auth_hash) {
    //TODO: Implement INSERT
    return -1;
}

int update_user_stats(sqlite3 *db, const char *username, int win_increment, int loss_increment, int forfeit_increment) {
    //TODO: Implement UPDATE
    return -1;
}
