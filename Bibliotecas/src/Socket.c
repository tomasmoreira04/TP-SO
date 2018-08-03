#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "commons/txt.h"
#include "commons/string.h"
#include "commons/collections/list.h"
#include <sys/wait.h>
#include <signal.h>
#include "Socket.h"
#include "Estructuras.h"

void avisar(int socket, Accion accion) {
	int i = 1;
	enviarMensaje(socket, accion, &i, sizeof(i));
}

void esperar(int socket, Accion accion) {
	void* s;
	while (recibirMensaje(socket, &s) != accion);
}

int recibirMensaje(int socket, void** stream) {
	header heder;//MODIFICAR FLAG
	int verificador;

	if((verificador = recv(socket, &heder, sizeof(header), MSG_WAITALL)) <= 0){
		if(verificador == 0)
			return 0;
	}

	*stream = malloc(heder.tamano);

	if((verificador = recv(socket, *stream, heder.tamano, MSG_WAITALL))<=0){
		if(verificador == 0)
			return 0;
	}
	return heder.accion;
}

void enviarMensaje(int socket,int accion,void* contenido,int tamano){
	header heder={.accion=accion,.tamano=tamano};//MODIFICAR FLAG
	void* stream=malloc( sizeof(header)+tamano );
	memcpy(stream, &heder, sizeof(header));
	memcpy(stream+sizeof(header), contenido, tamano);
	send(socket,stream,sizeof(header)+tamano,0);
	free(stream);
}

int crear_socket_de_escucha(int puerto) {
	struct sockaddr_in myaddr;
	int listener;
	int yes = 1;
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		perror("setsockopt");
		exit(1);
	}
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(puerto);
	memset(&(myaddr.sin_zero), '\0', 8);
	if (bind(listener, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
		perror("bind");
		exit(1);
	}
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(1);
	}
	return listener;
}

int aceptar_nueva_conexion(int listener) {
	addrlen = sizeof(remoteaddr);
	int newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
	if (newfd == -1) {
		perror("accept");
	}
	else
	{
		FD_SET(newfd, &master);
		if (newfd > fdmax) {
			fdmax = newfd;
		}
		//printf("\nselectserver: new connection from %s on socket %d\n", inet_ntoa(remoteaddr.sin_addr), newfd);
	}
	return newfd;
}

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

void *getSin_Addr(struct sockaddr *sa) {
	return &(((struct sockaddr_in*)sa)->sin_addr); //IPV4
}

int conexion_con_servidor(char* ip, int port) {
	char puerto[10];
	sprintf(puerto, "%d", port);
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(ip, puerto, &hints, &servinfo)) != 0) {
		fprintf(stderr, "\ngetaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}
	if (p == NULL) {
		fprintf(stderr, "\nfailed to connect\n");
		return 2;
	}
	inet_ntop(p->ai_family, getSin_Addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	//printf("\nConexion con ip %s - \n", s);
	freeaddrinfo(servinfo); // all done with this structure
	return sockfd;
}

void handShake(int socket,int modulo){
	send(socket, &modulo, sizeof(int), 0);//HS
}








