#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include  <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../Bibliotecas/src/Socket.c"

#define PORT 9034

void main() {
	int listener;
	char buf[256];
	int nbytes;

	int i, j;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	listener = crear_socket_de_escucha(PORT);

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
					aceptar_nueva_conexion(listener);
				} else {
					int tamanio;
					recv(i, &tamanio, sizeof(int), 0);

					printf(" tamanio:   %s\n  ", tamanio);


					if ((nbytes = recv(i, buf, tamanio, 0)) <= 0) {
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
									int tamanio_enviar = strlen(buf);

									send(j, &tamanio_enviar, sizeof(int), 0);

									if (send(j, buf, tamanio_enviar, 0) == -1) {
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

