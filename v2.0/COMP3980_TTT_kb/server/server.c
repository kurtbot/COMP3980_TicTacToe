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

#define PORT 8080
#define BUFSIZE 1024

typedef struct
{
    int p1;
    int p2;
} room;

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

int check_chatrooms_pexist(room **rooms, int client, int roomsize)
{
    printf("checking\n");
    int i = 0;
    while (i < roomsize)
    {
        printf("checking %d: contains p1: %d, p2:%d, client: %d\n", i, (*rooms)[i].p1, (*rooms)[i].p2, client);
        if((*rooms)[i].p1 == client || (*rooms)[i].p2 == client)
            return 1;
        i++;
    }
    printf("ret 0\n");
    return 0;
}

int get_chatroom_pair(room **rooms, int client, int roomsize)
{
    int i = 0;
    while (i < roomsize)
    {
        if ((*rooms)[i].p1 == client)
        {
            return (*rooms)[i].p2;
        }
        else if ((*rooms)[i].p2 == client)
        {
            return (*rooms)[i].p1;
        }
        i++;
    }
    return -1;
}

void send_recv(int currclient, fd_set *master, int *waitingclient, room **rooms, int *numrooms)
{
    int nbytes_recvd;
    char recv_buf[BUFSIZE];
    if ((nbytes_recvd = recv(currclient, recv_buf, BUFSIZE, 0)) <= 0)
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
        // if client is in a room
        // find pair
        // if (currclient is not in chat rooms)
        if (!check_chatrooms_pexist(rooms, currclient, *numrooms))
        {
            printf("Waiting Client: %d for current client %d\n", *waitingclient, currclient);
            if (*waitingclient == -1)
            {
                // set as waiting
                printf("assigned waiter to %d\n", currclient);
                *waitingclient = currclient;
            }
            else
            {
                char *f = "Match found";
                send_to_client(currclient, strlen(f), f, master);
                send_to_client(*waitingclient, strlen(f), f, master);

                // create chat room here
                (*rooms)[*numrooms].p1 = *waitingclient;
                (*rooms)[*numrooms].p2 = currclient;
                printf("incrementing\n");
                *numrooms = *numrooms + 1;
                printf("numrooms %d\n", *numrooms);
                *rooms = (room*)realloc(*rooms, *numrooms * sizeof(room) + 1);
                *waitingclient = -1;
            }
        }
        else
        {
            printf("sending\n");

            send_to_client(get_chatroom_pair(rooms, currclient, *numrooms), nbytes_recvd, recv_buf, master);
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
        printf("new connection from %s on port %d \n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    }
}

void connect_request(int *sockfd, struct sockaddr_in *my_addr)
{
    int yes = 1;

    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
        exit(1);
    }

    my_addr->sin_family = AF_INET;
    my_addr->sin_port = htons(8080);
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
    printf("\nTCPServer Waiting for client on port 8080\n");
    fflush(stdout);
}

int main()
{
    time_t t;
    fd_set master;
    fd_set read_fds;
    int fdmax, i;
    int sockfd = 0;
    int waiting = -1;
    int numrooms = 1;
    struct sockaddr_in my_addr, client_addr;

    room *rooms = (room*) malloc(sizeof(room));
    rooms[0].p1 = -1;
    rooms[0].p2 = -1;
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
                    send_recv(i, &master, &waiting, &rooms, &numrooms);
            }
        }
    }
    return 0;
}
