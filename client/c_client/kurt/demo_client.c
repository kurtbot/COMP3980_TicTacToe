#include "kfsm.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum
{
    SETUP = STATE_START, // 2
    READ_SERVER,         // 3
    READ_INPUT,          // 4
    SEND,                // 5
    UPDATE,              // 6
    ERROR,               // 7
} States;

typedef struct
{
    int tileID;
    int owner;
} tile;

typedef struct
{
    Environment common;
    // add more global variables here'
    int fd;
    int ID;
    int setup;
    int move;
    int server_input;
    tile board[9];
} Globals;

// FUNCTION PROTO

static int setup(Environment *globals);
static int read_server(Environment *globals);
static int read_input(Environment *globals);
static int update(Environment *globals);
static int send(Environment *globals);
static int common_error(Environment *globals);

// MAIN

int main(int argc, char const *argv[])
{
    Globals globals;

    Transition trn_table[] =
        {
            {STATE_INIT, SETUP, &setup},
            {SETUP, READ_SERVER, &read_server},
            {SETUP, ERROR, &common_error},
            {READ_SERVER, READ_INPUT, &read_input},
            {READ_SERVER, UPDATE, &update},
            {READ_SERVER, ERROR, &common_error},
            {READ_INPUT, SEND, &send},
            {READ_INPUT, ERROR, &common_error},
            {UPDATE, READ_INPUT, &read_input},
            {UPDATE, ERROR, &common_error},
            {SEND, READ_SERVER, &read_server},
            {SEND, ERROR, &common_error},
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
    printf("Connecting to server..\n");

    g->setup = 1;
    g->server_input = -1;
    g->move = -1;

    for (int i = 0; i < 9; i++)
    {
        g->board[i] = (tile){-1, -1};
    }

    return READ_SERVER;
}

static int read_server(Environment *globals)
{
    printf("read server state\n");

    Globals *g;
    g = (Globals *)globals;
    printf("waiting for server input\n");

    int server_input;
    int status = scanf("%d", &server_input);

    if (status == EOF)
    {
        if (ferror(stdin))
        {
            return ERROR;
        }
    }

    // player ID assignment
    {
        if (server_input < 9 && g->setup)
        {
            g->ID = 1; // player 2
        }
        else if (server_input >= 9 && g->setup)
        {
            g->ID = 0; // player 1
        }
        g->setup = 0;
    }

    g->server_input = server_input;

    // VERIFY SERVER DATA

    if (server_input < 9)
    {
        return UPDATE;
    }
    else if (server_input >= 9)
    {
        g->move = -1;
        return READ_INPUT;
    }
    else
    {
        return ERROR;
    }
}

static int read_input(Environment *globals)
{
    Globals *g;
    g = (Globals *)globals;

    printf("waiting for client %d input\n", g->ID);

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

    return SEND;
}

static int send(Environment *globals)
{
    Globals *g;
    g = (Globals *)globals;

    printf("sending [%d] to server\n", g->move);
    return READ_SERVER;
}

static int update(Environment *globals)
{
    Globals *g;
    g = (Globals *)globals;

    g->board[g->server_input].tileID = g->server_input;
    g->board[g->server_input].owner = (g->ID) ? 0 : 1;

    g->board[g->move].tileID = g->move;
    g->board[g->move].owner = g->ID;

    for (int i = 0; i < 9; i++)
    {
        int c = '?';

        if (g->board[i].owner == 0)
            c = 'O';
        else if (g->board[i].owner == 1)
            c = 'X';

        printf("%c", c);
        if (i == 2 || i == 5 || i == 8)
            printf("\n");
    }

    return READ_INPUT;
}