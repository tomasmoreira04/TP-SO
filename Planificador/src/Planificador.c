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
#include "Consola.h"
#include "Planificador.h"
#include "../../Bibliotecas/src/Socket.c"
#include <commons/config.h>
#include <commons/string.h>

#define PORT 9034

int main() {
	Configuracion* configuracion = cargar_configuracion("Configuracion.cfg");
	//pthread_t thread_consola, thread_escucha;
	//pthread_create(&thread_consola, NULL, iniciar_consola, NULL);
	RecibirConecciones();
	//pthread_join(thread_consola, NULL);
}

void RecibirConecciones() {
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
				}
				else {
					int tamanio;
					recv(i, &tamanio, sizeof(int), 0);

					//puts(tamanio);
					printf(" tamanio:   %d\n  ", tamanio);

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
						/*for (j = 3; j <= fdmax; j++) {
							if (FD_ISSET(j, &master)) {
								if (j != listener && j != i) {
									int tamanio_enviar = strlen(buf);

									send(j, &tamanio_enviar, sizeof(int), 0);

									if (send(j, buf, tamanio_enviar, 0) == -1) {
										perror("send");
									}
								}
							}
						}*/
						//consola planif
					}
				}
			}
		}
	}
}

Configuracion* cargar_configuracion(char* ruta) {
	char* campos[6] = { "PUERTO_ESCUCHA","ESTIMACION_INICIAL","PUERTO_COORDINADOR","IP_COORDINADOR","ALGORITMO_PLANIFICACION", "CLAVES_BLOQUEADAS"};
	t_config* archivo = crear_archivo_configuracion(ruta, campos);
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	configuracion->puerto_escucha = config_get_int_value(archivo,"PUERTO_ESCUCHA");
	configuracion->estimacion_inicial = config_get_int_value(archivo, "ESTIMACION_INICIAL");
	configuracion->puerto_coordinador = config_get_int_value(archivo, "PUERTO_COORDINADOR");
	strcpy(configuracion->ip_coordinador, config_get_string_value(archivo,"IP_COORDINADOR"));
	strcpy(configuracion->algoritmo_planificacion, config_get_string_value(archivo, "ALGORITMO_PLANIFICACION"));
	return configuracion;
}


int estan_todos_los_campos(t_config* config, char** campos) {
	for (int i = 0; i < config_keys_amount(config); i++)
		if(!config_has_property(config, campos[i]))
			return 0;
	return 1;
}

t_config* crear_archivo_configuracion(char* ruta, char** campos) {
	t_config* config = config_create(ruta);
	if(config == NULL /*|| !estan_todos_los_campos(config, campos)*/)
		return crear_prueba_configuracion("SJF-CD");
	return config;
}

t_config* crear_prueba_configuracion(char* algoritmo_planificacion) {
	char* ruta = "Configuracion.cfg";
	fclose(fopen(ruta, "w"));
	t_config *config = config_create(ruta);
	config_set_value(config, "PUERTO_ESCUCHA", "8000");
	config_set_value(config, "ALGORITMO_PLANIFICACION", algoritmo_planificacion);
	config_set_value(config, "ESTIMACION_INICIAL", "5");
	config_set_value(config, "IP_COORDINADOR", "127.0.0.1");
	config_set_value(config, "PUERTO_COORDINADOR", "8000");
	config_set_value(config, "CLAVES_BLOQUEADAS", "[materias:K3002,materias:K3001,materias:K3003]");
	config_save(config);
	return config;
}
