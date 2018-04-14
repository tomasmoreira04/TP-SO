#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../../Librerias/src/Socket.c"
#include "commons/string.h"

void main() {
	char * j=malloc(50);
	int socketServer=conexionConServidor("127.0.0.1","9034");
	while(1){
		fflush(stdin);
		gets(j);
		int tamano=strlen(j)+1;
		printf("\ņ %s \n",j);
		send(socketServer,&tamano,sizeof(int),0);
		send(socketServer,j,tamano,0);
	}
}



