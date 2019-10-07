#ifndef MUL_THREAD_S_H
#define MUL_THREAD_S_H

#include "server.h"
#include "server_func.h"
#include <pthread.h>
// #include <sys/event.h>


#define CLIENT_CNT 8

struct thread_mutex{
	pthread_mutex_t pmutex;
	pthread_cond_t pcond;
};

extern int clnt_cnt;
extern pthread_mutex_t mutex;

void* thread_init(void* sock);

#endif
