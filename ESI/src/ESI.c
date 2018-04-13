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
	char * j=malloc(5);
	strcpy(j,"hola");
	int socketServer=conexionConServidor("127.0.0.1","9034");
	send(socketServer,j,sizeof(j),0);
}



