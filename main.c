#include <stdio.h>

#include "display.h"

//Activ debug mod
#define DEBUG 0

#define ROW 6
#define COLUMN 8

//STRUCTURES
typedef struct {
    int row;
    int col;
    char symbol;
} Player;

typedef struct {
    char board [ROW][COLUMN];
    Player p1;
    Player p2;
    int current;
} Game;

//FUNCTIONS
void init_board(Game *game);
void move_player(char board[ROW][COLUMN], Player * player, int destRow, int destColumn);
int is_in_reach(Player player, int row, int col);
int is_in_board(int row, int col);
void set_player_pos(Player *player, int row, int column);
void get_player_pos(char board[ROW][COLUMN], char player, int *row, int *column);
void display_board(char board [ROW][COLUMN]);
void delete_tile(char board[ROW][COLUMN], int row, int column);
int get_game_result(Game game);
int can_move(char board[ROW][COLUMN], Player player);

int main(void) {

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

    #ifdef DEBUG
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

void init_board(Game *game) {

    //Resetting the board
    int i,j;
    for(i = 0; i < ROW; i++) {
        for(j = 0; j < COLUMN; j++) {
            game->board[i][j] = '0';
        }
    }

    //Placing player A
    game->board[2][0] = game->p1.symbol;
    set_player_pos(&(game->p1), 2, 0);

    //Placing player B
    game->board[3][7] = game->p2.symbol;
    set_player_pos(&(game->p2), 3, 7);

    /* TO REFACTOR
        split in init_board() + init_players() = init_game()
        place_player(); //Update board and player position
    */
}

void move_player(char board[ROW][COLUMN], Player * player, int destRow, int destColumn)
{
    if (is_in_board(destRow, destColumn) && is_in_reach(*player, destRow, destColumn) && board[destRow][destColumn] == '0')//->REFACTOR must be player turn
    {
        board[player->row][player->col] = '0';
        board[destRow][destColumn] = player->symbol;
        set_player_pos(player, destRow, destColumn);

        #ifdef DEBUG
        printf("Player %c has moved (%d, %d)\n", player->symbol, destRow, destColumn);
        #endif
    }
    #ifdef DEBUG
    else {
        if (!is_in_board(destRow, destColumn)) printf("Destination is not in board \n");
        else if (!is_in_reach(*player, destRow, destColumn)) printf("Destination is not in reach\n");
        else if (board[destRow][destColumn] != '0') printf("Destination is not isn't available\n");
    }
    #endif

}

//Can player move to this position
int is_in_reach(Player player, int row, int col)
{
    int dx = player.row - row;
    int dy = player.col - col;
    return (dx*dx + dy*dy) < 4;
}

int is_in_board(int row, int col)
{
    return (row >= 0 && row < ROW && col >= 0 && col < COLUMN);
}

void set_player_pos(Player *player, int row, int column) {
    //REFACTOR : verify inner condition
    player->row = row;
    player->col = column;
}

void get_player_pos(char board[ROW][COLUMN], char player, int *row, int *column)
{
    int i = 0, found = 0, j = 0;

    *row = -1;
    *column = -1;

    while (!found && i < ROW) {

        j = 0;

        while (!found && j < COLUMN) {
            (board[i][j] == player) ? found = 1 : j++;
        }
        if (!found) i++;
    }

    if (found) {
        *row = i;
        *column = j;
    }

    #ifdef DEBUG
        if (found) printf("Player %c found : (%d;%d)\n", player, *row, *column);
        else printf("Player %c not found\n", player);
    #endif
}

void display_board(char board [ROW][COLUMN])
{
    int i, j;

    printf("------------------------\n");

    for (i = 0; i < ROW; i++)
    {
        for (j = 0; j < COLUMN; j++) printf(" %c ", board[i][j]);
        printf("\n");
    }

    printf("------------------------\n");
}

void delete_tile(char board[ROW][COLUMN], int row, int column) {
    if (board[row][column] == '0') board[row][column] = 'X';
}

int get_game_result(Game game)
{
    //Current player is either player 1 or player 2
    Player currentPlayer = game.current == 1 ? game.p1 : game.p2;
    Player opponent = game.current == 1 ? game.p2 : game.p1;
    //The winner order will be his order number
    int opponentOrder = game.current == 1 ? 2 : 1;

    if (!can_move(game.board, currentPlayer)) return opponentOrder;

    if (!can_move(game.board, opponent)) return game.current;

    return 0; //Game is not over yet
}

int can_move(char board[ROW][COLUMN], Player player) {
    //Arrays representing the 8 possible movement directions
    const int rowOffset[] = {-1, -1, -1,  0,  0,  1,  1,  1};
    const int colOffset[] = {-1,  0,  1, -1,  1, -1,  0,  1};

    for (int i = 0; i < 8; i++) {
        int tileRow = player.row + rowOffset[i];
        int tileCol = player.col + colOffset[i];

        //!\ Function order here is crucial
        if (is_in_board(tileRow, tileCol) && board[tileRow][tileCol] == '0') {
            return 1; // Valid move found, exit immediately
        }
    }

    return 0; // No valid moves found after checking all 8 tiles
}