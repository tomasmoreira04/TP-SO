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
#include "../../Bibliotecas/src/Semaforo.c"
#include <commons/collections/list.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

ConfigPlanificador config;
t_list* cola_de_listos;
t_list* cola_de_bloqueados;
t_list* cola_de_finalizados;
t_list* lista_claves_bloqueadas;
ESI* esi_ejecutando = NULL;
t_dictionary* estimaciones_actuales;
int ultimo_id;
int planificar = 1;
int coordinador_conectado = 0;
int hay_hilo_coordinador = 0;

int coord = 0;

//semaforos
pthread_mutex_t lista_disponible = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t siguiente_sentencia = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ejecucion = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acceso_cola_listos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t recibir = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fds_disponibles = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_coord_con = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t coord_ok = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t numero_esi = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_esis = PTHREAD_MUTEX_INITIALIZER;



int main(int argc, char* argv[]) {
	inicializar_estructuras();
	config = cargar_config_planificador(argv[1]);


	config.algoritmo = sjf_sd;


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
	coord = coordinador;

	while (1) {
		if(planificar)
		{
			int n = fdmax + 1;

			s_wait(&fds_disponibles);
			read_fds = master;
			if (select(n, &read_fds, NULL, NULL, NULL) == -1)
				exit(1);
			s_signal(&fds_disponibles);

			for (int i = listener; i < n; i++)
				if (FD_ISSET(i, &read_fds)) {
					recibir_mensajes(i, listener, coordinador);
				}
		}
	}
}

int conectar_con_coordinador(int listener) { //fijarse bien esto, conectar siempre primero el coordinador
	int socket_coord = aceptar_nueva_conexion(listener);
	printf(MAGENTA "\nCOORDINADOR " RESET "conectado en socket " GREEN "%d\n" RESET, socket_coord);
	return socket_coord;
}

void recibir_mensajes(int socket, int listener, int socket_coordinador) {
	if (socket == listener)
		aceptar_nueva_conexion(listener);
	else if (socket == socket_coordinador && hay_hilo_coordinador == 0) {
		hay_hilo_coordinador = 1;
		FD_CLR(socket, &master);
		crear_hilo(socket_coordinador, coordinador);
	}
	else {
		s_wait(&mutex_coord_con);
		if (coordinador_conectado == 1) {
			s_signal(&mutex_coord_con);
			FD_CLR(socket, &master);
			crear_hilo(socket, esi);
		}
		s_signal(&mutex_coord_con);

	}
}


void* procesar_mensaje_coordinador(void* sock) {

	int coordinador = (int)sock;

	void* mensaje;
	Accion accion;

	while(1) {
		accion = recibirMensaje(coordinador, &mensaje);
		s_wait(&coord_ok);

		switch(accion) {
				case conectar_coord_planif:
					s_wait(&mutex_coord_con);
					coordinador_conectado = 1;
					s_signal(&mutex_coord_con);
					break;
				case sentencia_coordinador: {
					t_sentencia sentencia = *(t_sentencia*)mensaje;
					esperar_que_exista(sentencia.id_esi);
					nueva_sentencia(sentencia, coordinador);
					break;
				}
				case terminar_esi:
					finalizar_esi(*(int*)mensaje);
					break;
				default:
					break;
			}
		if (accion == error) {
			printf(RED"\n\nEL COORDINADOR SE HA DESCONECTADO"RESET);
			break;
		}
		s_signal(&coord_ok);
	}
	free(mensaje);
	return NULL;
}

void esperar_que_exista(int id_esi) { //para que el coordi no me meta una sentencia sin que esté cargado ese esi
	ESI* esi;
	while(true) {
		esi = obtener_esi(id_esi);
		if (esi != NULL) {
			if (esi->id == id_esi)
				break;
		}
	}
}

int ver_disponibilidad_clave(char* clave) { //0: no, 1: disponible
	if (esta_bloqueada(clave))
		return 0;
	return 1;
}

void* procesar_mensaje_esi(void* sock) {
	int socket = (int)sock;
	void* mensaje;
	while(1) {
		Accion accion = recibirMensaje(socket, &mensaje);
		switch(accion) {
			case nuevo_esi:
				proceso_nuevo(*(int*)mensaje, socket);
				break;
			case resultado_ejecucion:
				procesar_resultado(*(int*)mensaje);
				break;
			default:
				break;
		}
		if (accion == error) {
			printf(RED"\n\nEL ESI EN SOCKET %d SE HA DESCONECTADO"RESET, socket);
			break;
		}
	}
	free(mensaje);
	return NULL;
}

void crear_hilo(int nuevo_socket, Modulo modulo) {
	pthread_attr_t attr;
	pthread_t hilo;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (modulo == esi)
		pthread_create (&hilo ,&attr, procesar_mensaje_esi, (void*)nuevo_socket);
	else
		pthread_create (&hilo ,&attr, procesar_mensaje_coordinador , (void *)nuevo_socket);

	pthread_attr_destroy(&attr);
}

void procesar_resultado(ResultadoEjecucion resultado) {

	switch (resultado) {
	case no_exitoso:
		s_wait(&ejecucion);
		printf(RED "\nEl esi %d no pudo ejecutar la sentencia.\n" RESET, esi_ejecutando->id);
		s_signal(&ejecucion);
		break;
	case exitoso:
		s_wait(&ejecucion);
		printf(GREEN "\nEl esi %d pudo ejecutar la sentencia.\n" RESET, esi_ejecutando->id);
		s_signal(&ejecucion);
		break;
	case dudoso:
		break;
	}
	replanificar();
}

void ejecutar_esi(ESI* esi) {
	int id = esi->id;
	enviarMensaje(esi->socket_p, ejecutar_proxima_sentencia, &id, sizeof(int));

	esi->cola_actual = NULL;
	esi->posicion = 0;

	s_wait(&ejecucion);
	esi_ejecutando = esi;
	s_signal(&ejecucion);

}

void nueva_sentencia(t_sentencia sentencia, int coordinador) {
	s_wait(&siguiente_sentencia);
	ESI* esi = obtener_esi(sentencia.id_esi);
	switch (sentencia.tipo) {
	case S_GET:
		GET(sentencia.clave, esi, coordinador);
		break;
	case S_SET:
		SET(sentencia.clave, sentencia.valor, esi, coordinador);
		break;
	case S_STORE:
		STORE(sentencia.clave, esi, coordinador);
		break;
	}
	s_signal(&siguiente_sentencia);
}

ESI* obtener_esi(int id) { //al menos una vez voy a tener que filtrar en todo el planificador

	s_wait(&lista_disponible);
	int n = list_size(cola_de_listos);
	s_signal(&lista_disponible);

	ESI* esi;
	for (int i = 0; i < n; i++){
		s_wait(&lista_disponible);
		esi = list_get(cola_de_listos, i);
		s_signal(&lista_disponible);
		if (esi->id == id)
			return esi;
	}

	s_wait(&ejecucion);
	if (esi_ejecutando != NULL) {
		if (esi_ejecutando->id == id) {
			s_signal(&ejecucion);
			return esi_ejecutando;
		}
	}
	s_signal(&ejecucion);

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
	printf(RED "%s" YELLOW "%s "RESET "por parte del" GREEN " ESI %d" RESET, mensaje_error(tipo), clave, esi);
}

void GET(char* clave, ESI* esi, int coordinador) {
	if (esta_bloqueada(clave) == 1) {
		printf(YELLOW"\nLa clave "GREEN"%s "YELLOW"se encuentra bloqueada, se bloqueará el "GREEN"ESI %d"RESET, clave, esi->id);
		nueva_solicitud_clave(clave, esi);
		bloquear_esi(esi);
		avisar(coordinador, esi_bloqueado);
	}
	else {
		bloquear_clave(clave, esi);
		printf("\nLa clave "GREEN"%s "RESET"se asigno a "GREEN"ESI %d"RESET, clave, esi->id);
		avisar(coordinador, sentencia_coordinador);
	}
}

void SET(char* clave, char* valor, ESI* esi, int coordinador) {
	t_clave* c = buscar_clave_bloqueada(clave);
	if (c == NULL) {
		error_operacion(error_clave_no_identificada, clave, esi->id);
		finalizar_esi_ref(esi);
		avisar(coordinador, error_sentencia);
	}
	else if (c->bloqueada == 0 || c->esi_duenio != esi) {
		error_operacion(error_clave_no_bloqueada, clave, esi->id);
		finalizar_esi_ref(esi);
		avisar(coordinador, error_sentencia);
	}
	else { //puedo hacer store
		strcpy(c->valor, valor);
		printf("\nSe ha seteado la clave" GREEN " %s" RESET " con valor" MAGENTA " %s" RESET, c->clave, c->valor);
		avisar(coordinador, sentencia_coordinador);
	}
}

void STORE(char* clave, ESI* esi, int coordinador) { //esi: esi que hace el pedido de store
	t_clave* c = buscar_clave_bloqueada(clave);
	if (c == NULL) {
		error_operacion(error_clave_no_identificada, clave, esi->id);
		finalizar_esi_ref(esi);
		avisar(coordinador, error_sentencia);
	}
	else if (c->bloqueada == 0 || c->esi_duenio != esi) {
		error_operacion(error_clave_no_bloqueada, clave, esi->id);
		finalizar_esi_ref(esi);
		avisar(coordinador, error_sentencia);
	}
	else { //puedo hacer store
		liberar_clave(clave);
		printf("\nSe ha liberado la clave" GREEN " %s" RESET, c->clave);
		avisar(coordinador, sentencia_coordinador);
	}
}

void finalizar_esi_ref(ESI* esi) {
	printf(YELLOW"\nFinalizando ESI %d"RESET, esi->id);
	mover_esi(esi, cola_de_finalizados);
	s_wait(&ejecucion);
	esi_ejecutando = NULL;
	s_signal(&ejecucion);
	//liberar_recursos(esi);
	replanificar();
}

void finalizar_esi(int id_esi) {
	ESI* esi = obtener_esi(id_esi);
	finalizar_esi_ref(esi);
}

void liberar_recursos(ESI* esi) {
	int n_claves = list_size(esi->claves);
	for (int i = 0; i < n_claves; i++)
		liberar_clave(list_get(esi->claves, i));
}

void bloquear_clave(char* clave, ESI* esi) {
	t_clave* c = buscar_clave_bloqueada(clave);
	if (c == NULL) {
		t_clave* nueva_entrada = malloc(sizeof(t_clave));
		nueva_entrada->bloqueada = 1;
		strcpy(nueva_entrada->clave, clave);
		nueva_entrada->esi_duenio = esi;
		nueva_entrada->esis_esperando = list_create();
		list_add(lista_claves_bloqueadas, nueva_entrada);
	} else {
		c->bloqueada = 1;
		c->esi_duenio = esi;
	}
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
		if (esi_a_desbloquear != NULL)
			desbloquear_esi(esi_a_desbloquear);
	}
}

int esta_bloqueada(char* clave) {
	t_clave* bloq = buscar_clave_bloqueada(clave);
	if (bloq != NULL)
		return bloq->bloqueada;
	return 0;
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
	s_wait(&numero_esi);
	nuevo_esi->id = ++ultimo_id;
	s_signal(&numero_esi);
	nuevo_esi->cant_rafagas = rafagas;
	nuevo_esi->rafaga_anterior = rafagas; //VER ESTOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
	nuevo_esi->socket_p = socket;
	nuevo_esi->socket_c = 0;
	nuevo_esi->cola_actual = NULL;
	nuevo_esi->claves = list_create();
	printf( "\nNuevo ESI con" GREEN " %d rafagas " RESET, nuevo_esi->cant_rafagas);
	printf("(ID: %d, ESTIMACION: %d)\n", nuevo_esi->id, nuevo_esi->estimacion_anterior);
	ingreso_cola_de_listos(nuevo_esi);
}



void ingreso_cola_de_listos(ESI* esi) {
	mover_esi(esi, cola_de_listos);
	s_wait(&num_esis);
	if (hay_desalojo(config.algoritmo) || esi_ejecutando == NULL)
		replanificar();
	s_signal(&num_esis);
}

void replanificar() {
	ejecutar(config.algoritmo);
}

int hay_desalojo(AlgoritmoPlanif algoritmo) {
	if (algoritmo == sjf_cd || algoritmo == hrrn || algoritmo == rr || algoritmo == vrr)
		return 1;
	return 0;
}

void ejecutar(AlgoritmoPlanif algoritmo) {

	s_wait(&siguiente_sentencia);

	switch(algoritmo) {
	case fifo:
		ejecutar_por_fifo();
		break;
	case sjf_sd:
	case sjf_cd:
		ejecutar_por_sjf();
		break;
	case hrrn:
	case rr:
	case vrr:
		break;
	}

	s_signal(&siguiente_sentencia);
}

int _es_esi(ESI* a, ESI* b) {
	return a->id == b->id;
}

void mover_esi(ESI* esi, t_list* nueva_lista) {
	//s_wait(&lista_disponible);
	if (esi->cola_actual != NULL)
		list_remove(esi->cola_actual, esi->posicion);
	list_add(nueva_lista, esi);
	esi->cola_actual = nueva_lista;
	esi->posicion = list_size(nueva_lista) - 1;

//	s_signal(&lista_disponible);

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

ESI* primero_llegado() {
	int n = list_size(cola_de_listos);
	//s_wait(&lista_disponible);
	for (int i = 0; i < n; i++) {
		ESI* esi = list_get(cola_de_listos, i);
		if (esi != NULL) {
			//s_signal(&lista_disponible);
			esi->cola_actual = NULL;
			return list_remove(cola_de_listos, i);
		}
	}
	return NULL;
}

void ejecutar_por_fifo() {
	ESI* esi;
	s_wait(&ejecucion);
	if(esi_ejecutando == NULL)
		esi = primero_llegado();
	else
		esi = esi_ejecutando;
	s_signal(&ejecucion);
	if (esi != NULL)
		ejecutar_esi(esi);
}

void ejecutar_por_sjf() {
	ESI* esi;
	if (esi_ejecutando == NULL || hay_desalojo(config.algoritmo))
		 esi = esi_rafaga_mas_corta();
	else
		esi = esi_ejecutando;
	if (esi != NULL)
		ejecutar_esi(esi);
}

void sentencia_ejecutada() {
	//evento
}

float estimar(ESI* esi) {
	float a = config.alfa_planif / 100.0;
	int raf = esi->rafaga_anterior;
	int est = esi->estimacion_anterior;

	return a * raf + (1 - a) * est;
} //alfa entre 0 y 100


ESI* esi_rafaga_mas_corta() {
	int cant_esis = list_size(cola_de_listos); //antes hacia un sort de lista, esto es mas performante (??
	int est_a, est_b;
	ESI *a, *b;
	if (cant_esis >= 2){
		for (int i = 0; i < cant_esis; i++) {

				a = list_get(cola_de_listos, i);
				est_a = estimar(a);

				for (int j = 0; j < cant_esis; j++) {

					b = list_get(cola_de_listos, j);
					est_b = estimar(b);

					if (est_a < est_b)
						return list_remove(cola_de_listos, i);
					else if (a->id != b->id)
						return list_remove(cola_de_listos, j);
				}
			}
	}
	return list_remove(cola_de_listos, 0);
}

ESI* esi_resp_ratio_mas_corto() {
	return NULL;
}
