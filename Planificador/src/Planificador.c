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
#include <commons/collections/list.h>

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
	inicializar_listas();
	Configuracion* planif_cfg = cargar_configuracion("Configuracion.cfg");
	//pthread_t thread_consola, thread_escucha;
	//pthread_create(&thread_consola, NULL, iniciar_consola, NULL);
	ultimo_id = 0;
	RecibirConexiones(planif_cfg->puerto_escucha);
	//pthread_join(thread_consola, NULL);
	destruir_listas();
	config_destroy(planif_cfg);
}

void RecibirConexiones(int puerto) {
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
						int accion = recibirMensaje(i,&stream);
						switch(accion) {
							case 1: //accion = 1 entonces el proceso es NUEVO
								printf("me llego: %s \n",(char*) stream);
								ingreso_cola_de_listos(esi);
								break;
							case 2: //accion = 2 ME BLOQUEO
							break;
							case 0: //accion = 0 me desconecto
								close(socket);
								FD_CLR(i,&master);
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

void cargar_datos_de_esi(ESI* esi) {
	esi->id = ultimo_id;
	//luego hay que agregar otros campos
}

void ingreso_cola_de_listos(ESI* esi) {
	//agregar validaciones
	cargar_datos_de_esi(esi);
	list_add(cola_de_listos, &esi);
}

void movimiento_entre_estados(ESI* esi, int movimiento) {
	switch(movimiento) {
		case hacia_listos:
			mover_esi(esi, cola_de_listos);
			break;
		case hacia_ejecutando:
			//ejecutar
			break;
		case hacia_bloqueado:
			mover_esi(esi, cola_de_bloqueados);
			break;
		case hacia_finalizado:
			mover_esi(esi, cola_de_finalizados);
			break;
	}
}

int _es_esi(ESI* a, ESI* b) {
	return a->id == b->id;
}

void mover_esi(ESI* esi, t_list* nueva_lista) {
	t_list* lista_anterior = lista_por_numero(esi->cola_actual);
	list_remove_by_condition(lista_anterior, (void*) _es_esi);
	list_add(nueva_lista, esi);
}

t_list* lista_por_numero(int numero) {
	switch (numero){
		case 1:
			return cola_de_listos;
			break;
		case 2:
			return cola_de_bloqueados;
			break;
		case 3:
			return cola_de_finalizados;
			break;
		default:
			return NULL; //no se
	}
}

void inicializar_listas() {
	cola_de_listos = list_create();
	cola_de_bloqueados = list_create();
	cola_de_finalizados = list_create();
	lista_claves_bloqueadas = list_create();
}

void destruir_listas() {
	list_destroy(cola_de_listos);
	list_destroy(cola_de_bloqueados);
	list_destroy(cola_de_finalizados);
	list_destroy(lista_claves_bloqueadas);
}


