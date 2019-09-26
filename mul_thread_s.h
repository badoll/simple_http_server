#ifndef MUL_THREAD_S_H
#define MUL_THREAD_S_H

#include "server.h"
#include "server_func.h"
#include <pthread.h>

#define CLIENT_CNT 8

extern int clnt_cnt;
extern pthread_mutex_t mutex;

void* thread_init(void* sock);

#endif