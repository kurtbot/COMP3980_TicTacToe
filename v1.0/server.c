#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#define PORT 8080

#define BACKLOG 5

int main(int argc, char const *argv[])
{
    
    struct sockaddr_in addr;
    int sfd;
    int enable = 1;

    // setup socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(struct sockaddr_in));

    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    
    //
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind server to address
    bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    // listen to [BACKLOG] number of clients
    listen(sfd, BACKLOG);
    
    for(;;)
    {
        int cfd;
        ssize_t num_read;
        char* buf = "hello";

        // while((num_read = read(cfd, buf, BUF_SIZE)) > 0)
        // {
        //     write(STDOUT_FILENO, buf, num_read);
        // }
        cfd = accept(sfd, NULL, NULL);
        printf("Got connection from %d", cfd);

        write(cfd, buf, 5);
        
        close(cfd);
    }
    
    // dc_close(sfd); <- never reached because for(;;) never ends.

    return EXIT_SUCCESS;
}
