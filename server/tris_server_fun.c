#include "tris_server.h"

struct client* add_cl(int sk, struct sockaddr_in cl_addr){
	struct client* elem=malloc(sizeof(struct client));
	elem->fd=sk;
	elem->available=1;
	elem->wait_dim_msg=1;
	elem->close=0;
	inet_ntop(AF_INET,&(cl_addr.sin_addr), elem->ip, INET_ADDRSTRLEN);
	elem->next=list;
	list=elem;
	return elem;
}

void delete_cl(int sk)
{
	struct client* elem;
	struct client* appo=list;
	if(list==NULL) return;
	if(list->fd==sk){
		elem=list;
		list=list->next;
	}
	else{
		while((appo!=NULL)&&(appo->next!=NULL)){
			if (appo->next->fd==sk){
				elem=appo->next;
				appo->next=appo->next->next;
				break;
			}
			appo=appo->next;
		}
	}
	free(elem);
}

struct client* find_cl(int sk){
	struct client* elem=list;
	while(elem!=NULL){
		if(elem->fd==sk)
			return elem;
		elem=elem->next;
	}
	return NULL;
}
struct client* find_cl_name(char* name){
	struct client* elem=list;
	while(elem!=NULL){
		if(strcmp(elem->name,name)==0)
			return elem;
		elem=elem->next;
	}
	return NULL;
}

void disconnect_cl(int sk)
{
	struct client* cl = find_cl(sk);
	cl->available=1;
	cl->next_msg=malloc(DIM_CMD+4);
	*(int*)cl->next_msg=DIM_CMD;
	*(int*)(cl->next_msg+DIM_CMD)=4;
	FD_CLR(sk,&master_read);
	FD_SET(sk,&master_write);
	printf("%s è libero\n",cl->name);
	return;
}

void cl_cmd(struct client*cl, void *msg){
	int len=0,res;
	int cmd;
	cmd=*(int*)msg;
	char* all_name;
	struct client *elem, *adv_cl;
	switch(cmd){
		case 0:		//inserisce nome client
			cl->name=malloc(strlen((char*)(msg+DIM_CMD))+1);
			strcpy(cl->name,(char*)(msg+DIM_CMD));
			break;
		case 1:		//inserisce porta client
			cl->port=*((u_int16_t*)(msg+DIM_CMD));
			printf("%s si è connesso\n",cl->name);
			printf("%s è libero\n",cl->name);
			break;
		case 2:		//comando !who
			for(elem=list;elem!=NULL;elem=elem->next)
				len+=strlen(elem->name)+2;	//lunghezza_nome+virgola+numero per indicare disponibilità
			all_name=malloc(len+1);		
			//all_name[0]='\0';
			for(elem=list;elem!=NULL;elem=elem->next){
				if(elem->available==1)
					strcat(all_name,"1");	//disponibilità prima del nome
				else strcat(all_name,"0");
				strcat(all_name,elem->name);
				strcat(all_name,",");
			}
			all_name[strlen(all_name)]='\0';
			cl->next_msg=malloc(len+1+DIM_CMD+4);
			*(int*)cl->next_msg=len+1+DIM_CMD;
			*(int*)(cl->next_msg+4)=2;
			strcpy((char*)(cl->next_msg+8),all_name);
			free(all_name);
			FD_CLR(cl->fd,&master_read);
			FD_SET(cl->fd,&master_write);
			break;
		case 3:		//comando !connect
			cl->next_msg=malloc(3*DIM_CMD);
			*(int*)cl->next_msg=2*DIM_CMD;
			*(int*)(cl->next_msg+4)=3;
			adv_cl=find_cl_name((char*)(msg+DIM_CMD));
			if(adv_cl==NULL){
				*(int*)(cl->next_msg+8)=0;		//utente inesistente
			}else if(adv_cl->available==0){
				*(int*)(cl->next_msg+8)=1;		//utente occupato
				free(adv_cl);
			}else{
				*(int*)(cl->next_msg+8)=2;
				cl->available=0;
				adv_cl->available=0;		
				cl->adv_fd=adv_cl->fd;
				adv_cl->adv_fd=cl->fd;						
				adv_cl->next_msg=malloc(2*DIM_CMD+strlen(cl->name)+1);
				*(int*)adv_cl->next_msg=DIM_CMD+strlen(cl->name)+1;
				*(int*)(adv_cl->next_msg+4)=8;
				strcpy((char*)(adv_cl->next_msg+8),cl->name);
				FD_CLR(adv_cl->fd,&master_read);
				FD_SET(adv_cl->fd,&master_write);
			}
			cl->wait_dim_msg=1;
			FD_CLR(cl->fd,&master_read);
			FD_SET(cl->fd,&master_write);
			break;
		case 4:		//comando !disconnect
			adv_cl=find_cl(cl->adv_fd);
			cl->available=1;
			printf("%s si è disconnesso da %s\n",cl->name,adv_cl->name);
			printf("%s è libero\n",cl->name);
			printf("%s è libero\n",adv_cl->name);
			adv_cl->available=1;
			adv_cl->next_msg=malloc(DIM_CMD+4);
			*(int*)adv_cl->next_msg=DIM_CMD;
			*(int*)(adv_cl->next_msg+DIM_CMD)=4;
			FD_CLR(adv_cl->fd,&master_read);
			FD_SET(adv_cl->fd,&master_write);
			break;
		case 8:		//risposta comando !connect
			res=(*(char*)(msg+DIM_CMD)=='y')?1:0;
			adv_cl=find_cl(cl->adv_fd);
			if(res==0){	//l'utente ha rifiutato
				adv_cl->next_msg=malloc(DIM_CMD+8);
				*(int*)adv_cl->next_msg=DIM_CMD+4;
				*(int*)(adv_cl->next_msg+4)=9;
				*(int*)(adv_cl->next_msg+4+DIM_CMD)=res;
				adv_cl->available=1;
				cl->available=1;
				FD_CLR(adv_cl->fd,&master_read);
				FD_SET(adv_cl->fd,&master_write);
			}else if(res==1){		//l'utente ha accettato
				adv_cl->next_msg=malloc(4+DIM_CMD+4+INET_ADDRSTRLEN+2);
				*(int*)adv_cl->next_msg=DIM_CMD+4+INET_ADDRSTRLEN+2;
				*(int*)(adv_cl->next_msg+4)=9;
				*(int*)(adv_cl->next_msg+4+DIM_CMD)=res;
				strcpy((char*)(adv_cl->next_msg+4+DIM_CMD+4),cl->ip);
				*((u_int16_t*)(adv_cl->next_msg+4+DIM_CMD+4+INET_ADDRSTRLEN))=cl->port;
				FD_CLR(adv_cl->fd,&master_read);
				FD_SET(adv_cl->fd,&master_write);

				cl->next_msg=malloc(4+DIM_CMD+4+INET_ADDRSTRLEN+2);
				*(int*)cl->next_msg=DIM_CMD+4+INET_ADDRSTRLEN+2;
				*(int*)(cl->next_msg+4)=9;
				*(int*)(cl->next_msg+4+DIM_CMD)=2;
				strcpy((char*)(cl->next_msg+4+DIM_CMD+4),adv_cl->ip);
				*((u_int16_t*)(cl->next_msg+4+DIM_CMD+4+INET_ADDRSTRLEN))=adv_cl->port;
				FD_CLR(cl->fd,&master_read);
				FD_SET(cl->fd,&master_write);
			}
			break;
		case 10:	//partita finita
			adv_cl=find_cl(cl->adv_fd);
			cl->available=1;
			adv_cl->available=1;
			res=*(int*)(msg+DIM_CMD);
			adv_cl->next_msg=malloc(4+DIM_CMD+4);
			*(int*)adv_cl->next_msg=DIM_CMD+4;
			*(int*)(adv_cl->next_msg+4)=10;
			*(int*)(adv_cl->next_msg+4+DIM_CMD)=res;
			FD_CLR(adv_cl->fd,&master_read);
			FD_SET(adv_cl->fd,&master_write);
			printf("%s è libero\n",cl->name);
			printf("%s è libero\n",adv_cl->name);
			break;
	}
	return;
}
