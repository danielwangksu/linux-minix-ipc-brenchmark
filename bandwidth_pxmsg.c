/**************************************************************
*   Linux POSIX Message Queue for bandwidth Testing           *
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
#include <mqueue.h> /* For message queue */

#include <time.h>
#include <sys/time.h>

#define OK  0
#define BILLION  1000000000L

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

    cptr = vptr;
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
void reader(int comfd, mqd_t mqrecv, int totalnbytes)
{
    ssize_t n;
    if(write(comfd, &totalnbytes, sizeof(totalnbytes))!= sizeof(totalnbytes))
        bail("[ERROR]: write error");

    while((totalnbytes > 0) && ((n = mq_receive(mqrecv, buf, xfersize, NULL)) > 0))
    {
        totalnbytes -= n;
    }
}

 /*=================================================================*
 *              writer                          *
 *=================================================================*/
void writer(int comfd, mqd_t mqsend)
{
    int totalnbytes;
    while(1)
    {
        read(comfd, &totalnbytes, sizeof(totalnbytes));

        while(totalnbytes > 0)
        {
            mq_send(mqsend, buf, xfersize, 0);
            totalnbytes -= xfersize;
        }
    }

}

/*=================================================================*
 *              main                            *
 *=================================================================*/
int main(int argc, char ** argv)
{
    int i, nloop, comPipe[2];
    pid_t childpid;

    mqd_t mq;
    struct mq_attr attr;

    struct timespec before = {0, 0};
    struct timespec after = {0, 0};

    if(argc != 4)
        bail("[Error] usage: bandwidth_pxmsg {NUM_OF_LOOPS} {TOTAL_NUM_OF_MB} {BYTES_PER_WRITE}");
    nloop = atoi(argv[1]);
    totalnbytes = atoi(argv[2]) * 1024 * 1024;
    xfersize = atoi(argv[3]);

    buf = malloc(xfersize);
    if(touch(buf, xfersize) == -1)
        bail("[Error] touch error");

    pipe(comPipe);
    mq_unlink("/bandwdithMQ");
    attr.mq_maxmsg = MAXMSG;
    attr.mq_msgsize = xfersize;
    mq = mq_open("/bandwdithMQ", CRW_FLAG, FILE_MODE, &attr);

    if((childpid = fork()) == 0)
    {   /* child SERVER process */
        writer(comPipe[0], mq);
        exit(0);
    }

    /* child CLIENT process */
    for(i = 0; i < nloop; i++)
    {
        sleep(1);
        // get timestamp before
        clock_gettime(CLOCK_MONOTONIC, &before);
        reader(comPipe[1], mq, totalnbytes);
        // get timestamp after
        clock_gettime(CLOCK_MONOTONIC, &after);
        printf("sec: %ld, nonosec: %ld total bytes: %d\n", diff(before, after).tv_sec, diff(before, after).tv_nsec, totalnbytes);
    }

    kill(childpid, SIGTERM);
    mq_close(mq);
    mq_unlink("/bandwdithMQ");
    exit(0);

}
