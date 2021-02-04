#include "kfsm.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum
{
    SETUP = STATE_START,
    READ,
    VERIFY,
    SEND_VALID,
    SEND_INVALID,
    ERROR
} States;

typedef struct
{
    int tileID;
    int owner;
} tile;

typedef struct
{
    Environment common;
    // add more global variables here
    int fd;
    int turn;
    int move;
    int moves;
    tile board[9];
} Globals;

// FUNCTION PROTO

static int setup(Environment *globals);
static int read(Environment *globals);
static int send_valid(Environment *globals);
static int send_invalid(Environment *globals);
static int verify(Environment *globals);
static int common_error(Environment *globals);

// MAIN

int main(int argc, char const *argv[])
{
    Globals globals;

    Transition trn_table[] =
        {
            {STATE_INIT, SETUP, &setup},
            {SETUP, READ, &read},
            {SETUP, ERROR, &read},
            {READ, ERROR, &common_error},
            {READ, VERIFY, &verify},
            {VERIFY, SEND_INVALID, &send_invalid},
            {VERIFY, SEND_VALID, &send_valid},
            {SEND_VALID, READ, &read},
            {SEND_INVALID, READ, &read},
            {STATE_NULL, STATE_NULL, NULL},
        };

    state_t start_state;
    state_t end_state;
    int status;

    start_state = STATE_INIT;
    end_state = SETUP;
    status = fsm_run((Environment *)&globals, &start_state, &end_state, trn_table);

    if (status != 0)
    {
        fprintf(stderr, "Cannot move from %d to %d\n", start_state, end_state);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// FUNCTION DECLARATION

static int common_error(Environment *globals)
{
    printf("** ERROR ENCOUNTER from state[%d] and state[%d]\n**", globals->from_state, globals->to_state);

    return STATE_NULL;
}

static int setup(Environment *globals)
{

    Globals *g;
    g = (Globals *)globals;

    printf("Loading...\n");
    printf("Waiting for clients..\n");

    g->moves = 0;
    g->turn = 0;

    for (int i = 0; i < 9; i++)
    {
        g->board[i] = (tile){-1, -1};
    }

    return READ;
}

static int read(Environment *globals)
{
    printf("read state\n");

    Globals *g;
    g = (Globals *)globals;
    printf("player %d's turn\n", (g->turn + 1));

    int client_input;
    int status = scanf("%d", &client_input);

    if (status == EOF)
    {
        if (ferror(stdin))
        {
            return ERROR;
        }
    }

    g->move = client_input;

    return VERIFY;
}

static int verify(Environment *globals)
{
    printf("verify state\n");

    Globals *g;
    g = (Globals *)globals;

    if (g->move >= 9)
        {
            printf("invalid move: must be 0-8\n");
            return SEND_INVALID;
        }

    for (int i = 0; i < 9; i++)
    {
        if (g->board[i].tileID == g->move)
        {
            printf("move exists\n");
            return SEND_INVALID;
        }
    }

    g->board[g->move].tileID = g->move;
    g->board[g->move].owner = g->turn;

    g->moves++;

    g->turn = (g->turn == 1) ? 0 : 1;

    return SEND_VALID;
}

static int send_valid(Environment *globals)
{
    printf("send valid state\n");

    Globals *g;
    g = (Globals *)globals;

    printf("BOARD: \n");

    for (int i = 0; i < 9; i++)
    {
        int c = '?';

        if (g->board[i].owner == 0) c = 'O';
        else if (g->board[i].owner == 1) c = 'X';

        printf("%c", c);
        if (i == 2 || i == 5 || i == 8)
            printf("\n");
    }

    printf("\n");

    return READ;
}

static int send_invalid(Environment *globals)
{
    printf("send invalid state\n");

    printf("sent retry code: 9\n");

    return READ;
}