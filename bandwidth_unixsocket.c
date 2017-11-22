/**************************************************************
*   Linux Unix Domain Socket for bandwidth Testing           *
*   by Daniel Wang                                            *
***************************************************************/
#include <stdio.h> /* For printf */
#include <stdlib.h> /* For exit() */
#include <unistd.h> /* For sysconf() */
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
#define UNIX_SOCK_FILE "./unix_sock"

#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define CRW_FLAG   (O_RDWR | O_CREAT)

#define MAXMSG  10

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
 *              reader                          *
 *=================================================================*/
void reader(int socket_fd, int nbytes)
{
    ssize_t n;
    if(send(socket_fd, &nbytes, sizeof(nbytes), 0)!= sizeof(nbytes))
        bail("[ERROR]: write error");

    while((nbytes > 0) && ((n = recv(socket_fd, buf, xfersize, 0)) > 0))
    {
        nbytes -= n;
    }
}

 /*=================================================================*
 *              writer                          *
 *=================================================================*/
void writer(int socket_fd)
{
    int nbytes;
    while(1)
    {
        recv(socket_fd, &nbytes, sizeof(nbytes), 0);

        while(nbytes > 0)
        {
            send(socket_fd, buf, xfersize, 0);
            nbytes -= xfersize;
        }
    }

}

/*=================================================================*
 *              main                            *
 *=================================================================*/
int main(int argc, char ** argv)
{
    int i, nloop, prio;
    pid_t childpid;

    //Server items
    struct sockaddr_un client_adr, local_adr;
    int client_fd;
    int local_fd;
    int len;

    //Client items
    struct sockaddr_un server_adr;
    int server_fd;

    struct timespec before = {0, 0};
    struct timespec after = {0, 0};

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[SERVER]: need more privilege to change priority\n");

    if(argc != 4)
        bail("[Error] usage: bandwidth_pxmsg {NUM_OF_LOOPS} {TOTAL_NUM_OF_MB} {BYTES_PER_WRITE}");
    nloop = atoi(argv[1]);
    totalnbytes = atoi(argv[2]) * 1024 * 1024;
    xfersize = atoi(argv[3]);

    buf = malloc(xfersize);
    if(touch(buf, xfersize) == -1)
        bail("[Error] touch error");

    //Setup server
    memset(&client_adr, 0, sizeof client_adr);
    memset(&local_adr, 0, sizeof local_adr);
    local_adr.sun_family = AF_UNIX;
    strcpy(local_adr.sun_path, UNIX_SOCK_FILE);
    unlink(local_adr.sun_path);
    len = strlen(local_adr.sun_path) + sizeof(local_adr.sun_family);

    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1)
        bail("[SERVER]: socket() failed");

    if(bind(client_fd, (struct sockaddr *) &local_adr, len) < 0)
        bail("[SERVER]: bind() failed");

    listen(client_fd, 1);

    //Setup clients
    memset(&server_adr, 0, sizeof server_adr);
    server_adr.sun_family = AF_UNIX;
    strcpy(server_adr.sun_path, UNIX_SOCK_FILE);
    len = strlen(server_adr.sun_path) + sizeof(server_adr.sun_family);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1)
        bail("[Error]: socket() failed");

    if((childpid = fork()) == 0)
    {   /* child SERVER process */
      if (connect(server_fd, (struct sockaddr *)&server_adr, len) == -1)
          bail("[Error]: connect() failed");

        writer(server_fd);
        exit(0);
    }

    //Wait for child to setup and connect
    sleep(1);
    len = sizeof(client_adr);
    local_fd = accept(client_fd, (struct sockaddr *) &client_adr, (socklen_t *)&len);

    if(local_fd < 0)
      bail("[SERVER]: accept() failed");

    /* child CLIENT process */
    for(i = 0; i < nloop; i++)
    {
        // get timestamp before
        clock_gettime(CLOCK_MONOTONIC, &before);
        reader(local_fd, totalnbytes);
        // get timestamp after
        clock_gettime(CLOCK_MONOTONIC, &after);
        printf("sec: %ld, nsec: %ld total bytes: %d\n", diff(before, after).tv_sec, diff(before, after).tv_nsec, totalnbytes);
    }

    kill(childpid, SIGTERM);
    close(local_fd);
    close(server_fd);
    close(client_fd);
    unlink(UNIX_SOCK_FILE);
    exit(0);
}