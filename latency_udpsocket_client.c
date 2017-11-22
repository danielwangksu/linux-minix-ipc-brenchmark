/**************************************************************
*   Linux UDP Socket for Latency Testing     Client           *
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
#include <arpa/inet.h>  /* For socket */
#include <sys/resource.h>

#include <time.h>
#include <sys/time.h>

#define OK  0
#define BILLION  1000000000L
#define MSGSIZE 8*1024

#define DEST_UDP_PORT 9090 /* receiving port */

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
    int i, nloop, msgsize, status, prio;
    char *destip;

    struct sockaddr_in dest_adr;
    socklen_t dest_adr_len;
    int socket_fd;
    char buffsend[MSGSIZE] = {0};
    char buffrecv[MSGSIZE] = {0};

    struct timespec before = {0, 0};
    struct timespec after = {0, 0};

    unsigned long elapse = 0;

    if(argc != 4)
        bail("[Error] usage: latency_udpsocket_client {NUM_OF_LOOPS} {MESSAGE_SIZE} {SERVER_IP}");

    nloop = atoi(argv[1]);
    msgsize = atoi(argv[2]);
    destip = argv[3];
    //printf("[CLIENT]: DEST IP = %s\n", destip);

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[CLIENT]: need more privilege to change priority\n");

    memset(&dest_adr, 0, sizeof dest_adr);
    dest_adr.sin_family = AF_INET;
    dest_adr.sin_port = htons(DEST_UDP_PORT);
    dest_adr.sin_addr.s_addr = inet_addr(destip);
    dest_adr_len = sizeof dest_adr;

    if (dest_adr.sin_addr.s_addr == INADDR_NONE)
        bail("[Error]: bad address");

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
        bail("[Error]: socket() failed");

    sleep(1);

    for(i = 0; i < nloop; i++)
    {   
        //printf("[CLIENT]: sending message to %s\n", inet_ntoa(dest_adr.sin_addr));
        clock_gettime(CLOCK_MONOTONIC, &before);
        status = sendto(socket_fd, buffsend, msgsize, 0, (struct sockaddr *)&dest_adr, dest_adr_len);
        if (status < 0)
            bail("[Error]: sendto() failed");
        status = recvfrom(socket_fd, buffrecv, sizeof(buffrecv), 0, (struct sockaddr *)&dest_adr, &dest_adr_len);
        if(status < 0)
            bail("[Error]: recvfrom() failed");
        clock_gettime(CLOCK_MONOTONIC, &after);

        elapse += diff(before, after);
    }
    printf("[CLIENT]: %d loop average nsec %lu\n", nloop, elapse/nloop);
    close(socket_fd);
    return 0;
}
