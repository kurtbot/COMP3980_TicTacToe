#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define PORT 2034
#define BUFSIZE 1024
#define DEBUG 0

typedef struct
{
    int turn;
    int moves;
    int move;
    int p1;
    int p2;
    int board[9];
} Game;

// Begin Function Prototypes

int client_has_game(Game **games, int client, int running_games);
int get_game_pair(Game **games, int client, int running_games);
void init(Game **game, int p1, int p2, int numgames);

void send_to_client(int client, int nbytes_recvd, char *recv_buf, fd_set *master);
void send_recv(int currclient, fd_set *master, int *waitingclient, Game **rooms, int *numrooms);
void connection_accept(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr);
void connect_request(int *sockfd, struct sockaddr_in *my_addr);
static int checkWinner(int board[3][3]);
static int equals3(int a, int b, int c);

// End Function Prototypes

void send_to_client(int client, int nbytes_recvd, char *recv_buf, fd_set *master)
{
    if (FD_ISSET(client, master))
    {
        if (write(client, recv_buf, nbytes_recvd) == -1)
        {
            perror("write");
        }
    }
}

void init(Game **game, int p1, int p2, int numgames)
{
    (*game)[numgames - 1].p1 = p1;
    (*game)[numgames - 1].p2 = p2;
    (*game)[numgames - 1].move = -1;
    int turn = ((p1 % 2 == 0) ? p1 : p2);
    (*game)[numgames - 1].turn = turn;
    for (int i = 0; i < 9; i++)
        (*game)[numgames - 1].board[i] = -1;
    numgames++;
    *game = (Game *)realloc(*game, numgames * sizeof(Game));
}

void finish_games(Game **game, int index)
{
    Game* g = &((*game)[index]);
    g->p1 = -1;
    g->p2 = -1;
    g->move = -1;
    g->turn = -1;
    for (int i = 0; i < 9; i++)
        g->board[i] = -1;
}

int client_has_game(Game **games, int client, int running_games)
{
    int i = 0;
    while (i < running_games)
    {
        if (DEBUG)
            printf("checking %d: contains p1: %d, p2:%d, client: %d\n", i, (*games)[i].p1, (*games)[i].p2, client);
        if ((*games)[i].p1 == client || (*games)[i].p2 == client)
            return i;
        i++;
    }
    return -1;
}

int get_game_pair(Game **games, int client, int running_games)
{
    int i = 0;
    while (i < running_games)
    {
        if ((*games)[i].p1 == client)
        {
            return (*games)[i].p2;
        }
        else if ((*games)[i].p2 == client)
        {
            return (*games)[i].p1;
        }
        i++;
    }
    return -1;
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

void send_recv(int currclient, fd_set *master, int *waitingclient, Game **rooms, int *numrooms)
{
    int nbytes_recvd;
    char recv_buf[BUFSIZE];
    if ((nbytes_recvd = recv(currclient, recv_buf, BUFSIZE, 0)) <= 0)
    {
        if (nbytes_recvd == 0)
        {
            if (DEBUG)
                printf("socket %d hung up\n", currclient);
            send_to_client(get_game_pair(rooms, currclient, *numrooms), 2, "13", master);
            int room_id;
            if ((room_id = client_has_game(rooms, currclient, *numrooms)) != -1){
                finish_games(rooms, room_id); 
            }
            
        }
        // else
        // {
        //     perror("recv");
        // }
        close(currclient);
        FD_CLR(currclient, master);
    }
    else
    {
        if (client_has_game(rooms, currclient, *numrooms) == -1)
        {
            if (DEBUG)
                printf("Waiting Client: %d for current client %d\n", *waitingclient, currclient);
            if (*waitingclient == -1)
            {
                // set as waiting
                if (DEBUG)
                    printf("assigned waiter to %d\n", currclient);
                *waitingclient = currclient;
            }
            else
            {
                // create Game here
                if (DEBUG)
                    printf("incrementing\n");
                init(rooms, *waitingclient, currclient, *numrooms);
                send_to_client(*waitingclient, 1, "9", master);
                *waitingclient = -1;
            }
        }
        else
        {
            if (DEBUG)
                printf("sending\n");
            int roomID = client_has_game(rooms, currclient, *numrooms);
            if (roomID != -1 && (*rooms)[roomID].turn == currclient)
            {
                int input = atoi(recv_buf);
                // check if valid input
                if (input >= 9)
                {
                    fprintf(stderr, "invalid move from %d: Move Out of Bounds\n", currclient);
                    send_to_client(currclient, 1, "9", master);
                    return;
                }

                else
                {
                    if ((*rooms)[roomID].board[input] != -1)
                    {
                        fprintf(stderr, "invalid move from %d: Move Exists\n", currclient);
                        send_to_client(currclient, 1, "9", master);
                        return;
                    }

                    (*rooms)[roomID].board[input] = currclient;

                    (*rooms)[roomID].moves++;
                    (*rooms)[roomID].turn = ((*rooms)[roomID].turn == (*rooms)[roomID].p1) ? (*rooms)[roomID].p2 : (*rooms)[roomID].p1;

                    for (int i = 0; i < 10; i++)
                    {
                        if (i == 9)
                        {
                            // Draw
                            send_to_client(
                                currclient,
                                2,
                                "12",
                                master);
                            send_to_client(
                                get_game_pair(rooms, currclient, *numrooms),
                                2,
                                "12",
                                master);
                            return;
                        }

                        if ((*rooms)[roomID].board[i] == -1)
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
                            board[i][j] = (*rooms)[roomID].board[k++];
                        }
                    }

                    // Check Win
                    int winner;
                    if ((winner = checkWinner(board)) != -1)
                    {
                        send_to_client(
                            currclient,
                            2,
                            (winner == currclient) ? "10" : "11",
                            master);
                        send_to_client(
                            get_game_pair(rooms, currclient, *numrooms),
                            2,
                            (winner == currclient) ? "11" : "10",
                            master);
                    }
                    char test[3];
                    sprintf(test, "%d", input);
                    send_to_client(get_game_pair(rooms, currclient, *numrooms), 1, test, master);
                }
            }
        }
    }
}

void connection_accept(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr)
{
    socklen_t addrlen;
    int newsockfd;

    addrlen = sizeof(struct sockaddr_in);
    if ((newsockfd = accept(sockfd, (struct sockaddr *)client_addr, &addrlen)) == -1)
    {
        perror("accept");
        exit(1);
    }
    else
    {
        FD_SET(newsockfd, master);
        if (newsockfd > *fdmax)
        {
            *fdmax = newsockfd;
        }
        if (DEBUG)
            printf("new connection from %s on port %d \n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    }
}

void connect_request(int *sockfd, struct sockaddr_in *my_addr)
{
    int yes = 1;
    // int tcpfd = 0;

    // tcp
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket TCP error");
        exit(1);
    }
    // tcpfd = *sockfd;
    my_addr->sin_family = AF_INET;
    my_addr->sin_port = htons(PORT);
    my_addr->sin_addr.s_addr = INADDR_ANY;
    memset(my_addr->sin_zero, 0, sizeof my_addr->sin_zero);

    if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    if (bind(*sockfd, (struct sockaddr *)my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Unable to bind");
        exit(1);
    }
    if (listen(*sockfd, 10) == -1)
    {
        perror("listen");
        exit(1);
    }

    // udp
    // if ((*sockfd = socket(AF_INET, SOCK_DGRAM, 0) == -1))
    // {
    //     perror("Socket UDP error");
    //     exit(1);
    // }
    
    bind(*sockfd, (struct sockaddr *)my_addr, sizeof(struct sockaddr));

    printf("\nServer: Waiting for client on port %d\n", PORT);
    fflush(stdout);
}

// int handleTCP(){

// }

// int handleUDP(){
//     int n = 0;

//     // recvfrom
//     // n = recvfrom();

//     // sendto
//     // sendto();

//     return n;
// }

int main()
{
    fd_set master;
    fd_set read_fds;
    int fdmax, i;
    int sockfd = 0;
    int waiting = -1;
    int numrooms = 1;
    struct sockaddr_in my_addr, client_addr;

    Game *games = (Game *)malloc(sizeof(Game) * 1);
    init(&games, -1, -1, numrooms);

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    connect_request(&sockfd, &my_addr);
    FD_SET(sockfd, &master);

    fdmax = sockfd;
    while (1)
    {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }

        for (i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == sockfd)
                    connection_accept(&master, &fdmax, sockfd, &client_addr);
                else
                    send_recv(i, &master, &waiting, &games, &numrooms);
            }
        }
    }

    free(games);
    return 0;
}
