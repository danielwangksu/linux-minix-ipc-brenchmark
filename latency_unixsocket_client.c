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
#include <sys/resource.h>

#include <time.h>
#include <sys/time.h>

#define OK  0
#define BILLION  1000000000L
#define MSGSIZE 8*1024

#define UNIX_SERVER_SOCK_FILE "./unix_domain_server"
#define UNIX_CLIENT_SOCK_FILE "./unix_domain_client"

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
    int i, nloop, msgsize, prio;

    struct sockaddr_un server_adr, local_adr;
    int socket_fd;
    ssize_t num_bytes;
    char buffsend[MSGSIZE] = {0};
    char buffrecv[MSGSIZE] = {0};

    struct timespec before = {0, 0};
    struct timespec after = {0, 0};

    unsigned long elapse = 0;

    if(argc != 3)
        bail("[Error] usage: latency_udpsocket_client {NUM_OF_LOOPS} {MESSAGE_SIZE}");

    nloop = atoi(argv[1]);
    msgsize = atoi(argv[2]);

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[CLIENT]: need more privilege to change priority\n");

    socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socket_fd == -1)
        bail("[Error]: socket() failed");

    /* setup local socket */
    memset(&local_adr, 0, sizeof local_adr);
    local_adr.sun_family = AF_UNIX;

    if (strlen(UNIX_CLIENT_SOCK_FILE) > sizeof(local_adr.sun_path) - 1)
        bail("[CLIENT]: fail socket path too long");
    strncpy(local_adr.sun_path, UNIX_CLIENT_SOCK_FILE, sizeof(local_adr.sun_path) - 1);

    if (bind(socket_fd, (struct sockaddr *)&local_adr, sizeof(struct sockaddr_un)) == -1)
        bail("[Error]: bind() failed");

    /* setup server address */
    memset(&server_adr, 0, sizeof server_adr);
    server_adr.sun_family = AF_UNIX;
    strncpy(server_adr.sun_path, UNIX_SERVER_SOCK_FILE, sizeof(server_adr.sun_path) - 1);

    sleep(1);

    for(i = 0; i < nloop; i++)
    {
        // printf("[CLIENT]: sending message\n");
        clock_gettime(CLOCK_MONOTONIC, &before);
        if (sendto(socket_fd, buffsend, msgsize, 0, (struct sockaddr *) &server_adr,
                sizeof(struct sockaddr_un)) != msgsize)
            bail("[CLIENT]: sendto() failed");

        num_bytes = recvfrom(socket_fd, buffrecv, MSGSIZE, 0, NULL, NULL);
        if(num_bytes == -1)
            bail("[CLIENT]: recvfrom() failed");
        clock_gettime(CLOCK_MONOTONIC, &after);

        elapse += diff(before, after);
    }
    
    printf("[Server]: %d loop average nsec %lu\n", nloop, elapse/nloop);

    close(socket_fd);
    unlink(UNIX_SERVER_SOCK_FILE);
    unlink(UNIX_CLIENT_SOCK_FILE); 
    return 0;
}