/**************************************************************
*   Linux UDP Socket for Latency Testing     Server           *
*   by Daniel Wang                                            *
***************************************************************/

#include <stdio.h> /* For printf */
#include <stdlib.h> /* For exit() */
#include <unistd.h>
#include <sys/types.h>
#include <signal.h> /* For kill() */
#include <string.h>
#include <fcntl.h>  /* For O_* constants */
#include <sys/stat.h>   /* For mode constants */
#include <arpa/inet.h> /* For socket related */

#include <time.h>
#include <sys/time.h>

#define OK  0
#define BILLION  1000000000L
#define MSGSIZE 64 // 1 or 1024 bytes

#define DEST_UDP_PORT 9090 /* 9090 receive port */

// fault handler
static void bail(const char *on_what) {
    perror(on_what);
}

/*=================================================================*
 *              diff                            *
 *=================================================================*/
struct timespec diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec -1;
        temp.tv_nsec = (BILLION + end.tv_nsec) - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

/*=================================================================*
 *              main                            *
 *=================================================================*/

int main(int argc, char ** argv)
{
    int status;

    struct sockaddr_in client_adr, local_adr;
    socklen_t client_adr_len;
    //socklen_t local_adr_len;
    int socket_fd;

    char buffsend[MSGSIZE];
    char buffrecv[MSGSIZE];

    memset(&client_adr, 0, sizeof client_adr);
    memset(&local_adr, 0, sizeof local_adr);
    local_adr.sin_family = AF_INET;
    local_adr.sin_port = htons(DEST_UDP_PORT);
    local_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    //local_adr_len = sizeof local_adr;

    if (local_adr.sin_addr.s_addr == INADDR_NONE)
        bail("[SERVER]: bad address");

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
        bail("[SERVER]: socket() failed");

    if(bind(socket_fd, (struct sockaddr *) &local_adr, sizeof(local_adr)) < 0)
        bail("[SERVER]: bind() failed");

    while(1)
    {
        client_adr_len = sizeof(client_adr);
        printf("[SERVER]: start receiving %s\n", inet_ntoa(local_adr.sin_addr));
        status = recvfrom(socket_fd, buffrecv, sizeof(buffrecv), 0, (struct sockaddr *)&client_adr, &client_adr_len);
        if(status < 0)
            bail("[INPROXY]: recvfrom(2) failed");
        status = sendto(socket_fd, buffsend, sizeof(buffsend), 0, (struct sockaddr *)&client_adr, client_adr_len);
        if(status < 0)
            bail("[INPROXY]: sendto() failed");
    }
    close(socket_fd);
    return 0;
}