#include "Planificador.h"
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
#include "../../Bibliotecas/src/Color.h"
#include "../../Bibliotecas/src/Socket.c"
#include "../../Bibliotecas/src/Configuracion.c"
#include "../../Bibliotecas/src/Estructuras.h"
#include <commons/collections/list.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

ConfigPlanificador config;
t_list* cola_de_listos;
t_list* cola_de_bloqueados;
t_list* cola_de_finalizados;
t_list* lista_claves_bloqueadas;
ESI* esi_ejecutando;
t_dictionary* estimaciones_actuales;
int ultimo_id;
int planificar = 1;


int main() {
	inicializar_estructuras();
	config = cargar_config_planificador();
	setbuf(stdout, NULL); //a veces el printf no andaba, se puede hacer esto o un fflush
	pthread_t thread_consola;
	pthread_create(&thread_consola, NULL, iniciar_consola, NULL);
	ultimo_id = 0;
	recibir_conexiones();
	pthread_join(thread_consola, NULL);
	destruir_estructuras();
}


void recibir_conexiones() {
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	int listener = crear_socket_de_escucha(config.puerto_escucha);
	FD_SET(listener, &master);
	int coordinador = conectar_con_coordinador(listener);

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
	void* mensaje;
	Accion accion = recibirMensaje(coordinador, &mensaje);
	//int accion = 999;
	switch(accion) {
		case sentencia_coordinador:
			nueva_sentencia(*(t_sentencia*)mensaje);
			break;
		case preguntar_recursos_planificador:
			//ver_disponibilidad_clave((char*)mensaje);
			enviarMensaje(coordinador, recurso_disponible, NULL, 0); //por ahora le digo que si siempre
			break;
		case ejecucion_ok:
			//logear que esta ok
			break;
		case terminar_esi:
			finalizar_esi((ESI*)mensaje);
			break;
		default:
			FD_CLR(coordinador, &master);
			break;
	}
}

void procesar_mensaje_esi(int socket) {
	void* mensaje;
	int accion = recibirMensaje(socket, &mensaje);

	switch(accion) {
		case nuevo_esi:
			proceso_nuevo(*(int*)mensaje, socket);
			//ejecutar_esi(); //por ahora, cada vez que me llega uno nuevo lo ejecuto
			break;
		default:
			FD_CLR(socket, &master);
			break;
	}
}

void ejecutar_esi() { //ESTO SERIA FIFO, POR AHORA DEJALO ASI
	ESI* esi = list_remove(cola_de_listos, 0); //criterio primero en la lista
	enviarMensaje(esi->socket_p, ejecutar_proxima_sentencia, NULL, 0);
	esi_ejecutando = esi;
}

void nueva_sentencia(t_sentencia sentencia) {
	ESI* esi = obtener_esi(sentencia.id_esi);
	switch (sentencia.tipo) {
	case S_GET:
		GET(sentencia.clave, esi);
		break;
	case S_SET:
		SET(sentencia.clave, sentencia.valor, esi);
		break;
	case S_STORE:
		STORE(sentencia.clave, esi);
		break;
	}
}

ESI* obtener_esi(int id) { //al menos una vez voy a tener que filtrar en todo el planificador
	int n = list_size(cola_de_listos);
	ESI* esi;
	for (int i = 0; i < n; i++){
		esi = list_get(cola_de_listos, i);
		if (esi->id == id)
			return esi;
	}
	return NULL;
}

char* mensaje_error(ErrorOperacion tipo) {
	switch (tipo) {
		case error_tamanio_clave:
			return "\nERROR DE TAMAÑO DE CLAVE: ";
		case error_clave_no_identificada:
			return "\nERROR DE CLAVE NO IDENTIFICADA: ";
		case error_comunicacion:
			return "\nERROR DE COMUNICACION: ";
		case error_clave_inaccesible:
			return "\nERROR DE CLAVE INACCESSIBLE: ";
		case error_clave_no_bloqueada:
			return "\nERROR DE CLAVE NO BLOQUEADA: ";
		default:
			return "\nERROR: ";
	}
}

void error_operacion(ErrorOperacion tipo, char* clave, int esi) {
	printf(RED "%s" YELLOW "%s " GREEN "%d" RESET, mensaje_error(tipo), clave, esi);
}

void GET(char* clave, ESI* esi) {
	if (esta_bloqueada(clave)) {
		printf(YELLOW"\nLa clave "GREEN"%s "YELLOW"se encuentra bloqueada, se bloqueará el "GREEN"ESI %d"RESET, clave, esi->id);
		nueva_solicitud_clave(clave, esi);
		bloquear_esi(esi);
	}
	else {
		bloquear_clave(clave, esi);
		printf("\nLa clave "GREEN"%s "RESET"se asigno a "GREEN"ESI %d"RESET, clave, esi->id);
	}
}

void SET(char* clave, char* valor, ESI* esi) {
	t_clave* c = buscar_clave_bloqueada(clave);
	if (c == NULL) {
		error_operacion(error_clave_no_identificada, clave, esi->id);
		finalizar_esi(esi);
	}
	else if (!c->bloqueada && c->esi_duenio != esi) {
		error_operacion(error_clave_no_bloqueada, clave, esi->id);
		finalizar_esi(esi);
	}
	else { //puedo hacer store
		strcpy(c->valor, valor);

		//avisarle al coordinador: operacion set OK

		printf("\nSe ha seteado la clave" GREEN " %s" RESET " con valor" MAGENTA " %s" RESET, c->clave, c->valor);
	}
}

void STORE(char* clave, ESI* esi) { //esi: esi que hace el pedido de store
	t_clave* c = buscar_clave_bloqueada(clave);
	if (c == NULL) {
		error_operacion(error_clave_no_identificada, clave, esi->id);
		finalizar_esi(esi);
	}
	else if (!c->bloqueada && c->esi_duenio != esi) {
		error_operacion(error_clave_no_bloqueada, clave, esi->id);
		finalizar_esi(esi);
	}
	else { //puedo hacer store
		liberar_clave(clave);

		//avisarle al coordinador: operacion store OK

		printf("\nSe ha liberado la clave" GREEN " %s" RESET, c->clave);
	}
}

void finalizar_esi(ESI* esi) {
	mover_esi(esi, cola_de_finalizados);
	liberar_recursos(esi);
}

void liberar_recursos(ESI* esi) {
	int n_claves = sizeof(esi->claves) / LARGO_CLAVE;
	for (int i = 0; i < n_claves; i++)
		liberar_clave(esi->claves[i]);
}

void bloquear_clave(char* clave, ESI* esi) {
	t_clave* entrada = malloc(sizeof(t_clave));
	entrada->bloqueada = 1;
	strcpy(entrada->clave, clave);
	entrada->esi_duenio = esi;
	entrada->esis_esperando = list_create();
	list_add(lista_claves_bloqueadas, entrada);
}

void nueva_solicitud_clave(char* clave, ESI* esi) {
	t_clave* entrada = buscar_clave_bloqueada(clave);
	list_add(entrada->esis_esperando, esi);
}

void liberar_clave(char* clave) {
	t_clave* c = buscar_clave_bloqueada(clave);
	if (c != NULL) {
		c->bloqueada = 0;
		c->esi_duenio = NULL;
		ESI* esi_a_desbloquear = list_remove(c->esis_esperando, 0); //el primero en haberse bloqueado, y lo saco
		desbloquear_esi(esi_a_desbloquear);
	}
}

int esta_bloqueada(char* clave) {
	return buscar_clave_bloqueada(clave) != NULL;
}

t_clave* buscar_clave_bloqueada(char* clave) {
	t_list* lista = lista_claves_bloqueadas;
	for (int i = 0; i < list_size(lista); i++) {
		t_clave* c = list_get(lista, i);
		if (strcmp(clave, c->clave) == 0)
			return c;
	}
	return NULL;
}

void bloquear_esi(ESI* esi) {
	mover_esi(esi, cola_de_bloqueados);
}

void desbloquear_esi(ESI* esi) {
	mover_esi(esi, cola_de_listos);
}

void proceso_nuevo(int rafagas, int socket) {
	ESI* nuevo_esi = malloc(sizeof(ESI));
	nuevo_esi->estimacion_anterior = config.estimacion_inicial;
	nuevo_esi->id = ++ultimo_id;
	nuevo_esi->cant_rafagas = rafagas;
	nuevo_esi->socket_p = socket;
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
	/*algo = procesar algoritmo
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
	}*/
}

int _es_esi(ESI* a, ESI* b) {
	return a->id == b->id;
}

void mover_esi(ESI* esi, t_list* nueva_lista) {
	if (esi->cola_actual != NULL)
		list_remove(esi->cola_actual, esi->posicion);
	esi->cola_actual = nueva_lista;
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
	close(file_descriptors[1]);
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
