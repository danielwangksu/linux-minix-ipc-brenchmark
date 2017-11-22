/**************************************************************
*   Linux Unix Domain Socket for Latency Testing Server       *
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
#include <arpa/inet.h> /* For socket related */
#include <sys/un.h>  /* For Unix Domain socket struct */
#include <sys/resource.h>

#include <time.h>
#include <sys/time.h>

#define OK  0
#define BILLION  1000000000L
#define MSGSIZE 8*1024

#define UNIX_SERVER_SOCK_FILE "./unix_domain_server"

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
    int i, nloop, msgsize, prio, len;

    ssize_t num_bytes;
    struct sockaddr_un client_adr, local_adr;

    int server_fd;

    char buffsend[MSGSIZE] = {0};
    char buffrecv[MSGSIZE] = {0};

    if(argc != 3)
        bail("[Error] usage: latency_unixsocket_server {NUM_OF_LOOPS} {MESSAGE_SIZE}");
    nloop = atoi(argv[1]);
    msgsize = atoi(argv[2]);

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[SERVER]: need more privilege to change priority\n");

    memset(&client_adr, 0, sizeof client_adr);
    memset(&local_adr, 0, sizeof local_adr);

    server_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (server_fd == -1)
        bail("[SERVER]: socket() failed");

    if (strlen(UNIX_SERVER_SOCK_FILE) > sizeof(local_adr.sun_path) - 1)
        bail("[SERVER]: fail socket path too long");

    local_adr.sun_family = AF_UNIX;
    strncpy(local_adr.sun_path, UNIX_SERVER_SOCK_FILE, sizeof(local_adr.sun_path) - 1);
    unlink(local_adr.sun_path);

    if(bind(server_fd, (struct sockaddr *) &local_adr, sizeof(struct sockaddr_un)) < 0)
        bail("[SERVER]: bind() failed");

    for(i = 0; i < nloop; i++)
    {
        //printf("[SERVER]: start receiving \n");
        len = sizeof(struct sockaddr_un);
        num_bytes = recvfrom(server_fd, buffrecv, MSGSIZE, 0, (struct sockaddr *) &client_adr, (socklen_t *) &len);
        if (num_bytes != msgsize)
            bail("[SERVER]: recvfrom() failed");

        if(sendto(server_fd, buffsend, num_bytes, 0, (struct sockaddr *) &client_adr, len) != num_bytes)
            bail("[SERVER]: sendto() failed");
    }
    close(server_fd);
    unlink(UNIX_SERVER_SOCK_FILE);
    return 0;
}