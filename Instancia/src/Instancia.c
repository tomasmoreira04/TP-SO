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
	int socketServer = conexion_con_servidor("127.0.0.1","9034");
	while(1){
		int tamano=strlen(j);
		recv(socketServer,&tamano,sizeof(int),0);
		recv(socketServer,j,tamano,0);

		printf("\nel valor %s\n", j);
	}
}

