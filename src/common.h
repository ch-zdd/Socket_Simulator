#ifndef _COMMON_H_
#define _COMMON_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>

#define MY_ERROR 	1
#define MY_OK		0

#define PRINT_COLOR_GREEEN "\033[1;40;32m"
#define PRINT_COLOR_CLEAR	"\033[0m"

#define SERIVE_IP "service_ip: "
#define SERIVE_PORT "service_port: "
#define CLIENT_IP "client_ip: "
#define CLIENT_PORT "client_port: "

#define my_assert(flag, str) if(flag); else {perror(str" error"); exit(0);}
#define my_debug(format, ...) printf(format"(%s:%d)\n", ##__VA_ARGS__, __func__, __LINE__);

#define CFG_CONTEXT_MAX_LEN 4096

typedef unsigned char uint8_t;

struct {
	char service_ip[16];
	in_port_t service_port;
	char client_ip[16];
	in_port_t client_port;
}cfg;

typedef struct 
{
#define PRO_TCP 1
#define PRO_UDP	0
	int pro; //0:not set ; 1: tcp ; 2: udp
	char path[128];
}send_task_t;
//FORMAT
//TCP: /home/name/A.TXT
//UDP: /home/name/B.TXT

extern send_task_t send_task[128];

extern char cfg_context[CFG_CONTEXT_MAX_LEN];

char* index2string(int index);
int parse_string(const char* data, const char*parameter, char* output_value, uint8_t read_length);
int parse_integer_u64(const char* data, const char*parameter, uint64_t* output_value);
char* set_cfg_path(int argc, char* argv[]);
void cfg_context_comment_remove(char* cfg_context, char* data_cfg);
void read_cfg(const char* cfg_path);
char* read_msg(const char* msg_path);
void read_task(char* buff);

#endif