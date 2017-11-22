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

int totalnbytes, xfersize;

char buff[MSGSIZE] = {0};

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
 *              recvTotal                       *
 *=================================================================*/
void recvTotal(int socket_fd, int nbytes, struct sockaddr_in client_adr)
{   
    ssize_t n;
    socklen_t client_adr_len;
    client_adr_len = sizeof(client_adr);

    while((nbytes > 0) && ((n = recvfrom(socket_fd, buff, sizeof(buff), 0, (struct sockaddr *) &client_adr, &client_adr_len)) > 0))
    {
        nbytes -= n;
    }
}

/*=================================================================*
 *              main                            *
 *=================================================================*/
int main(int argc, char ** argv)
{
    int prio, i, nloop;

    struct sockaddr_in client_adr, local_adr;
    int socket_fd;

    struct timespec before = {0, 0};
    struct timespec after = {0, 0};

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[SERVER]: need more privilege to change priority\n");

    if(argc != 3)
        bail("[Error] usage: bandwidth_udpsocket_server {NUM_OF_LOOPS} {TOTAL_NUM_OF_MB}");
    nloop = atoi(argv[1]);
    totalnbytes = atoi(argv[2]) * 1024 * 1024;

    memset(&local_adr, 0, sizeof local_adr);
    memset(&client_adr, 0, sizeof client_adr);
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
        clock_gettime(CLOCK_MONOTONIC, &before);
        recvTotal(socket_fd, totalnbytes, client_adr);
        clock_gettime(CLOCK_MONOTONIC, &after);
        printf("sec: %ld, nsec: %ld total bytes: %d\n", diff(before, after).tv_sec, diff(before, after).tv_nsec, totalnbytes);
    }
    close(socket_fd);
    return 0;
}