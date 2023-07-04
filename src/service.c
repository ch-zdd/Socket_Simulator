#include "common.h"

void my_exit()
{
	printf("Program exited\n");
}

void init(void)
{
	memset(&cfg, 0, sizeof(cfg));
	memset(&cfg_context, 0, sizeof(cfg_context));
}

void print_hello_text_img(void)
{
	printf("%s\n", read_msg("/home/fm5gc/code_zhao_test/net/start_string.txt"));
}

void create_tcp_connection(int* connect_fd, int* sock_fd)
{
	struct sockaddr_in addr;
	struct sockaddr client_addr;
	socklen_t clinent_sock_len;
	int fd, client_fd;
	int ret;
	int opt = 1;

	if(inet_addr(cfg.service_ip) != 0 && cfg.service_port != 0){
		addr.sin_family = AF_INET; //设置tcp协议族
		addr.sin_port = htons(cfg.service_port); //设置端口号
		addr.sin_addr.s_addr = inet_addr(cfg.service_ip); //设置ip地址
	}

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	my_assert(fd != -1, "socket");

	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	ret = bind(fd, (struct sockaddr*)(&addr), sizeof(addr));
	my_assert(ret == 0, "bind");

	ret = listen(fd, 10);
	my_assert(ret == 0, "listen");

	printf("create tcp socket success...\n");
	printf("wait client...\n");
	client_fd = accept(fd, &client_addr, &clinent_sock_len);
	my_assert(client_fd != -1, "accept");

	printf("connect client: %s:%hu\n", inet_ntoa(((struct sockaddr_in*)&client_addr)->sin_addr), ((struct sockaddr_in*)&client_addr)->sin_port);

	*connect_fd = client_fd;
	*sock_fd = fd;
}

void create_udp_connection(int* connect_fd)
{
	struct sockaddr_in addr;
	int fd, client_fd;
	int ret;

	if(inet_addr(cfg.service_ip) != 0 && cfg.service_port != 0){
		addr.sin_family = AF_INET; //设置tcp协议族
		addr.sin_port = htons(cfg.service_port); //设置端口号
		addr.sin_addr.s_addr = inet_addr(cfg.service_ip); //设置ip地址

	}else{
		printf("No service ip or port, exit\n");
		exit(0);
	}

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	my_assert(fd != -1, "socket");

	ret = bind(fd, (struct sockaddr*)(&addr), sizeof(addr));
	my_assert(ret == 0, "bind");

	printf("create udp socket success...\n");

	*connect_fd = fd;
}

void* recv_udp(void* udp_connection)
{
	int ret = 0;
	socklen_t from_len = 0;
	struct sockaddr_in from;
	char msg[4096] = "";
	from_len = sizeof(struct sockaddr);

	memset(&from, 0, sizeof(from));
	if(inet_addr(cfg.client_ip) != 0 && cfg.client_port != 0){
		from.sin_family = AF_INET; //设置tcp协议族
		from.sin_port = htons(cfg.service_port); //设置端口号
		from.sin_addr.s_addr = inet_addr(cfg.service_ip); //设置ip地址
	}else{
		printf("No client ip or port, accept any client\n");
	}

	for(;;){
		ret = recvfrom(*((int*)udp_connection), msg, sizeof(msg), 0, (struct sockaddr*)&from, &from_len);
		if(ret == 0){
			printf("udp recv end\n");
			break;
		}
		my_assert(ret!=-1, "recv udp");

		printf(
		"\033[1;40;32m"
		"UDP(%s: %hu): recv len = %d, recv data:\n\n"
		"\033[0m"
		"*****************************************************\n"
		"%s\n"
		"*****************************************************\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), ret, msg);

		memset(msg, 0, sizeof(msg));
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
		"\033[1;40;32m"
		"TCP: recv len = %d, recv data:\n\n"
		"\033[0m"
		"*****************************************************\n"
		"%s\n"
		"*****************************************************\n",
		ret, msg);
		
		memset(msg, 0, sizeof(msg));
	}
}

void communication(int tcp_connection, int udp_connection)
{
	pthread_t tid_tcp, tid_udp;

	pthread_create(&tid_tcp, NULL, recv_tcp, &tcp_connection);
	pthread_create(&tid_udp, NULL, recv_udp, &udp_connection);

	pthread_join(tid_tcp, NULL);
	pthread_cancel(tid_udp);
	pthread_join(tid_udp, NULL);
}

int main(int argc, char* argv[])
{
	int tcp_connection = -1, udp_connection = -1, srv_fd = -1;
	char* cfg_path = NULL;

	atexit(my_exit);

	print_hello_text_img();

	cfg_path = set_cfg_path(argc, argv);

	read_cfg(cfg_path);

	create_tcp_connection(&tcp_connection, &srv_fd);

	create_udp_connection(&udp_connection);

	communication(tcp_connection, udp_connection);

	close(tcp_connection);
	close(udp_connection);
	close(srv_fd);

	return 0;
}