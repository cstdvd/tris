#include "tris_server.h"

fd_set master_read, master_write;
unsigned int len_addr=sizeof(struct sockaddr_in);
struct client* list=NULL;
void* buffer;

int main(int argc, char* argv[])
{
	struct sockaddr_in srv_addr, cl_addr;
	int sk_listener, sk_cl;
	int ret, i, yes=1, del;
	int fdmax;
	fd_set read_fds, write_fds;
	u_int16_t srv_port;
	struct client* new_cl, *next_cl;

	FD_ZERO(&master_read);
	FD_ZERO(&read_fds);
	FD_ZERO(&master_write);
	FD_ZERO(&write_fds);

	if(!argv[1] || !argv[2]){
		printf("Indirizzo IP o porta del server nulli\n");
		exit(1);
	}
	if(atoi(argv[2])<1024 || atoi(argv[2])>65535){
		printf("Porta server non valida\n");
		exit(1);
	}

	sk_listener=socket(AF_INET,SOCK_STREAM,0);	//creazione socket TCP
	if(sk_listener==-1){
		perror("Errore creazione socket TCP");
		exit(1);
	}
	ret=setsockopt(sk_listener,SOL_SOCKET,SO_REUSEADDR,&yes, sizeof(int)); //setsockopt()
	if(ret==-1){
		perror("Errore setsockopt()");
		close(sk_listener);
		exit(1);
	}
	memset(&srv_addr,0,len_addr);
	srv_addr.sin_family=AF_INET;
	ret=inet_pton(AF_INET, (char*)argv[1],&srv_addr.sin_addr.s_addr);
	if(ret==-1){
		perror("Indirizzo IP non valido");
		exit(1);
	}
	srv_port=atoi(argv[2]);
	srv_addr.sin_port=htons(srv_port);
	
	printf("Indirizzo: %s (porta: %s)\n", argv[1], argv[2]);

	ret=bind(sk_listener,(struct sockaddr*)&srv_addr,len_addr);	//bind()
	if(ret==-1){
		perror("Errore bind()");
		close(sk_listener);
		exit(1);
	}
	ret=listen(sk_listener,10);	//listen()
	if(ret==-1){
		perror("Errore listen()");
		close(sk_listener);
		exit(1);
	}
	
	FD_SET(sk_listener, &master_read);
	fdmax=sk_listener;

	for(;;){
		read_fds=master_read;	//copia descrittori
		write_fds=master_write;
		ret=select(fdmax+1, &read_fds, &write_fds, NULL, NULL);
		if(ret==-1){
			perror("Errore select()");
			close(sk_listener);
			exit(1);
		}
		for(i=0;i<=fdmax;i++){
			if(FD_ISSET(i,&read_fds)){
				if(i==sk_listener){
					sk_cl=accept(sk_listener,(struct sockaddr*)&cl_addr,&len_addr);	//accept()
					if(sk_cl==-1){
						perror("Errore connessione al client");
						exit(1);
					}
					printf("Connessione stabilita con il client\n");
					FD_SET(sk_cl, &master_read);
					new_cl=add_cl(sk_cl,cl_addr);	//aggiunge il nuovo client alla lista
					if(sk_cl > fdmax) fdmax = sk_cl;

				}
				else{
					do{
					next_cl=find_cl(i);
					if(next_cl->wait_dim_msg==1){
						buffer=malloc(DIM_CMD);
						ret=recv(i,buffer,DIM_CMD,0);	//riceve dimensione messaggio
						if(ret==0){
							next_cl->close=1;
							break;
						}
						if(ret<0 || ret<DIM_CMD){
							perror("Errore ricezione dimensione messaggio");
							next_cl->close=1;
							break;
						}
						next_cl->dim_msg=*(int*)buffer;
						next_cl->wait_dim_msg=0;
						free(buffer);
					}else{
						buffer=malloc(next_cl->dim_msg);
						ret=recv(i,buffer,next_cl->dim_msg,0);	//riceve messaggio
						if(ret==0){
							next_cl->close=1;
							break;
						}
						if(ret<0 || ret<next_cl->dim_msg){
							perror("Errore ricezione messaggio");
							next_cl->close=1;
							break;
						}
						cl_cmd(next_cl,buffer);
						next_cl->wait_dim_msg=1;
						free(buffer);
					}
					break;
					}while(1);
				}
			}
			if(FD_ISSET(i,&write_fds)){
			do{
				next_cl=find_cl(i);
				if(next_cl->wait_dim_msg==1){
					buffer=malloc(DIM_CMD);
					*(int*)buffer=*(int*)next_cl->next_msg;
					ret=send(i,buffer,DIM_CMD,0);	//invia dimensione messaggio
					if(ret==-1 || ret<DIM_CMD){
						perror("Errore invio dimensione messaggio");
						next_cl->close=1;
						break;
					}
					next_cl->wait_dim_msg=0;
					free(buffer);
				}else{
					ret=send(i,((next_cl->next_msg)+DIM_CMD),*(int*)next_cl->next_msg,0);	//invia messaggio
					if(ret==-1){
						perror("Errore invio messaggio");
						next_cl->close=1;
						break;
					}
					free(next_cl->next_msg);
					next_cl->wait_dim_msg=1;
					FD_CLR(i, &master_write);
					FD_SET(i, &master_read);
				}
			break;
			}while(1);
			}
			
		}
		if(next_cl->close==1){
			printf("%s si è disconnesso\n",next_cl->name);
			del=next_cl->fd;
			delete_cl(del);
			FD_CLR(del,&master_read);
			FD_CLR(del,&master_write);
			close(del);
			if(del==fdmax){
                		while((FD_ISSET(fdmax,&master_read)==0)&&(FD_ISSET(fdmax,&master_write)==0))
                		fdmax-= 1;
			}
		}
	}
	close(sk_listener);
	return 0;
}
