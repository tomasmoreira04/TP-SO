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

//hay que revisar si está bien esto
typedef struct {
	int id;
	int estimacion_anterior; //la inicial esta dada por arch de config
	int rafaga_anterior;
	char* claves[LARGO_CLAVE];
} ESI;

void sentencia_ejecutada() {
	//evento
}

float estimar(ESI esi, float alfa) {
	return alfa * esi.rafaga_anterior + (1 - alfa) * esi.estimacion_anterior;
}
/*
ESI* esi_rafaga_mas_corta(ESI** esis, float alfa) {
	ESI* esi_mas_corto = esis[0];
	for (int i = 0; i < num_esis(esis); i++)
		if (estimar(esis[i], alfa) < estimar(esi_mas_corto, alfa))
			esi_mas_corto = esis[i];
	return esi_mas_corto;
}*/

ESI* esi_resp_ratio_mas_corto(ESI** esis) {
	//
	return NULL;
}

int main() {
	cola_de_listos = list_create();
	Configuracion* configuracion = cargar_configuracion("Configuracion.cfg");
	//pthread_t thread_consola, thread_escucha;
	//pthread_create(&thread_consola, NULL, iniciar_consola, NULL);
	//RecibirConecciones(configuracion->puerto_escucha);
	//pthread_join(thread_consola, NULL);
	list_destroy(cola_de_listos);
}

void RecibirConecciones(int puerto) {
	int listener;
	char buf[256];
	int nbytes;

	int i, j;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	int socketCoordinador=9999;

	listener = crear_socket_de_escucha(puerto);


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
					//Y SI PONEMOS TOoDO EN UN SWITCH?
					if(i==socketCoordinador){
						//ACCIONES Coordinador
					}
					else{ // Este es el ESI
						void *stream;
						ESI esi; //<--luego vemos como manejar esto, seguro que con ids
						int accion=recibirMensaje(i,&stream);
						switch(accion) {
							case 1:{ //accion = 1 entonces el proceso es NUEVO
								printf("me llego: %s \n",(char*) stream);
								ingreso_cola_de_listos(esi);
							}
							break;
							case 2: { //accion = 2 replanificar?
							}
							break;
							case 0: { //accion = 0 me desconecto
								close(socket);
								FD_CLR(i,&master);
							}
							break;
						}
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
	//strcpy(configuracion->claves_bloqueadas, config_get_array_value(archivo, "CLAVES_BLOQUEADAS"));
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
	config_set_value(config, "PUERTO_ESCUCHA", "9034");
	config_set_value(config, "ALGORITMO_PLANIFICACION", algoritmo_planificacion);
	config_set_value(config, "ESTIMACION_INICIAL", "5");
	config_set_value(config, "IP_COORDINADOR", "127.0.0.1");
	config_set_value(config, "PUERTO_COORDINADOR", "8000");
	config_set_value(config, "CLAVES_BLOQUEADAS", "[materias:K3002,materias:K3001,materias:K3003]");
	config_save(config);
	return config;
}

void ingreso_cola_de_listos(ESI* esi) {
	//agregar validaciones
	list_add(cola_de_listos, &esi);
}

void movimiento_entre_estados(ESI esi, int movimiento) {

	//esqueleto del movimiento
	switch(movimiento) {
		case hacia_listos:{
			printf("estoy en la cola de listos!");
			//hacer algo
		} break;

		case hacia_ejecutando: {
			printf("estoy ejecutando!");
		} break;

		case hacia_bloqueado: {
			printf("estoy en la coal de blqueados!")
		} break;

		case hacia_finalizado: {
			printf("finalice!! :D")
		}
	}
}
