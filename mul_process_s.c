/*
 * multiprocess server
 */
#include "mul_process_s.h"
#include "server_func.h"
int
main(int ac, char** av)
{
    if (ac != 2) {
        printf("Usage: bin/server port\n");
        exit(0);
    }
    int client_id = 1;
    int ctrl_sock = socket(PF_INET,SOCK_STREAM,0);
    if (ctrl_sock < 0) {
        handle_error(ctrl_sock,"socket() error");
    }
    setsocket(&ctrl_sock);/* set socket attribute: REUSEADDR */
    struct sockaddr_in sevr_addr;    
    bzero(&sevr_addr,sizeof(ADDR_SIZE));
    bind_addr(&ctrl_sock,&sevr_addr,av[1]);/* bind server address in sevr_addr */
    if (listen(ctrl_sock,LISTEN_QUE) == -1) {
        handle_error(-1,"listen() error");
    }
    socklen_t addrlen = ADDR_SIZE;
    int cnet_sock;
    pid_t child_p;
    struct sigaction sig;
    set_childp_signal(&sig);/* avoid zombie process */
    while (1) {
        if ((cnet_sock = accept(ctrl_sock,(struct sockaddr*)&sevr_addr,
            (socklen_t*)&addrlen)) == -1) {
            if (errno == EINTR) {
            /* handle SIGCHLD signal interrupt accept()*/
                continue;
            }
            handle_error(-1,"accept() error");
        }
        printf("client %d is connecting.\n",client_id);
        child_p = fork();
        if (child_p == -1) {
            close(cnet_sock);
            handle_error(0,"fork() error");
            continue;
        }
        if (child_p == 0) {
            close(ctrl_sock);
            /* close ctrl_sock in the child process */
            handle_connection(cnet_sock);
            shutdown(cnet_sock,SHUT_WR); /* shutdown output stream, send eof to client */
            close(cnet_sock);
            printf("client %d disconnected...\n",client_id);
            exit(0);
        } else {
            close(cnet_sock);
            client_id++;
        }
    }
    close(ctrl_sock);
}