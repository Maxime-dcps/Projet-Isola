#include <stdio.h>

#include "network_core.h"
#include "sqlite_utils.h"

//Activ debug mod
//#define DEBUG 0





//FUNCTIONS
// void init_board(Game *game);
// void move_player(char board[ROW][COLUMN], Player * player, int destRow, int destColumn);
// int is_in_reach(Player * player, int row, int col);
// int is_in_board(int row, int col);
// void set_player_pos(Player *player, int row, int column);
// void get_player_pos(char board[ROW][COLUMN], char player, int *row, int *column);
// void display_board(char board [ROW][COLUMN]);
// void delete_tile(char board[ROW][COLUMN], int row, int column);
// int get_game_result(Game game);
// int can_move(char board[ROW][COLUMN], Player player);

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

    #ifdef DEBUG
        int gameResult = 0;
        /*
         * gameResult status codes:
         *   0 = Game is still in progress
         *   1 = Game over → Player 1 has won
         *   2 = Game over → Player 2 has won
         */

        Game game;

        game.p1.symbol = 'A';
        game.p2.symbol = 'B';
        game.current = 1;

        init_board(&game);

        // while (gameResult == 0 || sudden_disconnection)
        // {
        //     move();
        //     delete();
        //     game.current = game.current == 1 ? 1 : 2;
        //     gameResult = get_game_result(game);
        // }
        display_board(game.board);
        move_player(game.board, &game.p1, 2, 1);
        display_board(game.board);
        delete_tile(game.board, 1, 2);
        move_player(game.board, &game.p1, 1, 2);
        display_board(game.board);
        move_player(game.board, &game.p1, 1, 4);
        display_board(game.board);
    #endif

    return 0;
}

// void move_player(char board[ROW][COLUMN], Player * player, int destRow, int destColumn)
// {
//     if (is_in_board(destRow, destColumn) && is_in_reach(player, destRow, destColumn) && board[destRow][destColumn] == '0')//->REFACTOR must be player turn
//     {
//         board[player->row][player->col] = '0';
//         board[destRow][destColumn] = player->symbol;
//         set_player_pos(player, destRow, destColumn);
//
//         #ifdef DEBUG
//         printf("Player %c has moved (%d, %d)\n", player->symbol, destRow, destColumn);
//         #endif
//     }
//     #ifdef DEBUG
//     else {
//         if (!is_in_board(destRow, destColumn)) printf("Destination is not in board \n");
//         else if (!is_in_reach(player, destRow, destColumn)) printf("Destination is not in reach\n");
//         else if (board[destRow][destColumn] != '0') printf("Destination is not isn't available\n");
//     }
//     #endif
//
// }
//
// void set_player_pos(Player *player, int row, int column) {
//     //REFACTOR : verify inner condition
//     player->row = row;
//     player->col = column;
// }
//
// void get_player_pos(char board[ROW][COLUMN], char player, int *row, int *column)
// {
//     int i = 0, found = 0, j = 0;
//
//     *row = -1;
//     *column = -1;
//
//     while (!found && i < ROW) {
//
//         j = 0;
//
//         while (!found && j < COLUMN) {
//             (board[i][j] == player) ? found = 1 : j++;
//         }
//         if (!found) i++;
//     }
//
//     if (found) {
//         *row = i;
//         *column = j;
//     }
//
//     #ifdef DEBUG
//         if (found) printf("Player %c found : (%d;%d)\n", player, *row, *column);
//         else printf("Player %c not found\n", player);
//     #endif
// }
//
// void delete_tile(char board[ROW][COLUMN], int row, int column) {
//     if (board[row][column] == '0') board[row][column] = 'X';
// }
//
// int get_game_result(Game game)
// {
//     //Current player is either player 1 or player 2
//     Player currentPlayer = game.current == 1 ? game.p1 : game.p2;
//     Player opponent = game.current == 1 ? game.p2 : game.p1;
//     //The winner order will be his order number
//     int opponentOrder = game.current == 1 ? 2 : 1;
//
//     if (!can_move(game.board, currentPlayer)) return opponentOrder;
//
//     if (!can_move(game.board, opponent)) return game.current;
//
//     return 0; //Game is not over yet
//}