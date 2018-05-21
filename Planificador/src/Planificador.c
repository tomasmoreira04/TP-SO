#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include "Consola.h"
#include "Planificador.h"
#include "../../Bibliotecas/src/Color.h"
#include "../../Bibliotecas/src/Socket.c"
#include "../../Bibliotecas/src/Configuracion.c"
#include <commons/collections/list.h>
#include "../../ESI/src/ESI.h"
#include <parsi/parser.h> //para el struct t_operacion_esi

typedef enum { sjf_sd, sjf_cd, hrrn, fifo } Algoritmo;

ConfigPlanificador config;

int main() {
	inicializar_estructuras();
	config = cargar_config_planificador();
	pthread_t thread_consola;
	pthread_create(&thread_consola, NULL, iniciar_consola, NULL);
	ultimo_id = 0;
	recibir_conexiones(config);
	pthread_join(thread_consola, NULL);
	destruir_estructuras();
}

void recibir_conexiones() {
	int listener;
	char buf[256];
	int nbytes;

	int i, j;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	int socketCoordinador = 9999;
	listener = crear_socket_de_escucha(config.puerto_escucha);


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
					if(i == socketCoordinador) {
						//ACCIONES Coordinador
					}
					else { // Este es el ESI

						void* mensaje;
						int accion = recibirMensaje(i, &mensaje);
						ESI* esi = malloc(sizeof(ESI));
						esi->id = ultimo_id;
						switch(accion) {
							case nuevo_esi:
								proceso_nuevo(esi, *(int*)mensaje);
								break;
							case instruccion_esi: {
								procesar_instruccion_esi(esi->id, *(t_operacion*)mensaje);
								break;
							}
							default:
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


void procesar_instruccion_esi(int id_esi, t_operacion op) {
	switch (op.tipo) {
	case GET:
		printf("El ESI " GREEN "%d" RESET " necesita la clave " CYAN "<%s>\n" RESET, id_esi, op.clave);
		break;
	case STORE:
		printf("El ESI " GREEN "%d" RESET " necesita hacer store de la clave " CYAN "<%s>\n" RESET, id_esi, op.clave);
		break;
	case SET:
		printf("El ESI " GREEN "%d" RESET " necesita setear la clave " CYAN "<%s> " RESET "con el valor " MAGENTA "<%s>\n" RESET, id_esi, op.clave, op.valor);
		break;
	}
}

void proceso_nuevo(ESI* esi, int rafagas) {
	printf( "\nNuevo ESI con" GREEN " %d rafagas " RESET, rafagas);
	ultimo_id++;
	esi->estimacion_anterior = config.estimacion_inicial;
	esi->id = ultimo_id;
	ingreso_cola_de_listos(esi);
	printf("(ID: %d, ESTIMACION: %d)\n", esi->id, esi->estimacion_anterior);
}

void ingreso_cola_de_listos(ESI* esi) {
	/*if (hay_desalojo(algoritmo)) //llega uno a listo, y hay desalojo -> ver
		replanificar();*/
	mover_esi(esi, cola_de_listos);

}

void movimiento_entre_estados(ESI* esi, int movimiento) {
	switch(movimiento) {
		case hacia_listos:
			mover_esi(esi, cola_de_listos);
			break;
		case hacia_ejecutando:
			ejecutar(config.algoritmo_planif);
			break;
		case hacia_bloqueado:
			mover_esi(esi, cola_de_bloqueados);
			break;
		case hacia_finalizado:
			mover_esi(esi, cola_de_finalizados);
			break;
	}
}

int hay_desalojo(int algoritmo) {
	return 0;
}

void ejecutar(char* algoritmo) {
	int algo = numero_algoritmo(algoritmo);
	switch(algo) {
	case fifo:
		ejecutar_por_fifo();
		break;
	case sjf_sd:
		//implementar
		break;
	case sjf_cd:
		//implementar
		break;
	case hrrn:
		//implementar
		break;
	}
}

int numero_algoritmo(char* nombre) {
	if (strcmp(nombre, "SJF-SD") == 0)
		return 0;
	if (strcmp(nombre, "SDJ-CD") == 0)
		return 1;
	if (strcmp(nombre, "HRRN") == 0)
		return 2;
	return 3; //default: fifo
}

int _es_esi(ESI* a, ESI* b) {
	return a->id == b->id;
}

void mover_esi(ESI* esi, t_list* nueva_lista) {
	if (esi->cola_actual != 0) { //si el esi es nuevo me tiraria segm fault xD
		t_list* lista_anterior = lista_por_numero(esi->cola_actual);
		list_remove_by_condition(lista_anterior, (void*) _es_esi);
	}
	list_add(nueva_lista, esi);
}

t_list* lista_por_numero(int numero) {
	switch (numero){
		case 1:
			return cola_de_listos;
		case 2:
			return cola_de_bloqueados;
		case 3:
			return cola_de_finalizados;
	}
	return NULL;
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
	ESI* esi = list_remove(cola_de_listos , 0);
	//enviarMensaje(); habra enviarle mensaje al esi correspondiente
	puts("executing..");
}

void ejecutar_por_sjf() {
	ESI* esi = esi_rafaga_mas_corta();
	puts("executing..");
}

void sentencia_ejecutada() {
	//evento
}

float estimar(ESI* esi, float alfa) {
	return (alfa/100) * esi->rafaga_anterior + (1 - (alfa/100)) * esi->estimacion_anterior;
} //alfa entre 0 y 100


ESI* esi_rafaga_mas_corta() {
	list_sort(cola_de_listos, (void*) _es_mas_corto);
	return list_remove(cola_de_listos, 0);
}

int _es_mas_corto(ESI* a, ESI* b) {
	return estimar(a, config.alfa_planif) < estimar(b, config.alfa_planif);
}

ESI* esi_resp_ratio_mas_corto() {
	return NULL;
}
