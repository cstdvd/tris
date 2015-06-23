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
#define UDP_MSG 8
#define flush(stdin) while ((getchar()) != '\n')

//variabili tris_client.c
extern int sk_tcp, sk_udp;
extern struct sockaddr_in srv_addr,adv_addr,udp_addr;
extern u_int16_t port_udp;
extern unsigned int len_addr;
extern fd_set master_read, master_write;
extern int in_game, quit;
extern void *buffer, *buffer_udp, *next_msg, *next_msg_udp;
extern int wait_dim_msg, wait_cmd;
extern int dim_msg;
extern char *name, *adv_name;
extern char symbol, adv_symbol;
extern int own_turn;
extern int first;

//funzioni tris_client_fun.c
extern void show_menu();
extern int insert_name();
extern int insert_port();
extern void stdin_cmd(char* cmd);
extern void tcp_cmd(void *msg);
extern void udp_cmd(void *msg);

//variabili e funzioni tris_client_game.c
extern int map[9];
extern void init_map();
extern char *print_symbol(int i);
extern void show_map();
extern int mark(int n, char sbl);
extern int check_map();
