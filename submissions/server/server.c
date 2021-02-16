#include "kfsm.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#define PORT 4200

#define BACKLOG 2
#define BUF_SIZE 1024
#define DEBUG 0

typedef enum
{
    SETUP = STATE_START,
    READ,
    VERIFY,
    SEND_VALID,
    SEND_INVALID,
    ERROR,
    DRAW,
    WIN
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
    int winner;
    tile board[9];
} Globals;

// FUNCTION PROTO

static int setup(Environment *globals);
static int readClient(Environment *globals);
static int send_valid(Environment *globals);
static int send_invalid(Environment *globals);
static int verify(Environment *globals);
static int win(Environment *globals);
static int draw(Environment *globals);
static int common_error(Environment *globals);

static int checkWinner(int board[3][3]);
static int equals3(int a, int b, int c);

// MAIN

int main(int argc, char const *argv[])
{
    Globals globals;

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
            {SEND_VALID, WIN, &win},
            {SEND_VALID, DRAW, &draw},
            {WIN, STATE_EXIT, NULL},
            {WIN, ERROR, &common_error},
            {DRAW, STATE_EXIT, NULL},
            {DRAW, ERROR, &common_error},
            {SEND_INVALID, READ, &readClient},
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
            if (DEBUG)
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
    if (DEBUG)
        printf("read state\n");

    Globals *g;
    g = (Globals *)globals;
    printf("player %d's turn\n", (g->turn + 1));

    int client_input;
    ssize_t num_read;
    char buf[BUF_SIZE];
    read(g->cfd[g->turn], buf, BUF_SIZE);
    if (DEBUG)
        printf("the buf contains %s\n", buf);
    client_input = atoi(buf);

    if (DEBUG)
        printf("the byte is %d\n", client_input);

    g->move = (int)client_input;
    if (DEBUG)
        printf("the g-move byte is %d\n", g->move);

    // fflush(stdin);
    return VERIFY;
}

static int verify(Environment *globals)
{
    if (DEBUG)
        printf("verify state\n");

    Globals *g;
    g = (Globals *)globals;

    if (g->move >= 9)
    {
        printf("invalid move: must be 0-8\n");
        return SEND_INVALID;
    }

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
    if (DEBUG)
        printf("send valid state\n");

    Globals *g;
    g = (Globals *)globals;

    char c = g->move + '0';
    printf("the move is %c\n", c);
    char move[] = {c};

    // write(g->cfd[g->turn], move, 1);

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

    // CHECKS

    // Check Draw
    for (int i = 0; i < 10; i++)
    {
        if (i == 9)
        {
            return DRAW;
        }

        if (g->board[i].owner == -1)
        {
            break;
        }
    }

    int board[3][3];

    int k = 0;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            board[i][j] = g->board[k++].owner;
        }
    }

    // Check Win
    int winner;
    if ((winner = checkWinner(board)) != -1)
    {
        g->winner = winner;
        return WIN;
    }

    write(g->cfd[g->turn], move, 1);
    printf("\n");

    return READ;
}

static int send_invalid(Environment *globals)
{
    Globals *g;
    g = (Globals *)globals;
    if (DEBUG)
        printf("send invalid state\n");
    printf("sent retry code: 9\n");
    write(g->cfd[g->turn], "9", 1);
    return READ;
}

static int win(Environment *globals)
{
    Globals *g;
    g = (Globals *)globals;
    write(g->cfd[0], (g->winner == 0) ? "10" : "11", 2);
    write(g->cfd[1], (g->winner == 0) ? "10" : "11", 2);
    return STATE_EXIT;
}

static int draw(Environment *globals)
{
    Globals *g;
    g = (Globals *)globals;
    write(g->cfd[0], "12", 2);
    write(g->cfd[1], "12", 2);
    return STATE_EXIT;
}

static int checkWinner(int board[3][3])
{
    int winner = -1;

    // vertical
    for (int i = 0; i < 3; i++)
    {
        if (equals3(board[i][0], board[i][1], board[i][2]))
            winner = board[i][0];
    }

    // horizontal
    for (int i = 0; i < 3; i++)
    {
        if (equals3(board[0][i], board[1][i], board[2][i]))
            winner = board[i][0];
    }

    // diagonal
    for (int i = 0; i < 3; i++)
    {
        if (equals3(board[0][0], board[1][1], board[2][2]))
            winner = board[0][0];
    }

    for (int i = 0; i < 3; i++)
    {
        if (equals3(board[2][0], board[1][1], board[0][2]))
            winner = board[2][0];
    }

    return winner;
}

static int equals3(int a, int b, int c)
{
    return (a == b && b == c && a != -1);
}