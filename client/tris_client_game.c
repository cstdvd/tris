#include "tris_client.h"

int map[9];

void init_map(){
	int i;
	for(i=0;i<9;i++)
		map[i]=0;
}

char *print_symbol(int i){
	char *ret;
	switch(i){
		case 0:
			ret=malloc(4);
			strcpy(ret,"  ");
			break;
		case 1:
			ret=malloc(2);
			strcpy(ret," X");
			break;
		case 2:
			ret=malloc(2);
			strcpy(ret," O");
			break;
	}
	return ret;
}

void show_map(){
	int i;
	printf("\n");
	for(i=0;i<9;i++){
		if((i==0)||(i==3)||(i==6)||(i==8))
			printf("	     |     |     \n");
		if(i==1){
			printf("	 %s  | %s  |  %s \n",print_symbol(map[6]),print_symbol(map[7]),print_symbol(map[8]));
			}
		if(i==4){
			printf("	 %s  | %s  |  %s \n",print_symbol(map[3]),print_symbol(map[4]),print_symbol(map[5]));
			}
		if(i==7){
			printf("	 %s  | %s  |  %s \n",print_symbol(map[0]),print_symbol(map[1]),print_symbol(map[2]));
			}
		if((i==2)||(i==5))
			printf("	_____|_____|_____\n");
	}
	printf("\n");
}

int mark(int n, char sbl){
	int cl;
	if(sbl=='X') cl=1;
	else if(sbl=='O') cl=2;
	if(n<1 || n>9)
		return -2;
	if(map[n-1]!=0)
		return -1;
	map[n-1]=cl;
	return 0;
}

int check_map(){
	int i;
	for(i=0;i<3;i+=3)
		if((map[i]!=0)&&(map[i]==map[i+1])&&(map[i]==map[i+2]))
			return 1;
	for(i=0;i<3;i++)
		if((map[i]!=0)&&(map[i]==map[i+3])&&(map[i]==map[i+6]))
			return 1;
	if((map[0]!=0)&&(map[0]==map[4])&&(map[0]==map[8]))
		return 1;
	if((map[2]!=0)&&(map[2]==map[4])&&(map[2]==map[6]))
		return 1;
	for(i=0;i<9;i++)
		if(map[i]==0) return 0;
	return 2;
}
