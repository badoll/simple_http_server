/*
 * multi-thread server
 */
#include "mul_thread_s.h"

int clnt_cnt = 1;
pthread_mutex_t mutex;
int
main(int ac, char** av)
{
    if (ac != 2) {
        printf("Usage: bin/server port\n");
        exit(0);
    }
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
    pthread_t t_id;
    pthread_mutex_init(&mutex,NULL);
    while (1) {
        if ((cnet_sock = accept(ctrl_sock,(struct sockaddr*)&sevr_addr,
            (socklen_t*)&addrlen)) == -1) {
            if (errno == EINTR) {
            /* handle SIGCHLD signal interrupt accept()*/
                continue;
            }
            handle_error(-1,"accept() error");
        }
        pthread_create(&t_id,NULL,(void*)thread_init,(void*)&cnet_sock);
        pthread_detach(t_id);
    }
    close(ctrl_sock);
}