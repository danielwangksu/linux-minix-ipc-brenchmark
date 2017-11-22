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
#include <sys/resource.h>

#include <time.h>
#include <sys/time.h>

#define OK  0
#define BILLION  1000000000L
#define MSGSIZE 8*1024

#define DEST_UDP_PORT 9090 /* 9090 receive port */

// fault handler
static void bail(const char *on_what) {
    perror(on_what);
}

/*=================================================================*
 *              diff                            *
 *=================================================================*/
unsigned long diff(struct timespec start, struct timespec end)
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
    return (unsigned long)temp.tv_sec * BILLION + (unsigned long)temp.tv_nsec;
}

/*=================================================================*
 *              main                            *
 *=================================================================*/

int main(int argc, char ** argv)
{
    int status, nloop, i, msgsize, prio;
    struct sockaddr_in client_adr, local_adr;
    socklen_t client_adr_len;
    int socket_fd;

    char buffsend[MSGSIZE] = {0};
    char buffrecv[MSGSIZE] = {0};

    if(argc != 3)
        bail("[Error] usage: latency_unixsocket_server {NUM_OF_LOOPS} {MESSAGE_SIZE}");
    nloop = atoi(argv[1]);
    msgsize = atoi(argv[2]);

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[CLIENT]: need more privilege to change priority\n");

    memset(&client_adr, 0, sizeof client_adr);
    memset(&local_adr, 0, sizeof local_adr);
    local_adr.sin_family = AF_INET;
    local_adr.sin_port = htons(DEST_UDP_PORT);
    local_adr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (local_adr.sin_addr.s_addr == INADDR_NONE)
        bail("[SERVER]: bad address");

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
        bail("[SERVER]: socket() failed");

    if(bind(socket_fd, (struct sockaddr *) &local_adr, sizeof(local_adr)) < 0)
        bail("[SERVER]: bind() failed");

    for(i = 0; i < nloop; i++)
    {
        client_adr_len = sizeof(client_adr);
        //printf("[SERVER]: start receiving %s\n", inet_ntoa(local_adr.sin_addr));
        status = recvfrom(socket_fd, buffrecv, sizeof(buffrecv), 0, (struct sockaddr *)&client_adr, &client_adr_len);
        if(status < 0)
            bail("[SERVER]: recvfrom(2) failed");
        status = sendto(socket_fd, buffsend, msgsize, 0, (struct sockaddr *)&client_adr, client_adr_len);
        if(status < 0)
            bail("[SERVER]: sendto() failed");
    }
    close(socket_fd);
    return 0;
}