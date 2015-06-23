#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#define DIM_CMD 4
#define flush(stdin) while ((getchar()) != '\n')

//variabili tris_server.c
extern fd_set master_read, master_write;
extern unsigned int len_addr;
extern struct client* list;
extern void* buffer;

//funzioni client tris_server_fun.c
struct client{
	char* name;
	u_int16_t port;
	char ip[INET_ADDRSTRLEN];
	int fd, adv_fd;
	int available;
	int close;
	int wait_dim_msg, dim_msg;
	void* next_msg;
	struct client* next;
};
extern struct client* add_cl(int sk,struct sockaddr_in cl_addr);
extern void delete_cl(int sk);
extern void cl_cmd(struct client*cl,void*msg);
extern struct client* find_cl(int sk);
extern struct client* find_cl_name(char *name);
extern void disconnect_cl(int sk);
