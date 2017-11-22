/**************************************************************
*   Linux POSIX Message Queue for Latency Testing             *
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
#include <mqueue.h> /* For message queue */
#include <sys/resource.h>

#include <time.h>
#include <sys/time.h>

#define OK  0
#define BILLION  1000000000L

#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define CRW_FLAG   (O_RDWR | O_CREAT)

#define MAXMSG  10
#define MSGSIZE 8*1024

struct timeval tv_start, tv_stop;

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
 *              tv_sub                            *
 *=================================================================*/
void tv_sub(struct timeval *out, struct timeval *in)
{
    if ((out->tv_usec -= in->tv_usec) < 0) {   /* out -= in */
        --out->tv_sec;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

/*=================================================================*
 *              start_time                            *
 *=================================================================*/
int start_time(void)
{
    return(gettimeofday(&tv_start, NULL));
}


/*=================================================================*
 *              stop_time                            *
 *=================================================================*/
double stop_time(void)
{
    double  clockus;

    if (gettimeofday(&tv_stop, NULL) == -1)
        return(0.0);
    tv_sub(&tv_stop, &tv_start);
    clockus = tv_stop.tv_sec * 1000000.0 + tv_stop.tv_usec;
    if(clockus == 0.0)
        bail("[ERROR] stop_time error");
    return(clockus);
}

/*=================================================================*
 *              send_message                    *
 *=================================================================*/
unsigned long send_message(mqd_t mqsend, mqd_t mqrecv, int msgsize, int round)
{
    char buff[MSGSIZE] = {0};
    int status;
    int i;
    unsigned long elapse;
    struct timespec before = {0, 0};
    struct timespec after = {0, 0};

    for(i = 0; i < MSGSIZE; i++)
        buff[i] = 'A';

    for(i = 0; i < MSGSIZE; i++)
        buff[i] += round;

    clock_gettime(CLOCK_MONOTONIC, &before);
    //start_time();
    status = mq_send(mqsend, buff, msgsize, 0);
    if(status != OK)
        bail("[CLIENT]: mq_send error");
    if (mq_receive(mqrecv, buff, msgsize, NULL) != msgsize)
        bail("[CLIENT]: mq_receive error");
    clock_gettime(CLOCK_MONOTONIC, &after);
    elapse = diff(before, after);
    //printf("Cost: %f usec\n", stop_time());
    // printf("after buff: %s\n", buff);
    // printf("nonosec: %lu\n", elapse);
    return elapse;

}

/*=================================================================*
 *              main                            *
 *=================================================================*/
int main(int argc, char ** argv)
{
    int i, nloop, msgsize, prio;
    pid_t childpid;
    unsigned long timepass = 0; 

    mqd_t mqd_in, mqd_out;
    struct mq_attr attr;

    // struct timespec before = {0, 0};
    // struct timespec after = {0, 0};

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[SERVER]: need more privilege to change priority\n");

    if(argc != 3)
        bail("[Error] usage: latency_pxmsg {NUM_OF_LOOPS} {MESSAGE_SIZE}");
    nloop = atoi(argv[1]);
    msgsize = atoi(argv[2]);

    if(msgsize > 8192)
        bail("[Error] {MESSAGE_SIZE} cannot be larger than 8192");

    attr.mq_maxmsg = MAXMSG;
    attr.mq_msgsize = msgsize;

    mqd_in = mq_open("/client_server", CRW_FLAG, FILE_MODE, &attr);
    mqd_out = mq_open("/server_client", CRW_FLAG, FILE_MODE, &attr);

    if((childpid = fork()) == 0)
    {   /* child SERVER process */
        int i = 0;
        int status, prio;
        char buff[MSGSIZE];
        // struct timespec before = {0, 0};
        // struct timespec after = {0, 0};
        
        prio = setpriority(PRIO_PROCESS, 0, -20);
        if(prio != 0)
            printf("[SERVER]: need more privilege to change priority\n");

        while(1)
        {
            if(mq_receive(mqd_in, buff, msgsize, NULL) != msgsize)
                bail("[SERVER]: mq_receive error");

            // printf("received buff: %s\n", buff);

            // clock_gettime(CLOCK_MONOTONIC, &before);
            for(i = 0; i < msgsize; i++)
                buff[i] += 1;
            // clock_gettime(CLOCK_MONOTONIC, &after);
            // printf("[SERVER]: forloop nonosec: %lu\n", diff(before, after));

            status = mq_send(mqd_out, buff, msgsize, 0);
            if(status != OK)
                bail("[SERVER]: mq_send error");
            i++;
        }
        exit(0);

    }

    /* parent CLIENT process */
    send_message(mqd_in, mqd_out, msgsize, 0);

    for(i = 0; i < nloop; i++)
    {
        timepass +=send_message(mqd_in, mqd_out, msgsize, i);
    }
    printf("[Server]: %d loop average nsec %lu\n", nloop, timepass/nloop);
    kill(childpid, SIGTERM);
    mq_close(mqd_in);
    mq_close(mqd_out);
    mq_unlink("/client_server");
    mq_unlink("/server_client");
    exit(0);
}