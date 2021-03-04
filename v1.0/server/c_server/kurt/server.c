#include "kfsm.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#define PORT 6969

#define BACKLOG 2
#define BUF_SIZE 1024

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
    int cfd[2];
    int turn;
    int move;
    int moves;
    tile board[9];
} Globals;

// FUNCTION PROTO

static int setup(Environment *globals);
static int readClient(Environment *globals);
static int send_valid(Environment *globals);
static int send_invalid(Environment *globals);
static int verify(Environment *globals);
static int common_error(Environment *globals);

// MAIN

int main()
{
    Globals *globals = malloc(sizeof(Globals));

    Transition trn_table[] =
        {
            {STATE_INIT, SETUP, &setup},
            {SETUP, READ, &readClient},
            {SETUP, ERROR, &readClient},
            {READ, ERROR, &common_error},
            {READ, VERIFY, &verify},
            {VERIFY, SEND_INVALID, &send_invalid},
            {VERIFY, SEND_VALID, &send_valid},
            {SEND_VALID, READ, &readClient},
            {SEND_INVALID, READ, &readClient},
            {STATE_NULL, STATE_NULL, NULL},
        };

    state_t start_state;
    state_t end_state;
    int status;

    start_state = STATE_INIT;
    end_state = SETUP;
    status = fsm_run((Environment *)globals, &start_state, &end_state, trn_table);

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

    struct sockaddr_in addr;
    int sfd;

    // setup socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(struct sockaddr_in));

    int enable = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    //
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind server to address
    bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    // listen to [BACKLOG] number of clients
    listen(sfd, BACKLOG);

    int num_clients = 0;
    while (num_clients != 2)
    {
        int cfd;
        cfd = accept(sfd, NULL, NULL);

        if (cfd != -1)
        {
            g->cfd[num_clients] = cfd;
            printf("Got connection from %d", cfd);
            num_clients++;
        }
    }

    g->moves = 0;
    g->turn = 0;
    write(g->cfd[0], "9", 1);

    for (int i = 0; i < 9; i++)
    {
        g->board[i] = (tile){-1, -1};
    }

    return READ;
}

static int readClient(Environment *globals)
{
    printf("read state\n");

    Globals *g;
    g = (Globals *)globals;
    printf("player %d's turn\n", (g->turn + 1));

    int client_input;

    char buf[BUF_SIZE];
    read(g->cfd[g->turn], buf, BUF_SIZE);
    printf("the buf contains %s\n", buf);
    client_input = atoi(buf);

    printf("the byte is %d\n", client_input);

    g->move = (int)client_input;
    printf("the g-move byte is %d\n", g->move);

    // fflush(stdin);
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

    // else if (isdigit(g->move) == 0)
    // {
    //     printf("input is not an integer, please enter an int between 0-8\n");
    //     return SEND_INVALID;
    // }

    else
    {
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
    }

    return SEND_VALID;
}

static int send_valid(Environment *globals)
{
    printf("send valid state\n");

    Globals *g;
    g = (Globals *)globals;


    char c = g->move + '0';
    printf("the move is %c\n", c);
    char move[] = {c};

    // write(g->cfd[g->turn], move, 1);
    write(g->cfd[g->turn], move, 1);

    printf("BOARD: \n");

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

    printf("\n");

    return READ;
}

static int send_invalid(Environment *globals)
{
    Globals *g;
    g = (Globals *)globals;
    printf("send invalid state\n");
    printf("sent retry code: 9\n");
    write(g->cfd[g->turn], "9", 1);
    return READ;
}
