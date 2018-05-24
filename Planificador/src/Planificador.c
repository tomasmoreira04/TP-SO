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
#include "../../Bibliotecas/src/Estructuras.h"
#include <commons/collections/list.h>
#include <parsi/parser.h> //para el struct t_operacion_esi

typedef enum { sjf_sd, sjf_cd, hrrn, fifo } Algoritmo;

ConfigPlanificador config;

int main() {
	inicializar_estructuras();
	config = cargar_config_planificador();
	setbuf(stdout, NULL); //a veces el printf no andaba, se puede hacer esto o un fflush

	pthread_t thread_consola;
	pthread_create(&thread_consola, NULL, iniciar_consola, NULL);
	ultimo_id = 0;
	recibir_conexiones(config);
	pthread_join(thread_consola, NULL);
	destruir_estructuras();
}

void recibir_conexiones() {
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	int listener = crear_socket_de_escucha(config.puerto_escucha);
	int coordinador = conectar_con_coordinador(listener);

	FD_SET(listener, &master);
	fdmax = coordinador;
	while (true) {
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}
		for (int i = 0; i <= fdmax; i++)
			if (FD_ISSET(i, &read_fds))
				recibir_mensajes(i, listener, coordinador);
	}
}

int conectar_con_coordinador(int listener) { //fijarse bien esto, conectar siempre primero el coordinador
	int socket_coord = aceptar_nueva_conexion(listener);
	printf(MAGENTA "\nCOORDINADOR " RESET "conectado en socket " GREEN "%d" RESET, socket_coord);
	return socket_coord;
}

void recibir_mensajes(int socket, int listener, int coordinador) {
	if (socket == listener)
		aceptar_nueva_conexion(listener);
	else if (socket == coordinador)
		procesar_mensaje_coordinador(socket);
	else procesar_mensaje_esi(socket);
}

void procesar_mensaje_coordinador(int coordinador) {
	//FD_CLR(coordinador, &master);
}

void procesar_mensaje_esi(int socket) {
	void* mensaje;
	int accion = recibirMensaje(socket, &mensaje);

	switch(accion) {
		case nuevo_esi:
			proceso_nuevo(*(int*)mensaje);
			break;
		default:
			FD_CLR(socket, &master);
			break;
	}
}

void proceso_nuevo(int rafagas) {
	ESI* nuevo_esi = malloc(sizeof(ESI));
	nuevo_esi->estimacion_anterior = config.estimacion_inicial;
	nuevo_esi->id = ++ultimo_id;
	nuevo_esi->cant_rafagas = rafagas;
	ingreso_cola_de_listos(nuevo_esi);
	printf( "\nNuevo ESI con" GREEN " %d rafagas " RESET, nuevo_esi->cant_rafagas);
	printf("(ID: %d, ESTIMACION: %d)\n", nuevo_esi->id, nuevo_esi->estimacion_anterior);
}

void ingreso_cola_de_listos(ESI* esi) {
	/*if (hay_desalojo(algoritmo)) //llega uno a listo, y hay desalojo -> ver
		replanificar();*/
	mover_esi(esi, cola_de_listos);

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
	if (esi->cola_actual != NULL)
		list_remove(esi->cola_actual, esi->posicion);
	esi->posicion = list_add(nueva_lista, esi);
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
