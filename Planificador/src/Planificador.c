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
#include <pthread.h>
#include "commons/string.h"
#include "Consola.h"
#include "Planificador.h"
#include "../../Bibliotecas/src/Socket.c"

void main() {
	pthread_t thread_consola, thread_escucha;
	pthread_create(&thread_consola, NULL, iniciar_consola, NULL);
	pthread_create(&thread_escucha, NULL, conectar_coordinador, NULL);
	pthread_join(thread_consola, NULL);
}

void* conectar_coordinador() {
	char* buffer = "hola";
	int socket_server = conexion_con_servidor("127.0.0.1", "9034");
	send(socket_server, buffer, strlen(buffer), 0);
		/*while(1){
			int tamanio = strlen(j);
			recv(socket_server, j, tamanio, 0);
			printf("\nel valor %s\n", j);
		}*/
	return NULL;
}
