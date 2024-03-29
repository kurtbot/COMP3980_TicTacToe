#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 4950
#define BUFSIZE 1024

void send_to_all(int target, int sender, int sockfd, int nbytes_recvd, char *recv_buf, fd_set *master)
{
    if (FD_ISSET(target, master))
    {
        if (target != sockfd && target != sender)
        {
            if (send(target, recv_buf, nbytes_recvd, 0) == -1)
            {
                perror("send");
            }
        }
    }
}

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

void handle_request(char *msg)
{
    // check value
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

void send_recv(int currclient, fd_set *master, int sockfd, int fdmax, int *waitingclient)
{
    int nbytes_recvd;
    char recv_buf[BUFSIZE], buf[BUFSIZE];

    if ((nbytes_recvd = read(currclient, recv_buf, BUFSIZE)) <= 0)
    {
        if (nbytes_recvd == 0)
        {
            printf("socket %d hung up\n", currclient);
        }
        else
        {
            perror("recv");
        }
        close(currclient);
        FD_CLR(currclient, master);
    }
    else
    {
        //	printf("%s\n", recv_buf);]
        // parse message here and do shit

        if (recv_buf == "0")
        {
            // find pair
            if (*waitingclient == -1)
            {
                // set as waiting
                *waitingclient = currclient;
            }
            else
            {
                // match
                char *wtc = *waitingclient + '0';
                char *ctw = currclient + '0';
                send_to_client(currclient, strlen(wtc), wtc, master);
                send_to_client(*waitingclient, strlen(ctw), ctw, master);
                *waitingclient = -1;
            }
        }
        else
        {
            int target_client;
            char *msg = strtok(recv_buf, ' ');
            target_client = atoi(msg[0]);
            char *board = msg[1];
            char move = msg[2];
            char senderNo = msg[3];
            for (int i = 0; i < 9; ++i)
            {
                if (board[i] == move)
                    // move exists
                    send_to_client(currclient, 1, "9", master);
            }

            for (int i = 0; i < 10; i++)
            {
                if (i == 9)
                {
                    send_to_client(currclient, 2, "14", master);
                    send_to_client(target_client, 2, "14", master);
                }

                if (board[i] == '_')
                {
                    break;
                }
            }

            // check winner
            int brd[3][3] = {{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}};

            int k = 0;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    if (brd[k] != '_')
                        brd[i][j] = atoi(board[k++]);
                }
            }

            int winner;
            if ((winner = checkWinner(brd)) != -1)
            {
                if (winner + '0' == senderNo)
                {
                    send_to_client(currclient, 2, "12", master);
                    send_to_client(target_client, 2, "13", master);
                }
                else
                {
                    send_to_client(currclient, 2, "13", master);
                    send_to_client(target_client, 2, "12", master);
                }
            }
        }

        // if (0)
        // find a pair
        // if pair is found
        //  send current client pair code
        //  send to pair curr client code
        // or
        //  place in waiting
        //  send 11 to curr client

        // else
        //
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
        printf("new connection from %s on port %d \n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    }
}

int main()
{
    fd_set master;
    fd_set read_fds;
    int fdmax, i;
    int sockfd = 0;
    int waiting = -1;
    struct sockaddr_in my_addr, client_addr;

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
                    send_recv(i, &master, sockfd, fdmax, &waiting);
            }
        }
    }
    return 0;
}