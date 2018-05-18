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

void sentencia_ejecutada() {
	//evento
}

float estimar(ESI esi, float alfa) {
	return (alfa/100) * esi.rafaga_anterior + (1 - (alfa/100)) * esi.estimacion_anterior;
} //alfa entre 0 y 100
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
	inicializar_estructuras();
	Configuracion* planif_cfg = cargar_configuracion("Configuracion.cfg");
	//pthread_t thread_consola, thread_escucha;
	//pthread_create(&thread_consola, NULL, iniciar_consola, NULL);
	ultimo_id = 0;
	RecibirConexiones(planif_cfg);
	//pthread_join(thread_consola, NULL);
	destruir_estructuras();
	config_destroy(planif_cfg);
}

void RecibirConexiones(Configuracion* cfg) {
	int listener;
	char buf[256];
	int nbytes;

	int i, j;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	int socketCoordinador=9999;

	listener = crear_socket_de_escucha(cfg->puerto_escucha);


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
					if(i==socketCoordinador) {
						//ACCIONES Coordinador
					}
					else { // Este es el ESI
						/*HACER EL FREE-todavia no se donde ponerlo xdxdxdxdxdxx
						 *y falta hacer validaciones en el malloc
						 */
						ESI* esi = malloc(sizeof(ESI));
						void* stream;
						int accion = recibirMensaje(i, &stream);
						//int valor_recibido = stream;
						//printf("accion nro: %d", accion);
						switch(accion) {
							case 100: //proceso NUEVO
								printf("me llego2: %d \n", *((int*)stream));
								esi->estimacion_anterior = cfg->estimacion_inicial;
								puts("aca esta");
								ingreso_cola_de_listos(esi);
								printf("mi est es %d\n", esi->estimacion_anterior);
								printf("soy el esi con id: %d\n", esi->id);
								accion = 101;
								break;
							case 101: //
								puts("dsasdasda");
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

void ingreso_cola_de_listos(ESI* esi) {
	//agregar validaciones
	esi->id = ultimo_id;
	puts("pase");
	list_add(cola_de_listos, esi);
	ultimo_id++;
}

void movimiento_entre_estados(ESI* esi, int movimiento) {
	switch(movimiento) {
		case hacia_listos:
			mover_esi(esi, cola_de_listos);
			break;
		case hacia_ejecutando:
			ejecutar_por_fifo();
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

void inicializar_estructuras() {
	cola_de_listos = list_create();
	cola_de_bloqueados = list_create();
	cola_de_finalizados = list_create();
	lista_claves_bloqueadas = list_create();
	estimaciones_actuales = dictionary_create();
}

void destruir_estructuras() {
	list_destroy(cola_de_listos);
	list_destroy(cola_de_bloqueados);
	list_destroy(cola_de_finalizados);
	list_destroy(lista_claves_bloqueadas);
	dictionary_destroy(estimaciones_actuales);
}

//para el prox checkpoint esta funcion va a ser mas generica
void ejecutar_por_fifo() {
	ESI* esi = malloc(sizeof(ESI));
	esi = list_remove(cola_de_listos, 0);
	//enviarMensaje(); habra enviarle mensaje al esi correspondiente
	puts("executing..");
}
