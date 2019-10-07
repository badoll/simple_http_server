#ifndef SERVER_H
#define SERVER_H
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
/*
	scoket();
	bind();
	recv();
	send();
	inet_aton();
	inet_ntoa();
	AF_INET
	SOCK_STREAM
*/

#include <arpa/inet.h>
/*
	htons();
	ntonl();
	ntohs();
	inet_aton();
	inet_ntoa();
*/

#include <netinet/in.h>
/*
	inet_aton();
	inet)ntoa();
*/

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>

//#define SERVER_PORT 8080
#define BUFFERLEN		  	 	1024
#define REQUESTLEN  			5120
#define RESPONSELEN  			5120
#define ATTR_LEN 				1024
#define BODY_LEN	 			1024
#define VERSION_LEN 			16
#define NUM_LEN					10
#define TYPE_LEN				50
#define REPO_STATUS_WORD_LEN	50

#define KEEP_ALIVE_TIME_OUT		20

#define LISTEN_QUE  			16
#define ATTR_NUM				20
#define TYPE_NUM				20
#define BINARY_TYPE_NUM			11
#define POST_FORMAT_NUM			2

#define ADDR_SIZE 				sizeof(struct sockaddr)

#define HTML_DIR				"./html"
#define HTML_404				"./html/404.html"
#define HTTP_VER				"HTTP/1.1"

enum request_id{
	GET,
	POST
};

typedef struct{
	int attr_num;
	char key[ATTR_NUM][TYPE_LEN];
	char value[ATTR_NUM][BUFFERLEN];
}connect_attr;

typedef struct{
	FILE* file;
	char  type[TYPE_LEN];
}file_info;

typedef struct{
	int id;
	char pathname[BUFFERLEN];
	char version[VERSION_LEN];
	char body[BODY_LEN];
	connect_attr attr;
}request;

typedef struct{
	int status;
	file_info file_info;
	char version[VERSION_LEN];
	char body[BODY_LEN];
	connect_attr attr;
}response;

#endif