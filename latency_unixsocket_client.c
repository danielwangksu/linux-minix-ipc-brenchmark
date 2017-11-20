/**************************************************************
*   Linux Unix Domain Socket for Latency Testing Client       *
*   by Daniel Wang and Kevin Dennis                           *
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
#include <sys/un.h>  /* For Unix Domain socket struct */

#include <time.h>
#include <sys/time.h>

#define OK  0
#define BILLION  1000000000L
#define MSGSIZE 64 // 1 or 1024 bytes

#define DEST_UDP_PORT 9090 /* receiving port */

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
    int i, nloop, status;
    int len;
    char *destipfile;

    struct sockaddr_un dest_adr;
    //socklen_t dest_adr_len;
    int socket_fd;
    char buffsend[MSGSIZE] = {0};
    char buffrecv[MSGSIZE] = {0};

    struct timespec before = {0, 0};
    struct timespec after = {0, 0};

    if(argc != 3)
        bail("[Error] usage: latency_udpsocket_client {NUM_OF_LOOPS} {SERVER_FILE}");

    nloop = atoi(argv[1]);
    destipfile = argv[2];
    printf("[CLIENT]: DEST FILE = %s\n", destipfile);

    memset(&dest_adr, 0, sizeof dest_adr);
    dest_adr.sun_family = AF_UNIX;
    strcpy(dest_adr.sun_path, destipfile);
    len = strlen(dest_adr.sun_path) + sizeof(dest_adr.sun_family);

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1)
        bail("[Error]: socket() failed");

    if (connect(socket_fd, (struct sockaddr *)&dest_adr, len) == -1)
        bail("[Error]: connect() failed");

    for(i = 0; i < nloop; i++)
    {
        printf("[CLIENT]: sending message\n");
        clock_gettime(CLOCK_MONOTONIC, &before);
        status = send(socket_fd, buffsend, sizeof(buffsend), 0);
        if (status < 0)
            bail("[Error]: sendto() failed");
        status = recv(socket_fd, buffrecv, sizeof(buffrecv), 0);
        if(status < 0)
            bail("[Error]: recvfrom() failed");
        clock_gettime(CLOCK_MONOTONIC, &after);
        printf("sec: %ld, nonosec: %ld\n", diff(before, after).tv_sec, diff(before, after).tv_nsec);
    }
    close(socket_fd);
    return 0;
}
