#ifndef SERVER_FUNC_H
#define SERVER_FUNC_H
#include "server.h"

void bind_addr(int* sock, struct sockaddr_in* sockaddr, char* port);
void setsocket(int* sock);
void set_childp_signal(struct sigaction* sig);
int parse_request(char* message, request* requ, int* request_len);
void parse_attr(char* attr_msg, connect_attr* attr);
void deal_method(int cnet_sock, request* requ);

void method_get(int cnet_sock,request* requ,response* repo);
void method_post(int cnet_sock, request* requ, response* repo);

void add_attr(char* key, char* value, connect_attr* attr);

void set_cnetstatus(response* repo, int status);
void set_repoattr(FILE* file,char* path,connect_attr* requattr,response* repo);
void set_response(request* requ,response* repo);
void set_urlparam(request* requ);
void set_file(char* path,response* repo);
void set_filetype(char* path,char* filetype);

char* get_path(char* pathname);
int get_bodylen(connect_attr* attr);

int send_repo(int cnet_sock, response* repo);
int send_header(int cnet_sock, char* version, int status);
int send_attr(int cnet_sock, connect_attr* attr);
int send_file(int cnet_sock, file_info* file_info);

int send_data(int cnet_sock, char* data, int size);
int recv_data(int cnet_sock, char* data);

int handle_connection(int cnet_sock);
void handle_error(int retr, char* msg);

int istimeout(time_t keep_alive_t);

#endif
