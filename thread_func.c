#include "mul_thread_s.h"

void*
thread_init(void* sock)
{
    pthread_mutex_lock(&mutex);
    int clnt_id = clnt_cnt++;
    pthread_mutex_unlock(&mutex);
    int cnet_sock = *(int*)(sock);
    printf("client %d is connecting.\n",clnt_id);
    handle_connection(cnet_sock);
    shutdown(cnet_sock,SHUT_WR); /* shutdown output stream, send eof to client */
    close(cnet_sock);
    printf("client %d disconnected...\n",clnt_id);
    return NULL;
}