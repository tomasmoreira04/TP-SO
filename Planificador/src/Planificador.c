#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../Librerias/src/Socket.c"

#define PORT 9034   // puerto en el que escuchamos
void main() {

	// número máximo de descriptores de fichero
	int listener;     // descriptor de socket a la escucha
	char buf[256];    // buffer para datos del cliente
	int nbytes;

	int i, j;
	FD_ZERO(&master);    // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	listener = crearSocketDeEscucha(PORT);

	// obtener socket a la escucha
	// añadir listener al conjunto maestro
	FD_SET(listener, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = listener; // por ahora es éste
	// bucle principal
	while (1) {
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}
		// explorar conexiones existentes en busca de datos que leer
		for (i = 3; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == listener) {
					aceptarNuevaConexion(listener);
				} else {

					// gestionar datos de un cliente
					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0) {
							// conexión cerrada
							printf("selectserver: socket %d hung up\n", i);
						} else {
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					} else {
						printf("me llego %s\n", buf);
						// tenemos datos de algún cliente
						for (j = 3; j <= fdmax; j++) {
							// ¡enviar a todo el mundo!
							if (FD_ISSET(j, &master)) {
								// excepto al listener y a nosotros mismos
								if (j != listener && j != i) {
									if (send(j, buf, nbytes, 0) == -1) {
										perror("send");
									}
								}
							}
						}
						//consola planif
					}
				}
			} // Esto es ¡TAN FEO!
		}
	}
}

