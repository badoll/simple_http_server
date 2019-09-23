#include "server_func.h"
static char extension[TYPE_NUM][TYPE_LEN] = 
{
    "txt",
    "json",
    "js",
    "css",
    "html","htm","jsp","php",
    "xml",
    "tif","tiff",
    "gif",
    "jpe","jpeg","jpg",
    "png",
    "wbmp",
    "mp3","wav",
    "ico"
};
static char content_type[TYPE_NUM][TYPE_LEN] = 
{
    "text/plain",
    "application/json",
    "application/x-javascript",
    "text/css",
    "text/html","text/html","text/html","text/html; charset=utf-8",
    "application/xml",
    "image/tiff","image/tiff",
    "image/gif",
    "image/jpeg","image/apng","image/jpeg",
    "image/png",
    "image/vnd.wap.wbmp",
    "audio/mp3","audio/wav",
    "image/x-icon"
};

static char binary_type[BINARY_TYPE_NUM][TYPE_LEN] =
{
    "tif","tiff",
    "gif",
    "jpe","jpeg","jpg",
    "png",
    "wbmp",
    "mp3","wav",
    "ico"
};

static char post_format[POST_FORMAT_NUM][TYPE_LEN] = 
{
    "application/x-www-form-urlencoded",
    "multipart/form-data"
};

void wait_process();

void
bind_addr(int* sock, struct sockaddr_in* addr, char* port)
/* 
 * purpose: bind server address
 * details: if bind error, exit in -1 status
 */
{
    (*addr).sin_family = AF_INET;
    (*addr).sin_port = htons(atoi(port));
    (*addr).sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(*sock,(const struct sockaddr*)addr,
        (socklen_t)ADDR_SIZE) == -1) {
        handle_error(-1,"bind() error");
    }
    return;
}

void
setsocket(int* sock)
/*
 * purpose: set socket address reuse attribute
 * details: if setsockopt() error, exit in -1 status
 */
{
    int option = 1;
    socklen_t opt_len = sizeof(option);
    if (setsockopt(*sock,SOL_SOCKET,SO_REUSEADDR,
        (const void*)&option,opt_len) == -1) {
        handle_error(-1,"setsockopt() error");
    }
    return;
}

void
set_childp_signal(struct sigaction* sig)
/* 
 * purpose: set signal to deal finished child process, avoid zombie process
 * details: invoke wait_process() to deal SIGCHLD signal
 */
{
    (*sig).sa_flags = 0;
    // (*sig).sa_flags = SA_RESTART;
    sigemptyset(&(*sig).sa_mask);
    (*sig).sa_handler = &wait_process;
    sigaction(SIGCHLD,sig,0);
    return;
}

void
wait_process()
/*
 * purpose: handle finished child process
 * details: warns when waitpid() error
 */
{
    int status;
    pid_t pid = waitpid(-1,&status,WNOHANG);
    if (pid == -1)
    {
        perror("waitpid() error");
    }
    if (WIFEXITED(status)) {
        printf("child process(pid:%d) ends with status:%d\n",pid,
             WEXITSTATUS(status));
    } else {
        // handle_error(0,"waitpid() error");
        perror("waitpid() error");
    }
    return;
}

int
handle_connection(int cnet_sock)
/*
 * purpose: receive request packet
 * details: loop to receive message, unless client shutdown output stream or error.
 * if remain incomplete request message, merge to the next receive message
 * if timeout, break loop
 * After all connecting, shutdown output stream to send eof
 * returns: 1 if ok, 0 if client shutdown output stream or timeout,
 * if recv() error, exit in -1 status
 */
{
    time_t keep_alive_t = time(NULL);
    int recvlen, total_len, request_len;
    int hasremain = 0;
    char buf[REQUESTLEN];
    char remain_msg[REQUESTLEN*2];
    char *remain = NULL;
    char *total_msg = NULL;
    while ((recvlen = recv(cnet_sock,(void*)buf,REQUESTLEN,0)) > 0) {
        buf[recvlen] = '\0';
        // printf("receive message:%s\n",buf);
        if (hasremain)
        {
            total_len = strlen(buf) + strlen(remain);
            total_msg = (char*) realloc (total_msg,sizeof(char)*(total_len+1));
            strcpy(total_msg,remain);
            strcat(total_msg,buf);
            hasremain = 0;
        } else
        {
            total_len = strlen(buf);
            total_msg = (char*) realloc (total_msg,sizeof(char)*(total_len+1));
            strcpy(total_msg,buf);
        }
        strcpy(remain_msg,total_msg);
        remain = remain_msg;
        /* loop to parse each request in the message received */
        while((remain - remain_msg) < total_len)
        {
            request requ;
            bzero(&requ,sizeof(requ));
            if (parse_request(remain,&requ,&request_len) == 0)
            {
                hasremain = 1;
                break;
            }
            remain += request_len;
            printf("REQUEST:%s\n",remain-request_len);
            deal_method(cnet_sock,&requ); /* deal different method */
        }
        if (hasremain)
        {
            continue;
        }
        if (istimeout(keep_alive_t)) {
            break;
        }
    } 
    free(total_msg);
    if (recvlen == 0) {
        handle_error(0,"client shutdown output stream");
        return 0;
    } else if (recvlen == -1) {
        handle_error(-1,"recv() error");
    } else {
        handle_error(0,"time out");
        return 0;
    }
    return 1;
}

void
deal_method(int cnet_sock, request* requ)
/* 
 * purpose: handle each method
 * details: define a response variable, and set its attribute in the function
 * that invoke
 */
{
    response repo;
    bzero(&repo,sizeof(repo));
    strcpy(repo.version,HTTP_VER);
    if ((*requ).id == GET) {
        method_get(cnet_sock,requ,&repo);
    } else if ((*requ).id == POST) {
        method_post(cnet_sock,requ,&repo);
    }
    if (repo.file_info.file) {
        fclose(repo.file_info.file);
    }
    return;
}

int
parse_request(char* message, request* requ, int* request_len)
/*
 * purpose: parse request and restore in a request struct
 * returns: 0 if request is not complete, else return 1
 */
{
    int body_len, remain_len;
    char *method, *pathname, *version, *requ_body, *attr_msg;
    char *message_cpy = (char*) malloc (sizeof(char)*(strlen(message)+1));
    strcpy(message_cpy,message);

    /* split request body */
    requ_body = strstr(message_cpy,"\r\n\r\n");
    if (requ_body == NULL)
    {
        return 0;
    }
    *request_len = requ_body - message_cpy + 4; /* get the position of body */
    remain_len = strlen(message_cpy) - *request_len; /* get the remain body message */
    *requ_body = '\0';  /* 'delete' body and request behind from message_cpy */
    requ_body += 4;

    /* split request attribute */
    attr_msg = strstr(message_cpy,"\r\n");
    parse_attr(attr_msg, &(*requ).attr);
    body_len = get_bodylen(&(*requ).attr);
    
    if (remain_len < body_len)
    {
        return 0;
    }
    *request_len += body_len;
    strncpy((*requ).body,requ_body,body_len);
    
    /* split request first line */
    char *header_ptr, *each_ptr;
    char *header = strtok_r(message_cpy,"\r\n",&header_ptr);
    method  = strtok_r(header," ",&each_ptr);
    pathname = strtok_r(NULL," ",&each_ptr);
    version = strtok_r(NULL," ",&each_ptr);

    if (strcmp(method,"GET") == 0) {
        (*requ).id = GET;
    } else if (strcmp(method,"POST") == 0) {
        (*requ).id = POST;
    }
    strcpy((*requ).pathname,pathname);
    strcpy((*requ).version,version);
    free(message_cpy);
    return 1;
}

int
get_bodylen(connect_attr* attr)
/*
 * purpose: get the length of request body
 * returns: 0 if request doesn't have body content
 */
{
    for (int i = 0; i < (*attr).attr_num; ++i) {
        if (strcmp((*attr).key[i],"Content-Length") == 0) {
            return atoi((*attr).value[i]);
        }
    }
    return 0;
}

void
parse_attr(char* attr_msg, connect_attr* attr)
/*
 * purpose: parse request attribute and restore in a connect_attr struct
 * details: "\r\n" as splitting delimiter
 */
{
    char *message_cpy = (char*) malloc (sizeof(char)*(strlen(attr_msg)+1));
    strcpy(message_cpy,attr_msg);
    char *rowptr, *colptr;
    char *buf, *key, *value;
    while((buf = strtok_r(message_cpy,"\r\n",&rowptr)) != NULL) {
        key = strtok_r(buf,": ",&colptr);
        value = strtok_r(NULL,": ",&colptr);
        add_attr(key,value,attr);
        message_cpy = NULL;
    }
    free(message_cpy);
}

void
add_attr(char* key, char* value, connect_attr* attr)
/*
 * purpose: add attribute to connect_attr struct
 */
{
    strcpy((*attr).key[(*attr).attr_num],key);
    strcpy((*attr).value[(*attr).attr_num++],value);
}

void
method_get(int cnet_sock, request* requ, response* repo)
/*
 * purpose: handle GET method
 * details: set response and send it to client
 */
{
    set_urlparam(requ);
    set_response(requ,repo);
    send_repo(cnet_sock,repo);
    return;
}

void
method_post(int cnet_sock, request* requ, response* repo)
{
    set_response(requ,repo);
    send_repo(cnet_sock,repo);
    /* TODO: FASTCGI script parser */
}

void
set_response(request* requ, response* repo)
/*
 * purpose: get header, attribute and file that response needs
 */
{
    char *path = get_path((*requ).pathname);
    set_file(path,repo);
    set_repoattr((*repo).file_info.file,path,&(*requ).attr,repo);
    free(path);
}

void
set_urlparam(request* requ)
/*
 * purpose: get parameters from GET request
 */
{
    char *body;
    if ((body = strstr((*requ).pathname,"?")) != NULL)
    {
        *body = '\0';
        strcpy((*requ).body,body+1);
    }
    return;
}

void
set_cnetstatus(response* repo, int status)
/*
 * purpose: set response status
 */
{
    (*repo).status = status;
}

void
set_repoattr(FILE* file,char* path,connect_attr* requattr,response* repo)
/*
 * purpose: according to request attribute and file attribute, set response attribute
 */
{
    connect_attr* repoattr = &(*repo).attr;
    /* Content-Type */
    strcpy((*repoattr).key[(*repoattr).attr_num],"Content-Type");
    strcpy((*repoattr).value[(*repoattr).attr_num++],(*repo).file_info.type);
    /* Content-Length */
    int file_size;
    if (fseek(file,0,SEEK_END) == -1) {
        handle_error(-1,"fseek() error");
    }
    if ((file_size = ftell(file)) == -1) {
        handle_error(-1,"ftell() error");
    }
    rewind(file);/* rewind after fseek */
    char size[NUM_LEN];
    sprintf(size,"%d",file_size);
    strcpy((*repoattr).key[(*repoattr).attr_num],"Content-Length");
    strcpy((*repoattr).value[(*repoattr).attr_num++],size);
    /* Keep-Alive */
    for (int i = 0; i < (*requattr).attr_num; i++) {
        if (strcmp((*requattr).key[i],"Connection") == 0) {
            if (strcmp((*requattr).value[i],"keep-alive") == 0) {
                strcpy((*repoattr).key[(*repoattr).attr_num],"Connection");
                strcpy((*repoattr).value[(*repoattr).attr_num++],"keep-alive");
                strcpy((*repoattr).key[(*repoattr).attr_num],"Keep-Alive");
                strcpy((*repoattr).value[(*repoattr).attr_num],"timeout=");
                char keep_alive_t[NUM_LEN];
                sprintf(keep_alive_t,"%d",KEEP_ALIVE_TIME_OUT);
                strcat((*repoattr).value[(*repoattr).attr_num++],keep_alive_t);
            }
        }
    }
    return;
}

void
set_filetype(char* path,char* filetype)
/*
 * purpose: set file type from filename's postfix by extension array
 * details: if postfix is not found in extension array, set unknown
 */
{
    int path_len = strlen(path);
    char *token = NULL;
    char buf[BUFFERLEN];
    strcpy(buf,path);
    for (int i = path_len - 1; i >= 0; i--)
    {
        if (buf[i] == '.')
        {
            token = buf + i + 1;
            break;
        }
    }
    if (token == NULL)
    {
        /* no postfix extension */
        strcpy(filetype,"text/plain");
        return;
    }
    printf("token:%s\n",token);
    for (int i = 0; i < TYPE_NUM; ++i)
    {
        if (strcmp(token,extension[i]) == 0)
        {
            strcpy(filetype,content_type[i]);
            return;
        }
    }
    strcpy(filetype,"unknown");
    return;
}

int
send_attr(int cnet_sock, connect_attr* attr)
/*
 * purpose: send response attribute to client
 * returns: 1 if ok, -1 if send() error
 */
{
    char* attr_msg;
    int msg_len = 0;
    int k_len, v_len;
    for (int i = 0; i < (*attr).attr_num; ++i) {
        k_len = strlen((*attr).key[i]);
        v_len = strlen((*attr).value[i]);
        msg_len += k_len + v_len + 4; /* add ":"," ","\r","\n" */
    }
    msg_len += 2; /* blank line "\r\n" */
    attr_msg = (char*) malloc (sizeof(char)*(msg_len+1));
    bzero(attr_msg,msg_len+1);/* free() will not reset the data in the memory that malloc, need to reset actively */
    /* not using (realloc) to malloc everytime to avoid memory splitting in too many pieces */
    for (int i = 0; i < (*attr).attr_num; ++i) {
        strcat(attr_msg,(*attr).key[i]);
        strcat(attr_msg,": ");
        strcat(attr_msg,(*attr).value[i]);
        strcat(attr_msg,"\r\n");
    }
    strcat(attr_msg,"\r\n");
    if (send_data(cnet_sock,attr_msg,strlen(attr_msg)) == -1) {
        perror("send(attr_msg) error");
        return -1;
    }
    printf("response attr:%s", attr_msg);
    free(attr_msg);
    return 1;
}


char*
get_path(char* pathname)
/*
 * purpose: add prefix path to pathname
 * returns: whole path including prefix directory name
 * details: if pathname if '/', return homepage path
 */
{
    int len = strlen(pathname);
    char* path = (char*) malloc (sizeof(char)*(BUFFERLEN+len));
    strcpy(path,HTML_DIR); /* html directory */
    if (strcmp(pathname,"/") == 0) {
        strcat(path,"/index.html");
    } else {
        strcat(path,pathname);
    }
    return path;
}

void
set_file(char* path,response* repo)
/* 
 * purpose: get file that response needs
 * details: set file type, and set response status if open file error
 */
{
    printf("path:%s\n",path);
    set_filetype(path,(*repo).file_info.type);
    char *type = (*repo).file_info.type;
    char mode[TYPE_LEN] = "r";
    for (int i = 0; i < BINARY_TYPE_NUM; ++i)
    {
        if (strcmp(type,binary_type[i]) == 0)
        {
            strcpy(mode,"rb");
            break;
        }
    }
    if (((*repo).file_info.file = fopen(path,mode)) == NULL) {
        if (errno == EEXIST || errno == EACCES) {
            set_cnetstatus(repo,404);
        } else {
            /* TODO: handle other error */
            set_cnetstatus(repo,404);
        }
        if (((*repo).file_info.file = fopen(HTML_404,"r")) == NULL) {
            handle_error(-1,"fopen() error");
        }
    } else {
        /* open file successfully */
        set_cnetstatus(repo,200);
    }
}

int
send_header(int cnet_sock,char* version, int status)
/*
 * purpose: send response header
 * returns: 1 if ok, -1 if send() error
 */
{
    char data[BUFFERLEN];
    strcpy(data,version);
    switch (status)
    {
    case 200:
        strcat(data," 200 OK\r\n");
        break;
    case 404:
        strcat(data," 404 NOT FOUND\r\n");
        break;
    default:
        break;
    }
    if (send_data(cnet_sock,data,strlen(data)) == -1) {
        perror("send(header) error");
        return -1;
    }
    printf("response header:%s", data);
    return 1;
}

int
send_file(int cnet_sock, file_info* file_info)
/*
 * purpose: send html file base on (send_data) function
 * returns: 1 if ok, -1 if send() error
 */
{
    char buf[BUFFERLEN];
    int readlen;
    FILE *file = (*file_info).file;
    // FILE* writefp = fdopen(cnet_sock,"w");
    while (!feof(file)) {
        readlen = fread(buf,1,BUFFERLEN-1,file);
        buf[readlen] = '\0';
        if (send_data(cnet_sock,buf,readlen) == -1) {
            perror("send(file) error");
            return -1;
        }
        // printf("data:%s\n",buf);
        // fputs(buf,writefp); /* standrad I/O interface */
    }
    // fflush(writefp);
    return 1;
}

int
send_repo(int cnet_sock,response* repo)
/*
 * purpose: send whole response
 * returns: 1 if ok, -1 if send message to client error
 */
{
    if (send_header(cnet_sock,(*repo).version,(*repo).status) == -1) {
        return -1;
    }
    if (send_attr(cnet_sock,&(*repo).attr) == -1) {
        return -1;
    }
    if (send_file(cnet_sock,&(*repo).file_info) == -1) {
        return -1;
    }
    return 1;
}

int
send_data(int cnet_sock, char* data, int size)
/*
 * purpose: send data to client
 * details: basic send function, all message is sent by this function
 * returns: 1 if ok, -1 if send() error
 */
{
    int sendlen;
    if ((sendlen = send(cnet_sock,data,size,0)) == -1)
    {
        perror("send() error");
        return -1;
    }
    return 1;
}

int
recv_data(int cnet_sock, char* data)
/*
 * purpose: receive data from client
 * details: basic receive function, all message is received by this function
 * returns: 1 if ok, -1 if recv() error
 */
{
    int recvlen;
    if ((recvlen = recv(cnet_sock,data,BUFFERLEN-1,0)) == -1) {
        perror("recv() error");
        return -1;
    }
    data[recvlen] = '\0';
    if (recvlen < BUFFERLEN-1) {
        return 0;
    }
    return 1; /* receive completely */
}

void
handle_error(int retr, char* msg)
/* 
 * purpose: handle error 
 * details: if retr == -1, exit process, and report the error
 * if retr == 0, report the error and skip to continue
 */
{
    if (retr == -1) { 
        perror(msg);
        exit(-1);
    }
    if (retr == 0) {
        fprintf(stderr,"%s.\n",msg);
    }
    return;
}

int
istimeout(time_t keep_alive_t)
/* 
 * purpose: check if timeout
 * returns: 1 if timeout, else return 0
 */
{
    time_t now = time(NULL);
    if (keep_alive_t - now > KEEP_ALIVE_TIME_OUT) {
        return 1;
    }
    return 0;
}