#include "tris_client.h"

void show_menu(){
	printf("\nSono disponibili i seguenti comandi:\n");
 	printf("* !help --> mostra l'elenco dei comandi disponibili\n");
 	printf("* !who --> mostra l'elenco dei client connessi al server\n");
 	printf("* !connect nome_client --> avvia una partita con l'utente nome_client\n");
 	printf("* !disconnect -->disconnette il client dall'attuale partita intrapresa con un altro peer\n");
 	printf("* !quit --> disconnette il client dal server\n");
 	printf("* !show_map --> mostra la mappa di gioco\n");
 	printf("* !hit num_cell --> marca la casella num_cell (valido solo quando è il proprio turno)\n\n");
}
int insert_name(){
	int ret;
	printf("Inserisci il tuo nome: ");
	scanf("%s",name);
	printf("\n");
	buffer=malloc(DIM_CMD);
	*(int*)buffer=DIM_CMD+strlen(name)+1;
	ret=send(sk_tcp,buffer,DIM_CMD,0);	//invia dimensione buffer
	if((ret==-1)||(ret<DIM_CMD)){
		perror("Errore invio dimensione name");
		exit(1);
	}
	free(buffer);
	buffer=malloc(DIM_CMD+strlen(name)+1);
	*(int*)buffer=0;
	strcpy((char*)(buffer+DIM_CMD),name);
	ret=send(sk_tcp,buffer,DIM_CMD+strlen(name)+1,0);
	if((ret==-1)||(ret<DIM_CMD+strlen(name)+1)){
		perror("Errore invio name");
		exit(1);
	}
	free(buffer);
	return 0;
}
int insert_port(){
	int ret;
	int port;
	printf("Inserisci la porta UDP di ascolto: ");
	scanf("%d",&port);
	printf("\n");
	while(port<1023 || port>65535){	
		printf("Porta non valida\n");
		printf("Inserisci la porta UDP di ascolto: ");
		scanf("%d",&port);
		printf("\n");
	}
	buffer=malloc(DIM_CMD);
	*(int*)buffer=DIM_CMD+2;	
	ret=send(sk_tcp, buffer,DIM_CMD,0);
	if(ret==-1){
		perror("Errore invio dimensione porta UDP");
		exit(1);
	}
	free(buffer);
	buffer=malloc(DIM_CMD+2);
	*(int*)buffer=1;
	port_udp=htons((u_int16_t)port);
	*((u_int16_t*)(buffer+DIM_CMD))=port_udp;
	ret=send(sk_tcp, buffer,DIM_CMD+2,0);
	if(ret==-1){
		perror("Errore invio porta UDP");
		exit(1);
	}
	free(buffer);
	return 0;
}
void stdin_cmd(char* cmd){
	int hit, ret;
	if(strcmp(cmd,"!help")==0){
		show_menu();
		wait_cmd=1;
		return;
	}
	if(strcmp(cmd,"!who")==0){
		next_msg=malloc(DIM_CMD);
		*(int*)next_msg=2;
		dim_msg=DIM_CMD;
		FD_CLR(sk_tcp,&master_read);
		FD_SET(sk_tcp,&master_write);
		return;
	}
	if(strncmp(cmd,"!connect ",9)==0){
		if(in_game==1){
			printf("Comando non valido\n");
			wait_cmd=1;
			return;
		}
		adv_name=malloc((strlen(cmd))-8);
		strcpy(adv_name,cmd+9);
		if(strcmp(name,adv_name)==0){
			printf("Impossibile immettere il proprio nome\n");
			wait_cmd=1;
			return;
		}
		dim_msg=DIM_CMD+strlen(adv_name)+1;
		next_msg=malloc(dim_msg);
		*(int*)next_msg=3;
		strcpy((char*)(next_msg+DIM_CMD),adv_name);
		FD_CLR(sk_tcp,&master_read);
		FD_SET(sk_tcp,&master_write);
		return;
	}
	if(strcmp(cmd,"!disconnect")==0){
		if(in_game==0){
			printf("Comando non valido\n");
			wait_cmd=1;
			return;
		}
		printf("Disconnessione avvenuta con successo: TI SEI ARRESO\n");
		in_game=0;
		free(adv_name);
		dim_msg=DIM_CMD;
		next_msg=malloc(DIM_CMD);
		*(int*)next_msg=4;
		FD_CLR(sk_tcp,&master_read);
		FD_SET(sk_tcp,&master_write);
		wait_cmd=1;
		return;
	}
	if(strcmp(cmd,"!quit")==0){
		if(in_game==1){
			printf("Comando non valido\n");
			wait_cmd=1;
			return;
		}
		quit=1;
		wait_cmd=1;
		return;
	}
	if(strcmp(cmd,"!show_map")==0){
		if(in_game==0){
			printf("Comando non valido\n");
			wait_cmd=1;
			return;
		}
		show_map();
		wait_cmd=1;
		return;
	}
	if(strncmp(cmd,"!hit ",5)==0){
		if(in_game==0 || own_turn==0){
			printf("Comando non valido\n");
			wait_cmd=1;
			return;
		}
		hit=atoi(cmd+5);
		ret=mark(hit,symbol);
		if(ret==-1){
			printf("Casella già occupata\n");
			wait_cmd=1;
			return;
		}
		if(ret==-2){
			printf("Casella inesistente\n");
			wait_cmd=1;
			return;
		}
		next_msg_udp=malloc(UDP_MSG);
		*(int*)next_msg_udp=1;
		*(int*)(next_msg_udp+DIM_CMD)=hit;
		FD_SET(sk_udp,&master_write);
		ret=check_map();
		if(ret!=0){
			dim_msg=DIM_CMD+4;
			next_msg=malloc(dim_msg);
			*(int*)next_msg=10;
			*(int*)(next_msg+DIM_CMD)=ret;
			if(ret==1) printf("HAI VINTO\n");
			else if(ret==2) printf("PATTA\n");	
			FD_CLR(sk_tcp,&master_read);
			FD_SET(sk_tcp,&master_write);
			free(adv_name);
			in_game=0;
			first=0;
		}else printf("E' il turno di %s\n",adv_name);
		own_turn=0;
		wait_cmd=1;
		return;
	}
	printf("Comando non riconosciuto\n");
	wait_cmd=1;
	return;
}

void tcp_cmd(void *msg){
	int res, i, cmd;
	cmd=*(int*)msg;
	char *appo, available, r[2];
	fd_set tmp_read_set;
	FD_ZERO(&tmp_read_set);
	FD_SET(0, &tmp_read_set);
	switch(cmd){
		case 2:		//comando !who
			appo=malloc(dim_msg);
			strcpy(appo,(char*)(msg+4));
			available=appo[0];
			for(i=1; i<strlen(appo); i++)
			{
				if(appo[i]==','){
					if(available=='0') printf("(occupato)");
					printf(" ");
					available=appo[++i];
				}else printf("%c", appo[i]);
			}
			printf("\n");
			wait_cmd=1;
			break;
		case 3:		//comando !connect
			if(*(int*)(msg+4)==0){
				printf("Impossibile connettersi a %s: l'utente è inesistente\n",adv_name);
				free(adv_name);
				wait_cmd=1;
			}else if(*(int*)(msg+4)==1){
				printf("Impossibile connettersi a %s: l'utente è occupato\n",adv_name);
				free(adv_name);
				wait_cmd=1;
			}
			break;
		case 4:		//comando !disconnect
			printf("%s si è disconnesso: HAI VINTO\n",adv_name);
			in_game=0;
			FD_CLR(sk_udp,&master_read);
			free(adv_name);
			wait_cmd=1;
			first=0;
			break;
		case 8:		//richiesta di connessione
			adv_name=malloc(strlen((char*)(msg+DIM_CMD))+1);
			strcpy(adv_name,(char*)(msg+DIM_CMD));
			printf("%s vuole connettersi con te. Accettare? (y/n)\n",adv_name);
			do{
				select(1, &tmp_read_set, NULL, NULL, NULL);
				scanf("%s",r);
				flush(stdin);
			}while(r[0]!='y' && r[0]!='n');
			dim_msg=5;
			next_msg=malloc(dim_msg);
			*(int*)next_msg=8;
			*(char*)(next_msg+DIM_CMD)=r[0];
			if(r[0]=='n') wait_cmd=1;
			FD_CLR(sk_tcp,&master_read);
			FD_SET(sk_tcp,&master_write);
			break;
		case 9:		//risposta alla richiesta di connessione
			res=*(int*)(msg+DIM_CMD);
			if(res==0){		//l'utente avversario ha rifiutato
				printf("Impossibile connettersi a %s: l'utente ha rifiutato la partita\n",adv_name);
				free(adv_name);
			}else if(res==1){	//l'utente avversario ha accettato
				printf("%s ha accettato la partita\n",adv_name);
	 			adv_addr.sin_port=*(u_int16_t*)(msg+DIM_CMD+4+INET_ADDRSTRLEN);
				inet_pton(AF_INET,(char*)(msg+DIM_CMD+4), &(adv_addr.sin_addr));
				init_map();
				in_game=1;
				symbol='X';
				adv_symbol='O';
				printf("il tuo simbolo è %c\n",symbol);
				own_turn=1;
				printf("E' il tuo turno:\n");
			}else if(res==2){	//l'utente che ha accettato riceve indirizzo avversario
	 			adv_addr.sin_port=*((u_int16_t*)(msg+DIM_CMD+4+INET_ADDRSTRLEN));
	 			inet_pton(AF_INET,(char*)(msg+DIM_CMD+4), &(adv_addr.sin_addr));
				init_map();
				in_game=1;
				symbol='O';
				adv_symbol='X';
				printf("il tuo simbolo è %c\n",symbol);
				own_turn=0;
				printf("E' il turno di %s\n",adv_name);
			}
			wait_cmd=1;
			break;
		case 10:	//partita finita
			if(in_game==1){
				res=*(int*)(msg+DIM_CMD);
				if(res==1) printf("HAI PERSO\n");
				else if(res==2) printf("PATTA\n");
				in_game=0;
				free(adv_name);
				FD_CLR(sk_udp,&master_read);
				wait_cmd=1;
				first=0;
			}
			break;
	}
	return;
}

void udp_cmd(void*msg){
	int cmd, ret, hit;
	cmd=*(int*)msg;
	switch(cmd){
		case 1:		//comando !hit client avversario
			hit=*(int*)(msg+DIM_CMD);
			ret=mark(hit,adv_symbol);
			printf("%s ha marcato la casella %d\n",adv_name,hit);
			ret=check_map();
			if(ret!=0){
				if(ret==1) printf("HAI PERSO\n");
				else printf("PATTA\n");
				in_game=0;
				free(adv_name);
			}else printf("E' il tuo turno:\n");
			own_turn=1;
			break;
	}
	return;
}
