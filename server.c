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
#define MAX_CONNECTIONS 20
typedef struct usernames_ls usernames_ls;
typedef struct clients_info clients_info;
typedef enum CLIENT_STATUS CLIENT_STATUS;


enum CLIENT_STATUS{
    OK,
    WAITING_FOR_USERNAME_RESPONSE
};


struct clients_info
{
    int sockfd;
    char name[20];
    char IP[INET_ADDRSTRLEN];
    CLIENT_STATUS status;

    //double linked list to remove clients in O(1)
    clients_info* next;
    clients_info* prev;
};

int find_max(clients_info *cl){
    if (cl == NULL) return -1;
    int ans = find_max(cl->next);
    return (ans < cl->sockfd)? cl->sockfd : ans; 
}


int main(void){

    int sockfd, maxfd;
    struct sockaddr_in my_addr;
    char msg [1024];

    if ( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Couldn't create socket\n");
        exit(1);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

    if (bind(sockfd, (struct sockaddr*) &my_addr, sizeof my_addr) < 0){
        fprintf(stderr, "Couldn't bind to port\n");
        exit(1);
    }
    if (listen(sockfd, -1) == -1){
        fprintf(stderr, "Couldn't listen on the port %d\n",PORT);
        exit(1);
    }
    fd_set readfds, allfds;
    clients_info *clients = NULL, *clt, *clt2;

    int connections_count = 0;

    FD_ZERO(&allfds);
    FD_ZERO(&readfds);

    FD_SET(sockfd, &allfds);
    maxfd = sockfd;

    printf("Waiting for connection\n");
    //main program loop
    while (1){
        readfds = allfds;
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0){
            fprintf(stderr, "Error occured when select()\n");
            exit(1);
        }

        if (FD_ISSET(sockfd, &readfds)){
            //new connection
            struct sockaddr_in cl;
            int len = sizeof cl;
            int client_fd;
            if ( (client_fd = accept(sockfd,(struct sockaddr*) &cl, &len)) < 0){
                fprintf(stderr, "Couldn't accept\n");
                exit(1);
            }
            FD_SET(client_fd, &allfds);
            if (client_fd > maxfd) maxfd = client_fd;

            clients_info* new_clinet = malloc(sizeof(clients_info));
            new_clinet->name[0]='\0';
            inet_ntop(cl.sin_family, &cl.sin_addr,new_clinet->IP,sizeof new_clinet->IP);
            new_clinet->sockfd = client_fd;
            new_clinet->status = WAITING_FOR_USERNAME_RESPONSE;

            new_clinet->next = clients;
            new_clinet->prev = NULL;

            clients = new_clinet;
            connections_count++;

            printf("-[new connection from] %s \n", new_clinet->IP);
            strcpy(msg, "[Server] Please enter username (username should be no more than 20 lowercase letters) : ");
            int bytes_sent = send(client_fd,msg, strlen(msg),0);
        }
        else {
            clients_info* tmp = NULL;
            for (clt = clients; clt != NULL; clt = clt->next){
                if (tmp != NULL) free(tmp);
                tmp = NULL;
                if (FD_ISSET(clt->sockfd,&readfds)){
                    memset(msg, 0, strlen(msg)*sizeof(char));
                    int n = recv(clt->sockfd, msg, 1024,0);
                    if (n == 0){
                        FD_CLR(clt->sockfd,&allfds);
                        if (clt->prev != NULL)
                            clt->prev->next = clt->next;
                        else {
                            clients = clt->next;
                        }
                        tmp = clt;
                        connections_count--;
                        close(clt->sockfd);
                        if (clt->sockfd == maxfd) maxfd = find_max(clients);

                        printf("[Client disconnected] ");
                        if (clt->name != "")
                            printf("%s\n", clt->name);
                        else {
                            printf("%s\n", clt->IP);
                        }
                        break;
                    }
                    else {
                        //check the state of the clients
                        switch(clt->status){
                            case OK:{
                                //build msg
                                char msg_out[1024];
                                msg_out[0] = '\0';
                                strcat(msg_out,"[");
                                strcat(msg_out,clt->name);
                                strcat(msg_out,"] : ");
                                strcat(msg_out, msg);
                                printf("[Message] from\n\t%s\n", msg_out);
                                //send the msg to other users
                                for (clt2 = clients; clt2 != NULL; clt2 = clt2->next){
                                    if (clt2 == clt) continue;
                                    send(clt2->sockfd, msg_out,strlen(msg_out), 0);
                                }
                                break;
                            }
                            case WAITING_FOR_USERNAME_RESPONSE:{
                                //the msg is a respons should be the username
                                //loop over all the users and see if there is this name
                                int found = 0;
                                if (strcmp(msg,"server") == 0) found = 1;
                                for (clt2 = clients; clt2 != NULL && !found; clt2 = clt2->next){
                                    if (clt2 == clt) continue;
                                    if (strcmp(msg, clt2->name) == 0){
                                        found = 1;
                                        break;
                                    }
                                }
                                if ( !found){
                                    printf("[Username response] from\n\t%s\n", msg);
                                    strcpy(clt->name,msg);
                                    strcpy(msg, "[Server] Hello ");
                                    strcat(msg, clt->name);
                                    clt->status = OK;
                                }
                                else {
                                    //username in use...
                                    memset(msg, 0, strlen(msg));
                                    strcpy(msg, "[Server] Invalide username... try again");
                                }
                                send(clt->sockfd, msg, strlen(msg), 0);
                                break;
                            }
                            default : break;
                        }
                        
                    }
                }
            }
        }
    }
    
    close(sockfd);
    return 0;
}