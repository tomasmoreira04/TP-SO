#include "Coordinador.h"

Configuracion* leerArchivoConfiguracion(t_config* arch){
	Configuracion* conf = malloc(sizeof(Configuracion));
	conf->puerto= config_get_int_value(arch, "PUERTO");
	strcpy(conf->algoritmo, config_get_string_value(arch,"ALGORITMO"));
	conf->cant_entradas = config_get_int_value(arch, "CANTIDAD_ENTRADAS");
	conf->tamanio_entradas = config_get_int_value(arch, "TAMANIO_ENTRADAS");
	conf->retardo = config_get_int_value(arch, "RETARDO");
	return conf;
}


void mostrarPorPantallaConfig(Configuracion* config){
	puts("----------------PROCESO COORDINADOR--------------");
	printf("PUERTO: %i\n", config->puerto);
	printf("ALGORITMO: %s\n", config->algoritmo);
	printf("CANTIDAD DE ENTRADAS: %i\n", config->cant_entradas);
	printf("TAMANIO DE ENTRADAS: %i\n", config-> tamanio_entradas);
	printf("RETARDO: %i\n", config->retardo);

}


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

