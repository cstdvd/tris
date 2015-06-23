#include "tris_client.h"

int sk_tcp, sk_udp;
u_int16_t port_udp;
struct sockaddr_in srv_addr,adv_addr,udp_addr,adv_addr_appo;
unsigned int len_addr=sizeof(struct sockaddr_in);
int in_game=0, quit=0;
fd_set master_read, master_write;
void *buffer, *buffer_udp, *next_msg, *next_msg_udp;
int wait_dim_msg=1, wait_cmd=1;
int dim_msg;
char *name, *adv_name;
char symbol, adv_symbol;
int own_turn;
int first=1;

int main(int argc, char* argv[])
{
	int ret;
	fd_set read_set, write_set;
	char cmd[29];
	name=malloc(20);
	int fdmax;
	struct timeval timeout;
	timeout.tv_sec=60;
	timeout.tv_usec=0;

	FD_ZERO(&master_read);
	FD_ZERO(&read_set);
	FD_ZERO(&master_write);
	FD_ZERO(&write_set);

	sk_tcp=socket(AF_INET,SOCK_STREAM,0);	//creazione socket TCP
	if(sk_tcp==-1){
		perror("Errore creazione socket TCP");
		exit(1);
	}
	if(atoi(argv[2])<1024 || atoi(argv[2])>65535)
	{
		printf("Porta non valida\n");
		exit(1);
	}
	memset(&srv_addr,0,len_addr);
	srv_addr.sin_family=AF_INET;
	srv_addr.sin_port=htons(atoi(argv[2]));
	ret=inet_pton(AF_INET,argv[1],&srv_addr.sin_addr.s_addr);
	if(ret==0){
		perror("Errore traduzione indirizzo server");
		exit(1);
	}
	
	ret=connect(sk_tcp,(struct sockaddr*)&srv_addr,len_addr);	//connessione al server
	if(ret==-1){
		perror("Errore connessione al server");
		close(sk_tcp);
		exit(1);
	}
	printf("Connessione al server %s (porta %s) effettuata con successo\n",argv[1], argv[2]);

	show_menu();	//mostra menu
	ret=insert_name();	//inserimento nome client
	if(ret==-1){
		perror("Errore inserimento nome");
		exit(1);
	}
	ret=insert_port();	//inserimento porta di ascolto
	if(ret==-1){
		perror("Errore inserimento porta di ascolto");
		exit(1);
	}

	sk_udp=socket(AF_INET,SOCK_DGRAM,0);	//creazione socket UDP
	if(sk_udp==-1){
		perror("Errore creazione socket UDP");
		exit(1);
	}
	memset(&udp_addr,0,len_addr);
	udp_addr.sin_family=AF_INET;
	udp_addr.sin_port=port_udp;
	udp_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	ret=bind(sk_udp, (struct sockaddr*)&udp_addr,len_addr);	//bind()
	if(ret==-1){
		perror("Errore bind()");
		exit(1);
	}
	fdmax=(sk_tcp>sk_udp)?sk_tcp:sk_udp;

	memset(&adv_addr,0,sizeof(adv_addr));
	adv_addr.sin_family=AF_INET;
	memset(&adv_addr_appo,0,sizeof(adv_addr_appo));
	adv_addr_appo.sin_family=AF_INET;

	FD_SET(sk_tcp, &master_read);
	FD_SET(sk_udp, &master_read);
	FD_SET(0, &master_read);
	

	do{
		read_set=master_read;
		write_set=master_write;
		if(wait_cmd==1){
			if(in_game==1) printf("# ");
			else printf("> ");
			wait_cmd=0;
		}
		fflush(stdout);
		ret=select(fdmax+1, &read_set, &write_set, NULL, NULL);
		if(ret==-1){
			perror("Errore select()");
			exit(1);
		}
		if(ret==0 && in_game==1){	//timeout
			printf("Giocatore inattivo: disconnessione\n");
			in_game=0;
			free(adv_name);
			dim_msg=DIM_CMD;
			next_msg=malloc(DIM_CMD);
			*(int*)next_msg=4;
			FD_CLR(sk_tcp,&master_read);
			FD_SET(sk_tcp,&master_write);
			wait_cmd=1;
		}
		if(FD_ISSET(0,&read_set)){
			fgets(cmd,29, stdin);
			if(cmd[strlen(cmd)-1]!='\n') flush(stdin);
			else cmd[strlen(cmd)-1]='\0';
			if(first==1 && in_game==0) first=0;
			else stdin_cmd(cmd);
		}
		if(FD_ISSET(sk_tcp,&read_set)){
			if(wait_dim_msg==1){		//riceve dimensione messaggio
				buffer=malloc(4);
				ret=recv(sk_tcp,buffer,4,0);
				if(ret==-1){
					perror("Errore ricezione dimensione messaggio");
					exit(1);
				}
				dim_msg=*(int*)buffer;
				free(buffer);
				wait_dim_msg=0;
			}else {		//riceve messaggio
				buffer=malloc(dim_msg);
				ret=recv(sk_tcp,buffer,dim_msg,0);
				if(ret==-1 || ret<dim_msg){
					perror("Errore ricezione messaggio");
					exit(1);
				}
				tcp_cmd(buffer);
				free(buffer);
				wait_dim_msg=1;
			}
		}
		if(FD_ISSET(sk_udp,&read_set)){		//riceve dati UDP
			buffer_udp=malloc(UDP_MSG);	
			ret=recvfrom(sk_udp,buffer_udp,UDP_MSG,0,(struct sockaddr*)&adv_addr_appo,&len_addr);
			if(ret==-1){
				printf("Errore ricezione dati socket UDP\n");
				exit(1);
				break;
			}
			if(in_game==1 && adv_addr_appo.sin_addr.s_addr==adv_addr.sin_addr.s_addr){
				udp_cmd(buffer_udp);	//esegue comando UDP
				wait_cmd=1;
			}
			free(buffer_udp);
		}
		if(FD_ISSET(sk_tcp,&write_set)){
			if(wait_dim_msg==1){		//invia dimensione messaggio
				buffer=malloc(4);
				*(int*)buffer=dim_msg;
				ret=send(sk_tcp,buffer,4,0);
				if(ret==-1){
					perror("Errore invio dimensione messaggio");
					exit(1);
				}
				free(buffer);
				wait_dim_msg=0;
			}else{				//invia messaggio
				ret=send(sk_tcp,next_msg,dim_msg,0);
				if(ret==-1){
					perror("Errore invio messaggio");
					exit(1);
				}
				free(next_msg);
				wait_dim_msg=1;
				FD_CLR(sk_tcp, &master_write);
				FD_SET(sk_tcp, &master_read);
			}
		}
		if(FD_ISSET(sk_udp,&write_set)){	//invia dati UDP
			ret=sendto(sk_udp,next_msg_udp,UDP_MSG,0,(struct sockaddr*)&adv_addr,len_addr);
			if(ret==-1){
				printf("Errore invio dati socket UDP\n");
				exit(1);
				break;
			}
			free(next_msg_udp);
			FD_CLR(sk_udp, &master_write);
		}
	}while(quit==0);
	
	close(sk_tcp);
	close(sk_udp);

	return 0;
}

