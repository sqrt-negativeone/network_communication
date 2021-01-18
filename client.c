#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 1234
#define STDIN 0

int main(void){
    int sockfd, maxfd;
    struct sockaddr_in serv_addr;
    char msg [1024];
    char msg_recv[1024];

    if ( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Couldn't create socket\n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_aton("127.0.0.1", &(serv_addr.sin_addr));
    memset(&serv_addr.sin_zero, '\0', sizeof serv_addr.sin_zero);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof (struct sockaddr)) < 0){
        fprintf(stderr, "Couldn't connect to server\n");
        exit(1);
    }

    
    fd_set readfds, allfds;

    FD_ZERO(&allfds);
    FD_ZERO(&readfds);

    FD_SET(sockfd, &allfds);
    FD_SET(STDIN, &allfds);

    maxfd = sockfd;

    //main client loop
    while (1){
        readfds = allfds;
        int n;
        if ((n = select(maxfd + 1, &readfds, NULL, NULL, NULL)) < 0){
            fprintf(stderr, "Error occured when select()\n");
            exit(1);
        }
        if (n > 0) printf("\n>>> ");
        if (FD_ISSET(STDIN, &readfds)){
            //there is something from the stdin
            scanf(" %[^\n]",msg);
            if (msg[0] == '\0') continue;
            send(sockfd, msg, strlen(msg), 0);
        }
        else if (FD_ISSET(sockfd, &readfds)){
            //there is something from the server
            int nbyte_rec;
            if ((nbyte_rec = recv(sockfd, msg_recv, 1024, 0)) < 0){
                fprintf(stderr, "Error when recv()\n");
                exit(1);
            }
            if (nbyte_rec == 0){
                printf("Server Closed\n");
                break;
            }
            msg_recv[nbyte_rec] = '\0';
            printf("%s\n ", msg_recv);
        }
    }
    close(sockfd);
    return 0;
}