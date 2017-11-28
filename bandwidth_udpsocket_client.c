/**************************************************************
*   Linux UDP Socket for bandwidth Testing     Client         *
*   by Daniel Wang                                            *
***************************************************************/
#include <stdio.h> /* For printf */
#include <stdlib.h> /* For exit() */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
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

void *buf;
int totalnbytes, xfersize;

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
 *              touch                           *
 *=================================================================*/
int touch(void *vptr, int nbytes)
{
    char *cptr;
    static int pagesize = 0;

    if (pagesize == 0)
    {
#ifdef  _SC_PAGESIZE
        if ((pagesize=sysconf(_SC_PAGESIZE)) == -1)
            return(-1);
#else
        pagesize = getpagesize();
#endif
    }

    cptr = (char *) vptr;
    while (nbytes > 0) {
        *cptr = 1;
        cptr += pagesize;
        nbytes -= pagesize;
    }
    return(0);
}

/*=================================================================*
 *              sendTotal                       *
 *=================================================================*/
void sendTotal(int socket_fd, int total, struct sockaddr_in dest_adr, socklen_t len)
{
    void* offset;
    int n, sendall;
    offset = buf;
    sendall = total;

    while(total > 0)
    {
        n = sendto(socket_fd, offset, xfersize, 0, (struct sockaddr *)&dest_adr, len);
        if(n > 0)
        {
            offset = offset + n;
            sendall -= n;
            if(sendall == 0)
                offset = buf;
        }
    }

}

/*=================================================================*
 *              main                            *
 *=================================================================*/
int main(int argc, char ** argv)
{
    int i, nloop, prio;
    char *destip;

    struct sockaddr_in dest_adr;
    int socket_fd;

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[CLIENT]: need more privilege to change priority\n");

    if(argc != 5)
        bail("[Error] usage: bandwidth_udpsocket_client {NUM_OF_LOOPS} {TOTAL_NUM_OF_MB} {BYTES_PER_WRITE} {SERVER_IP}");
    nloop = atoi(argv[1]);
    totalnbytes = atoi(argv[2]) * 1024 * 1024;
    xfersize = atoi(argv[3]);
    destip = argv[4];

    buf = malloc(totalnbytes);
    if(buf == NULL)
        bail("cannot allocate that much");
    if(touch(buf, totalnbytes) == -1)
        bail("[Error] touch error");

    memset(&dest_adr, 0, sizeof dest_adr);
    dest_adr.sin_family = AF_INET;
    dest_adr.sin_port = htons(DEST_UDP_PORT);
    dest_adr.sin_addr.s_addr = inet_addr(destip);


    if (dest_adr.sin_addr.s_addr == INADDR_NONE)
        bail("[Error]: bad address");

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
        bail("[Error]: socket() failed");
    
    sleep(1);

    for(i = 0; i < nloop; i++)
    {
        sendTotal(socket_fd, totalnbytes, dest_adr, sizeof dest_adr);

    }
    free(buf);
    close(socket_fd);
    return 0;
}