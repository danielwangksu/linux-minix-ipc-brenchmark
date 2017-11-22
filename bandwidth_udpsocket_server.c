/**************************************************************
*   Linux UDP Socket for bandwidth Testing     Server         *
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
    int prio, status;

    struct sockaddr_in client_adr, local_adr;
    socklen_t client_adr_len;
    int socket_fd;
    char buff[MSGSIZE] = {0};

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[SERVER]: need more privilege to change priority\n");

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

    while(1)
    {
        client_adr_len = sizeof(client_adr);
        status = recvfrom(socket_fd, buff, sizeof(buff), 0, (struct sockaddr *)&client_adr, &client_adr_len);
        if(status < 0)
            bail("[SERVER]: recvfrom(2) failed");
    }
    close(socket_fd);
    return 0;
}