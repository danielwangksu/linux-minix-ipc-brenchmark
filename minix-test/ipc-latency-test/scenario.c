/**************************************************************
*   IPC benchmark Test -- Loading Process                     *
*   by Daniel Wang                                            *
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <lib.h>
#include <sys/un.h>
#include <strings.h>
#include <signal.h>
#include <epmisc.h>

#define MAX_ARGC  18
#define MAX_ARGS_LENGTH  10
#define INITIAL_ACID 100
int server_pid, client_pid;

// fault handler
static void bail(const char *on_what){
    perror(on_what);
    exit(1); 
}

// main function
int main(int argc, char ** argv){
    int nloop;
    int server_acid, client_acid, outproxy_acid, inproxy_acid;
    char *argv_control[] = {"holder", 0};

    if(argc != 2)
        bail("[PARENT] usage: scenario {NUM_OF_LOOPS}");
    nloop = atoi(argv[1]);

    server_acid = INITIAL_ACID;
    client_acid = INITIAL_ACID + 1;

    if((server_pid = fork2(server_acid)) == 0){
        // server process
        if(execv("server", argv_control) == -1){
            printf("Return not expected.\n");
            bail("execv(server)");
        }
        exit(0);
    } else if((client_pid = fork2(client_acid)) == 0) {
        // client proxy process
        //char *argv_client[] = {"client", "", 0};
        //sprintf(argv_client[1], "%d", nloop);
        if(execv("client", argv_control) == -1){
            printf("Return not expected.\n");
            bail("execv(client)");
        }
        exit(0);
    }
    else{
        // parent process
        int server_ep, client_ep;
        int child_status;
        int t_pid;

        server_ep = getendpoint(server_pid);
        client_ep = getendpoint(client_pid);
        printf("[PARENT]: server_pid=%d, client_pid=%d, server_ep=%d, client_ep=%d\n", server_pid, client_pid, server_ep, client_ep);

        t_pid = wait(&child_status);
        kill(server_pid, SIGKILL);
        kill(client_pid, SIGKILL);

        exit(0);
    }
    return 0;
}