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

#define MAXDATASIZE 100 // max number of bytes we can get at once
#define BACKLOG 10

fd_set master;   // conjunto maestro de descriptores de fichero
fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
struct sockaddr_in remoteaddr; // dirección del cliente
int addrlen;
int fdmax;

int crearSocketDeEscucha(int puerto) {
	struct sockaddr_in myaddr;     // dirección del servidor
	int listener;
	int yes = 1;
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	// obviar el mensaje "address already in use" (la dirección ya se está usando)
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		perror("setsockopt");
		exit(1);
	}
	// enlazar
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(puerto);
	memset(&(myaddr.sin_zero), '\0', 8);
	if (bind(listener, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
		perror("bind");
		exit(1);
	}
	// escuchar
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(1);
	}
	return listener;
}

int aceptarNuevaConexion(int listener) {
	// gestionar nuevas conexiones
	addrlen = sizeof(remoteaddr);
	int newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
	if (newfd == -1) {
		perror("accept");
	}
	else
	{
		FD_SET(newfd, &master); // añadir al conjunto maestro
		if (newfd > fdmax) {    // actualizar el máximo
			fdmax = newfd;
		}
		printf("selectserver: new connection from %s on "
				"socket %d\n", inet_ntoa(remoteaddr.sin_addr), newfd);
	}
	return newfd;
}

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

void *getSin_Addr(struct sockaddr *sa)
{
	return &(((struct sockaddr_in*)sa)->sin_addr); //IPV4
}

int conexionConServidor(char* ip,char* puerto)
{
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *servinfo;//=malloc(sizeof(struct addrinfo*));
	struct addrinfo *p;///=malloc(sizeof(struct addrinfo*));
	int rv;
	char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(ip, puerto, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and connect to the first we can
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
		fprintf(stderr, "failed to connect\n");
		return 2;
	}
	inet_ntop(p->ai_family, getSin_Addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("Conexion con ip %s - ", s);
	freeaddrinfo(servinfo); // all done with this structure

	return sockfd;
}
