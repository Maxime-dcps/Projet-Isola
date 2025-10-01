#include <stdio.h>

#include "display.h"

//Activ debug mod
#define DEBUG 0

#define ROW 6
#define COLUMN 8

void init_board(char gameBoard [ROW][COLUMN]);
int move_player(char gameBoard[ROW][COLUMN], char player, int destRow, int destColumn);
void get_player_pos(char gameBoard[ROW][COLUMN], char player, int *row, int *column);
void display_board(char gameBoard [ROW][COLUMN]);

int main(void) {

    char gameBoard [ROW][COLUMN];

    init_board(gameBoard);

    #ifdef DEBUG
        int a, b;
        get_player_pos(gameBoard, 'B', &a, &b );
        display_board(gameBoard);
    #endif

    //disp(0);

    return 0;
}

void init_board(char gameBoard [ROW][COLUMN]) {

    //Resetting the board
    int i,j;
    for(i = 0; i < ROW; i++) {
        for(j = 0; j < COLUMN; j++) {
            gameBoard[i][j] = '0';
        }
    }

    //Placing player A and B
    gameBoard[2][0] = 'A';
    gameBoard[3][7] = 'B';
}

int move_player(char gameBoard[ROW][COLUMN], char player, int destRow, int destColumn)
{
    int oldRow, oldColumn;

    if (gameBoard[destRow][destColumn] == '0')//->REFACTOR isAvailable
    {
        get_player_pos(gameBoard, player, &oldRow, &oldColumn );
        gameBoard[destRow][destColumn] = player;

        return 0;
    }
    else return -1;
}

void get_player_pos(char gameBoard[ROW][COLUMN], char player, int *row, int *column)
{
    int i = 0, found = 0, j = 0;

    *row = -1;
    *column = -1;

    while (!found && i < ROW) {

        j = 0;

        while (!found && j < COLUMN) {
            (gameBoard[i][j] == player) ? found = 1 : j++;
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

void display_board(char gameBoard [ROW][COLUMN])
{
    int i, j;

    printf("------------------------\n");

    for (i = 0; i < ROW; i++)
    {
        for (j = 0; j < COLUMN; j++) printf(" %c ", gameBoard[i][j]);
        printf("\n");
    }

    printf("------------------------\n");
}