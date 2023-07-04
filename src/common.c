#include "common.h"

char cfg_context[CFG_CONTEXT_MAX_LEN] = "";
send_task_t send_task[128];

char* index2string(int index)
{
	static char str[10] = "";
	memset(str, 0, sizeof(str));
	snprintf(str, 10, "[%d]", index);
	return str;
}

int parse_string(const char* data, const char*parameter, char* output_value, uint8_t read_length)
{
    char* pstr = NULL;
    char tmp[1024] = "";

    if((pstr = strstr(data, parameter) )== NULL){
        printf("Failed to search [%s]\n", parameter);
        return MY_ERROR;
    }
    if(sscanf(pstr+strlen(parameter), "%s", tmp) != 1){
        printf("Failed to parse [%s]\n", parameter);
        return MY_ERROR; 
    }
    if(strlen(tmp) > read_length){
        printf("The parsed string exceeds the length range\n");
    }else{
        strcpy(output_value, tmp);
    }

    printf("PARSE STRING: [%s] = %s\n", parameter, output_value);
    return MY_OK; 
}

int parse_integer_u64(const char* data, const char*parameter, uint64_t* output_value)
{
    char* pstr = NULL;
    uint64_t tmp;

    if((pstr = strstr(data, parameter) )== NULL){
        printf("Failed to search [%s]\n", parameter);
        return MY_ERROR;
    }

    if(sscanf(pstr+strlen(parameter), "%lu", &tmp) != 1){
        printf("Failed to parse [%s]\n", parameter);
        return MY_ERROR;
    }
    *output_value = tmp;
    printf("PARSE INTEGER: [%s] = %lu\n", parameter, *output_value);  

    return MY_OK;
}

char* set_cfg_path(int argc, char* argv[])
{
	static char cfg_path[255] = "";
	memset(cfg_path, 0, 255);
	char* p = NULL;

	if(argc == 1){
		sprintf(cfg_path, "%s", "net.cfg");
	}else if(argc == 2){
		sprintf(cfg_path, "%s", argv[1]);
	}else{
		printf("format: exe [CFG file path]");
		exit(0);
	}
	printf("Number of parameter :%d, cfg path is %s\n", argc, argv[1]);
	printf("\n");
	return cfg_path;
}

void cfg_context_comment_remove(char* cfg_context, char* data_cfg)
{
	char* p = NULL;
	char* p_copy_start = NULL;
	char* p_string_end = NULL;
	char* p_cfg_context = NULL;
	size_t copy_len = 0;
 
	p_string_end = data_cfg + strlen(data_cfg);
	p_copy_start = data_cfg;
	p_cfg_context = cfg_context;
	p = data_cfg;
	while(1){
		p = strchr(p_copy_start, '#');
		if(p == NULL){
			p = p_string_end;
		}

		copy_len = p - p_copy_start;
		memcpy(p_cfg_context, p_copy_start, copy_len);
		p_cfg_context += copy_len;
		p_copy_start = strchr(p, '\n');
		if(p_copy_start == NULL){
			break;
		}

		if(p == p_string_end) {
			break;
		}
		if(p > p_string_end) {
			printf("cfg_context_comment_remove error, %ld\n", p - p_string_end);
			break;
		}
	}
	printf("after remove comment, cfg context: \n");
	printf("***********************************\n");
	printf("%s\n", cfg_context);
	printf("***********************************\n");
}

void read_cfg(const char* cfg_path)
{
	int file_len,ret;
	uint64_t read_tmp;
	char data_cfg[1024] = "";
	char* p = NULL;

	FILE* file_cfg = fopen(cfg_path, "r");
	my_assert(file_cfg != NULL, "open file_cfg");

	ret = fseek(file_cfg,0,SEEK_END);
	my_assert(ret == 0, "fseek file_cfg");

	file_len =ftell(file_cfg);
	my_assert(file_len >0 && file_len<1024, "ftell file_cfg");

	rewind(file_cfg);
	ret = fread(data_cfg, 1, file_len, file_cfg);
	my_assert(ret == file_len, "fread file_cfg");

	cfg_context_comment_remove(cfg_context, data_cfg);

	if(parse_string(cfg_context, SERIVE_IP, cfg.service_ip, 16-1)) exit(0);

	if(parse_integer_u64(cfg_context, SERIVE_PORT, &read_tmp)) exit(0);
	cfg.service_port = (in_port_t)read_tmp;

	if(parse_string(cfg_context, CLIENT_IP, cfg.client_ip, 16-1)){
		printf("client ip not found\n");
	}

	if(parse_integer_u64(cfg_context, CLIENT_PORT, &read_tmp)){
		printf("client port not found\n");
	}else{
		cfg.client_port = (in_port_t)read_tmp;
	}

	fclose(file_cfg);
	printf("\n");
}

void read_task(char* buff)
{
	char pro[128] = "";
	char path[128] = "";
	char* p = buff;
	int index = 1, ret;

	p = strstr(p, "TASK");
	for(index = 0; ; index++){
		p = strstr(p, "\n");
		
		if(p == NULL){
			printf("parse END, number of messages parsed: %d\n", index);
			break;
		}

		p = p + strspn(p, " \r\n");
		ret = sscanf(p, "%[^(: )]: %s", pro, path);

		if(ret != 2){
			printf("parse END, number of messages parsed: %d\n", index);
			return;
		}

		if(strcmp(pro, "TCP") == MY_OK){
			send_task[index].pro = 1;
		}else if(strcmp(pro, "UDP") == MY_OK){
			send_task[index].pro = 2;
		}else{
			send_task[index].pro = 0;
			printf("parse error, unknown procotol, return\n");
			return;
		}

		printf("index: %d, msg path: %s\n", index, path);
		memcpy(send_task[index].path, path, strlen(path));
		memset(pro, 0, sizeof(pro));
		memset(path, 0, sizeof(path));
	}
	printf("\n");
}

char* read_msg(const char* msg_path)
{
	int file_len,ret;
	uint64_t read_tmp;
	static char msg[1024*10] = "";
	memset(msg, 0, sizeof(msg));
	FILE* file_msg = fopen(msg_path, "r");
	if(file_msg == NULL){
		perror("open msg fail");
		return NULL;
	}
	ret = fseek(file_msg,0,SEEK_END);
	if(ret != 0){
		perror("fseek msg fail");
		return NULL;
	}
	file_len =ftell(file_msg);
	if(file_len <0 || file_len>1024*10){
		perror("ftell msg fail");
		return NULL;
	}
	rewind(file_msg);
	ret = fread(msg, 1, file_len, file_msg);
	if(ret != file_len){
		perror("fread msg fail");
		return NULL;
	}
	fclose(file_msg);
	printf("\n");
	return msg;
}
