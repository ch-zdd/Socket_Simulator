#include "common.h"

pthread_mutex_t mutex_print_flag = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t has_print = PTHREAD_COND_INITIALIZER;
static int print_flag = 1;

void my_exit()
{
	printf("Program exited\n");
}

void init(void)
{
	memset(&cfg, 0, sizeof(cfg));
	memset(&cfg_context, 0, sizeof(cfg_context));
	memset(&send_task, 0, sizeof(send_task));
}

void print_hello_text_img(void)
{
	//system("figlet \"Socket Client Tool\"");
	printf("%s\n", read_msg("/home/fm5gc/code_zhao_test/net/start_string.txt"));
}

void print_wait_user_info(void)
{
	ssize_t len = 0;
	int attr = 1;
	char buf[5] = "";
	ioctl(STDIN_FILENO, FIONBIO, &attr);
	while(1){
		len = read(STDIN_FILENO, buf, 5);
		if(len > 0 ){
			if(strstr(buf, "exit") != NULL){
				break;
			}else{
				printf(PRINT_COLOR_GREEEN"Enter \"exit\" to close connection:"PRINT_COLOR_CLEAR);
				fflush(stdout);
				memset(buf, 0, 5);
				while(read(STDIN_FILENO, buf, 5) >0); //清空输入
				continue;
			}
		}else{
			usleep(100*1000);
		}

		pthread_mutex_lock(&mutex_print_flag);
		//while(print_flag == 0){
			//pthread_cond_wait(&has_print, &mutex_print_flag);
		//}
		if(print_flag == 1){
			printf(PRINT_COLOR_GREEEN"Enter \"exit\" to close connection:"PRINT_COLOR_CLEAR);
			fflush(stdout);
			print_flag = 0;
		}
		pthread_mutex_unlock(&mutex_print_flag);
	}	
}

void create_connection(int connect_fd)
{
	struct sockaddr_in service_addr, client_addr;
	int fd, ret = 0;
	int opt = 1;

	memset(&service_addr, 0, sizeof(service_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	fd = connect_fd;

	if(inet_addr(cfg.service_ip) != 0 && cfg.service_port != 0){
		service_addr.sin_family = AF_INET; //设置tcp协议族
		service_addr.sin_port = htons(cfg.service_port); //设置端口号
		service_addr.sin_addr.s_addr = inet_addr(cfg.service_ip); //设置ip地址
	}else{
		printf("service ip or/and serive port is incorrect \n");
		exit(0);
	}

	if(inet_addr(cfg.client_ip) != 0 && cfg.client_port != 0){
		client_addr.sin_family = AF_INET; //设置tcp协议族
		client_addr.sin_port = htons(cfg.client_port); //设置端口号
		client_addr.sin_addr.s_addr = inet_addr(cfg.client_ip); //设置ip地址

		setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
		my_assert(ret != -1, "setsockopt");

		ret = bind(fd, (struct sockaddr*)(&client_addr), sizeof(client_addr));
		my_assert(ret == 0, "bind");
	}else{
		printf("no client ip or port, use random port numbers\n");
	}

	printf("Waiting for server response...\n");

    ret = connect(fd, (struct sockaddr*)&service_addr, sizeof(service_addr));
    my_assert(ret != -1, "connect service");

}

void connect_service(int* tcp_fd, int* udp_fd)
{
	int tcp,udp;
	tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	my_assert(tcp != -1, "create tcp socket client");

	udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	my_assert(udp != -1, "create udp socket client");

	create_connection(tcp);
	printf("create tcp socket success...\n");

	create_connection(udp);
	printf("create udp socket success...\n");

	*tcp_fd = tcp;
	*udp_fd = udp;
}

void* send_msg(void* socket_fd)
{
	int ret = 0;
	int index = 0;
	int udp_flag = 0;
	char* msg = NULL;

	int tcp_connection = ((int*)socket_fd)[0];
	int udp_connection = ((int*)socket_fd)[1];


	for(index = 0; send_task[index].pro != 0 ; index++){
		printf("read msg path: %s\n", send_task[index].path);
		msg = read_msg(send_task[index].path);
		if(msg == NULL){
			printf("msg is NULL, go to next\n");
			continue;
		}
		if(send_task[index].pro == 1){
			ret = send(tcp_connection, msg, strlen(msg), MSG_NOSIGNAL);
		}else if(send_task[index].pro == 2){
			ret = send(udp_connection, msg, strlen(msg), MSG_NOSIGNAL);
		}else{
			printf("Unknow protocol\n");
			continue;
		}
		if(ret == -1){
			perror("send error");
			return NULL;
		}

		printf(PRINT_COLOR_GREEEN"%ssend len = %d, send data:\n"PRINT_COLOR_CLEAR, send_task[index].pro == 1 ? "TCP: " : "UDP: ", ret);
		printf("*****************************************************\n");
		printf("%s\n", msg);
		printf("*****************************************************\n");
		printf("\n");
		sleep(1);
	}
	printf("\n");

	print_wait_user_info();
}

void* recv_udp(void* udp_connection)
{
	int ret = 0;
	char msg[4096] = "";

	for(;;){
		ret = recv(*((int*)udp_connection), msg, sizeof(msg), 0);
		if(ret == 0){
			printf("udp recv end\n");
			break;
		}
		my_assert(ret!=-1, "recv udp");

		printf("\n"
		PRINT_COLOR_GREEEN
		"UDP: recv len = %d, recv data:\n\n"
		PRINT_COLOR_CLEAR
		"*****************************************************\n"
		"%s\n"
		"*****************************************************\n",
		ret, msg);

		memset(msg, 0, sizeof(msg));

		pthread_mutex_lock(&mutex_print_flag);
		print_flag = 1;
		//pthread_cond_signal(&has_print);
		pthread_mutex_unlock(&mutex_print_flag);
	}
}

void* recv_tcp(void* tcp_connection)
{
	int ret = 0;
	char msg[4096] = "";

	for(;;){
		ret = recv(*((int*)tcp_connection), msg, sizeof(msg), 0);
		if(ret == 0){
			printf("tcp recv end\n");
			printf("close tcp\n");
			break;
		}
		my_assert(ret!=-1, "recv tcp");

		printf(
		"\n\033[1;40;32m"
		"TCP: recv len = %d, recv data:\n\n"
		"\033[0m"
		"*****************************************************\n"
		"%s\n"
		"*****************************************************\n",
		ret, msg);
		
		memset(msg, 0, sizeof(msg));

		pthread_mutex_lock(&mutex_print_flag);
		print_flag = 1;
		//pthread_cond_signal(&has_print);
		pthread_mutex_unlock(&mutex_print_flag);
	} 
}

void communication(int tcp_connection, int udp_connection)
{
	int socket_fd[2] = {tcp_connection, udp_connection};
	pthread_t pthread_send, pthread_recv_tcp, pthread_recv_udp;

	pthread_create(&pthread_send, NULL, send_msg, &socket_fd);
	pthread_create(&pthread_recv_tcp, NULL, recv_tcp, &tcp_connection);
	pthread_create(&pthread_recv_udp, NULL, recv_udp, &udp_connection);

	pthread_join(pthread_send, NULL);
	printf("pthread_send exit\n");

	pthread_cancel(pthread_recv_tcp);
	pthread_join(pthread_recv_tcp, NULL);
	printf("pthread_recv_tcp exit\n");

	pthread_cancel(pthread_recv_udp);
	pthread_join(pthread_recv_udp, NULL);
	printf("pthread_recv_udp exit\n");
}

int main(int argc, char* argv[]){

	int ret = 0;
	int tcp_connection = -1, udp_connection;
	char* cfg_path = NULL;

	atexit(my_exit);
	init();
	
	print_hello_text_img();

	cfg_path = set_cfg_path(argc, argv);

	read_cfg(cfg_path);
	read_task(cfg_context);

	connect_service(&tcp_connection, &udp_connection);

	communication(tcp_connection, udp_connection);

	close(tcp_connection);
	close(udp_connection);

	return 0;
}