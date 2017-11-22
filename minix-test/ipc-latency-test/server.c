/**************************************************************
*   IPC benchmark Test -- Server Process                      *
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

message m;
int client_ep;

// fault handler
static void bail(const char *on_what){
    perror(on_what);
    exit(1); 
}

/*===========================================================================*
 *              initialize                     *
 *===========================================================================*/
void initialize(){
    memset(&m, 0, sizeof(m));
    client_ep = getendpoint_name("client");
    printf("[SERVER]: client_ep=%d\n", client_ep);
}

/*===========================================================================*
 *              response                       *
 *===========================================================================*/
int response(int data)
{
    memset(&m, 0, sizeof(m));
    m.m_type = 1;
    m.m_m1.m1i1 = data + 1;
    return ipc_send(client_ep, &m);
}

/*===========================================================================*
 *              main                           *
 *===========================================================================*/
int main(int argc, char ** argv){
    int r, status;
    int i = 0;
    int data = 0;
    
    sleep(2);
    initialize();

    sleep(2);

    while(1){
        r = ipc_receive(client_ep, &m, &status);
        //printf("[SERVER]: receive data m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
        data = m.m_m1.m1i1;
        //printf("[SERVER]: do work...\n");

        r = response(data);

        if(data == -1)
            exit(0);
    }
    return 0;
}