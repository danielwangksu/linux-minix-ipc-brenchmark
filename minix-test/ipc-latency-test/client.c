/**************************************************************
*   IPC benchmark Test -- Client Process                      *
*   by Daniel Wang                                            *
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include <epmisc.h>

#include <time.h>
#include <sys/time.h>
#define BILLION  1000000000L
struct timespec before;
struct timespec after;

struct timeval tv_start, tv_stop;

message m;
int server_ep;

int start_time(void);
double stop_time(void);
void tv_sub(struct timeval *out, struct timeval *in);

// fault handler
static void bail(const char *on_what){
    perror(on_what);
    exit(1); 
}

/*===========================================================================*
 *              diff                            *
 *===========================================================================*/
struct timespec diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec -1;
        temp.tv_nsec = BILLION + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
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
    tv_start = (struct timeval){ 0 };
    tv_stop = (struct timeval){ 0 };
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
    printf("[TIMER] start: sec=%d usec=%d stop: sec=%d usec=%d \n", tv_start.tv_sec, tv_start.tv_usec, tv_stop.tv_sec, tv_stop.tv_usec);
    tv_sub(&tv_stop, &tv_start);
    clockus = tv_stop.tv_sec * 1000000.0 + tv_stop.tv_usec;
    //if(clockus == 0.0)
    //    bail("[ERROR] stop_time error");
    return(clockus);
}

/*===========================================================================*
 *              initialize                      *
 *===========================================================================*/
void initialize(){
    memset(&m, 0, sizeof(m));
    server_ep = getendpoint_name("server");
    printf("[CLIENT]: server_ep=%d\n", server_ep);
}

/*===========================================================================*
 *              prepare                         *
 *===========================================================================*/
/* server <-> client uses m_m1 format */
void prepare(int data){
    memset(&m, 0, sizeof(m));
    m.m_type = 1;
    m.m_m1.m1i1 = data;
    printf("[CLIENT]: sending data m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
}

/*===========================================================================*
 *              main                           *
 *===========================================================================*/
int main(int argc, char ** argv){
    int r, nloop, status;
    int i = 0, j = 0;
    int data[] = {11, 22, 33, 44, 55, 66, 77, 88, 99, -1};

    //printf("[CLIENT]: number of args: %d\n", argc);
    //printf("[CLIENT]: argv[0]: %s\n", argv[0]);

    //if(argc != 2)
    //    bail("[CLIENT] usage: client {NUM_OF_LOOPS}");
    //nloop = atoi(argv[1]);

    nloop = 5;

    sleep(2);

    initialize();

    for(i = 0; i < nloop; i++)
    {
        sleep(2);
        // memset(&before, 0, sizeof(struct timespec));
        // memset(&after, 0, sizeof(struct timespec));

        prepare(data[i]);

        clock_gettime(CLOCK_MONOTONIC, &before); // get timestamp before
        //start_time();
        for(j = 0; j < 1000; j++)
        {
            ipc_send(server_ep, &m);
            r = ipc_receive(server_ep, &m, &status);
        }
        //printf("Cost: %f usec\n", stop_time());
        clock_gettime(CLOCK_MONOTONIC, &after); // get timestamp after

        printf("[CLIENT]: receive data m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
        printf("[CLIENT]: Start: sec=%lld nsec=%ld Stop: sec=%lld nsec=%d\n", before.tv_sec, before.tv_nsec, after.tv_sec, after.tv_nsec);
        printf("[CLIENT]: sec: %lld, nonsec: %ld\n", diff(before, after).tv_sec, diff(before, after).tv_nsec);

        if(data[i] == -1)
        {
            printf("[CLIENT]: terminate\n");
            exit(0);
        }
    }
    return 0;
}