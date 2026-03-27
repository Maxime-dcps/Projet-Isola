#include <stdio.h>

#include "network_core.h"
#include "sqlite_utils.h"

int main(void) {

    db_conn = init_database();

    if (db_conn == NULL) {
        printf("FATAL ERROR: Failed to initialize database connection.\n");
        return -1;
    }

    //Main loop, whiler server running
    if (start_server_loop() < 0) {
        printf("FATAL ERROR: Server loop failed to start or crashed.\n");
    }

    //Cleanup
    sqlite3_close(db_conn);

    return 0;
}