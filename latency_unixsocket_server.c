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
    unsigned int len;

    struct sockaddr_un client_adr, local_adr;
    //socklen_t client_adr_len;
    //socklen_t local_adr_len;
    int socket_fd;
    int socket_fd2;

    char buffsend[MSGSIZE] = {-1};
    char buffrecv[MSGSIZE] = {-1};

    memset(&client_adr, 0, sizeof client_adr);
    memset(&local_adr, 0, sizeof local_adr);
    local_adr.sun_family = AF_UNIX;
    strcpy(local_adr.sun_path, "./unix_domain_server");
    unlink(local_adr.sun_path);
    len = strlen(local_adr.sun_path) + sizeof(local_adr.sun_family);
    //local_adr_len = sizeof local_adr;

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1)
        bail("[SERVER]: socket() failed");

    if(bind(socket_fd, (struct sockaddr *) &local_adr, len) < 0)
        bail("[SERVER]: bind() failed");

    listen(socket_fd, 1);

    len = sizeof(client_adr);
    socket_fd2 = accept(socket_fd, (struct sockaddr *) &client_adr, &len);

    if(socket_fd2 < 0)
      bail("[SERVER]: accept() failed");

    while(1)
    {
        printf("[SERVER]: start receiving \n");
        status = recv(socket_fd2, buffrecv, sizeof(buffrecv), 0);
        if(status < 0)
            bail("[SERVER]: recvfrom(2) failed");
        status = send(socket_fd2, buffsend, sizeof(buffsend), 0);
        if(status < 0)
            bail("[SERVER]: sendto() failed");
    }
    close(socket_fd);
    return 0;
}
