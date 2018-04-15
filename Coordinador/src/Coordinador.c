#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include  <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../Librerias/src/Socket.c"

#define PORT 9034
void main() {
	int listener;
	char buf[256];
	int nbytes;

	int i, j;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	listener = crearSocketDeEscucha(PORT);

	FD_SET(listener, &master);
	fdmax = listener;
	while (1) {
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}
		for (i = 3; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener) {
					aceptarNuevaConexion(listener);
				} else {
					int tamano;
					recv(i,&tamano,sizeof(int),0);

					printf(" tamano:   %d\n  ",tamano);


					if ((nbytes = recv(i, buf, tamano, 0)) <= 0) {
						if (nbytes == 0) {
							printf("selectserver: socket %d hung up\n", i);
						} else {
							perror("recv");
						}
						close(i);
						FD_CLR(i, &master);
					} else {
						printf("me llego %s\n", buf);
						for (j = 3; j <= fdmax; j++) {
							if (FD_ISSET(j, &master)) {
								if (j != listener && j != i) {
									int tamanoEnviar=strlen(buf);

									send(j,&tamanoEnviar,sizeof(int),0);

									if (send(j, buf, tamanoEnviar, 0) == -1) {
										perror("send");
									}
								}
							}
						}
						//consola planif
					}
				}
			}
		}
	}
}

