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
#include "../../Bibliotecas/src/Semaforo.c"

ConfigPlanificador config;
t_list* cola_de_listos;
t_list* cola_de_bloqueados;
t_list* cola_de_finalizados;
t_list* lista_claves_bloqueadas;
ESI* esi_ejecutando = NULL;
t_dictionary* estimaciones_actuales;
int ultimo_id;
int planificar = 1;
int hayEsis = 0;
int coordinador_conectado = 0;

//semaforos
int lista_disponible = 1;
int siguiente_sentencia = 1;



int main(int argc, char* argv[]) {
	inicializar_estructuras();
	config = cargar_config_planificador(argv[1]);
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

	while (1) {
		if(planificar)
		{
			int n = fdmax + 1;
			read_fds = master;
			if (select(n, &read_fds, NULL, NULL, NULL) == -1) {
				perror("select");
				exit(1);
			}
			for (int i = listener; i < n; i++)
				if (FD_ISSET(i, &read_fds))
					recibir_mensajes(i, listener, coordinador);
		}
	}
}

int hay_hilo_coordinador = 0;

int conectar_con_coordinador(int listener) { //fijarse bien esto, conectar siempre primero el coordinador
	int socket_coord = aceptar_nueva_conexion(listener);
	printf(MAGENTA "\nCOORDINADOR " RESET "conectado en socket " GREEN "%d" RESET, socket_coord);
	return socket_coord;
}

void recibir_mensajes(int socket, int listener, int socket_coordinador) {
	if (socket == listener)
		aceptar_nueva_conexion(listener);
	else if (socket == socket_coordinador && hay_hilo_coordinador == 0) {
		hay_hilo_coordinador = 1;
		crear_hilo(socket_coordinador, coordinador);
	}
	else {
		if (coordinador_conectado == 1)
			crear_hilo(socket, esi);
	}

}

void* procesar_mensaje_coordinador(void* sock) {

	int coordinador = (int)sock;
	void* mensaje;
	printf("\nESTOY RE DURO\n");
	Accion accion;
	while((accion = recibirMensaje(coordinador, &mensaje)) != cerrar_conexion_coord) {
		printf("\nentre al while\n");
		switch(accion) {
				case conectar_coord_planif:
					printf("\nfruta\n");
					coordinador_conectado = 1;
					break;
				case sentencia_coordinador:
					printf("\nIVAN SALVANOS\n");
					nueva_sentencia(*(t_sentencia*)mensaje, coordinador);
					break;
				case preguntar_recursos_planificador: {
					int respuesta = ver_disponibilidad_clave((char*)mensaje);
					enviarMensaje(coordinador, recurso_disponible, &respuesta, sizeof(respuesta));
					break;
				}
				case terminar_esi:
					finalizar_esi((ESI*)mensaje);
					break;
				default:
					break;
			}
		//accion = 0;
	}
	printf("\nACCION :%d \nCerrando conexion con el coordinador\n",accion);
	close(coordinador);
	FD_CLR(coordinador, &master);

	hay_hilo_coordinador = 0;
	return NULL;
}

int ver_disponibilidad_clave(char* clave) { //0: no, 1: disponible
	if (esta_bloqueada(clave))
		return 0;
	return 1;
}

void* procesar_mensaje_esi(void* sock) {
	int socket = (int)sock;
	void* mensaje;
	Accion accion;
	while((accion = recibirMensaje(socket, &mensaje)) != cerrar_conexion_esi) {

		switch(accion) {
			case nuevo_esi:
				proceso_nuevo(*(int*)mensaje, socket);
				break;
			case resultado_ejecucion:
				procesar_resultado(*(int*)mensaje);
				break;
			default:
				FD_CLR(socket, &master);
				break;
		}
	}
	printf("\nACCION :%d \nCerrando conexion con el esi\n", accion);
	close(socket);
	FD_CLR(socket, &master);
	return NULL;
}

void crear_hilo(int nuevo_socket, Modulo modulo) {
	pthread_attr_t attr;
	pthread_t hilo;
	int  res = pthread_attr_init(&attr);
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (modulo == esi) {
		res = pthread_create (&hilo ,&attr, procesar_mensaje_esi, (void*)nuevo_socket);
	}
	else if (modulo == coordinador) {
		pthread_create (&hilo ,&attr, procesar_mensaje_coordinador , (void *)nuevo_socket);
	}

	pthread_attr_destroy(&attr);
}

void procesar_resultado(ResultadoEjecucion resultado) {
	switch (resultado) {
	case no_exitoso:
		printf(RED "\nEl esi %d termino su ejecucion de forma no existosa.\n" RESET, esi_ejecutando->id);
		break;
	case exitoso:
		printf(GREEN "\nEl esi %d termino su ejecucion de forma exitosa.\n" RESET, esi_ejecutando->id);
		break;
	case dudoso:
		break;
	}
	finalizar_esi(esi_ejecutando);
}

void ejecutar_esi(ESI* esi) {



	printf("\nestoy ejecutando el esi %d", esi->id);
	int id = esi->id;
	printf("\nmandando para q ejecute el esi %d", id);
	enviarMensaje(esi->socket_p, ejecutar_proxima_sentencia, &id, sizeof(int));
	printf("\nmoriré\n");
	//aca muere
	esi_ejecutando = (ESI*)esi;

}

void nueva_sentencia(t_sentencia sentencia, int coordinador) {

	printf("sentencia de esi %d recibida", sentencia.id_esi);

	s_wait(&siguiente_sentencia);
	ESI* esi = obtener_esi(sentencia.id_esi);
	printf("obtenido esi %d", esi->id);
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
	ESI* esi;
	for (int i = 0; i < n; i++){
		esi = list_get(cola_de_listos, i);
		if (esi->id == id)
			return esi;
	}
	s_signal(&lista_disponible);

	if (esi_ejecutando != NULL)
		if (esi_ejecutando->id == id)
			return esi_ejecutando;

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

void GET(char* clave, ESI* esi, int coordinador) {
	printf("evaluando GET");

	if (esta_bloqueada(clave) == 1) {
		printf(YELLOW"\nLa clave "GREEN"%s "YELLOW"se encuentra bloqueada, se bloqueará el "GREEN"ESI %d"RESET, clave, esi->id);
		nueva_solicitud_clave(clave, esi);
		bloquear_esi(esi);
		printf("pepito");
		avisar(coordinador, esi_bloqueado);
	}
	else {
		bloquear_clave(clave, esi);
		printf("%d", esi->id);
		printf("\nLa clave "GREEN"%s "RESET"se asigno a "GREEN"ESI %d"RESET, clave, esi->id);
		avisar(coordinador, sentencia_coordinador);
	}
}

void SET(char* clave, char* valor, ESI* esi, int coordinador) {
	t_clave* c = buscar_clave_bloqueada(clave);
	if (c == NULL) {
		error_operacion(error_clave_no_identificada, clave, esi->id);
		finalizar_esi(esi);
		avisar(coordinador, error_sentencia);
	}
	else if (!c->bloqueada && c->esi_duenio != esi) {
		error_operacion(error_clave_no_bloqueada, clave, esi->id);
		finalizar_esi(esi);
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
		finalizar_esi(esi);
		avisar(coordinador, error_sentencia);
	}
	else if (!c->bloqueada && c->esi_duenio != esi) {
		error_operacion(error_clave_no_bloqueada, clave, esi->id);
		finalizar_esi(esi);
		avisar(coordinador, error_sentencia);
	}
	else { //puedo hacer store
		liberar_clave(clave);
		printf("\nSe ha liberado la clave" GREEN " %s" RESET, c->clave);
		avisar(coordinador, sentencia_coordinador);
	}
}

void finalizar_esi(ESI* esi) {
	mover_esi(esi, cola_de_finalizados);
	liberar_recursos(esi);
	if (hay_desalojo(config.algoritmo)) //llega uno a listo, y hay desalojo -> ver
		replanificar();
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
	printf("\nbuscando clave bloqueada");
	t_clave* bloq = buscar_clave_bloqueada(clave);
	if (bloq == NULL) {
		printf("voy a retornar 0");
		return 0;
	}
	printf("voy a retornar 1");
	return 1;
}

t_clave* buscar_clave_bloqueada(char* clave) {
	t_list* lista = lista_claves_bloqueadas;
	printf("\nahora entro al for\n");
	for (int i = 0; i < list_size(lista); i++) {
		t_clave* c = list_get(lista, i);
		printf("\npepito\n");
		if (strcmp(clave, c->clave) == 0)
			return c;
	}
	printf("\nvoy a retornar null\n");
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
	nuevo_esi->socket_c = 0;
	nuevo_esi->cola_actual = NULL;
	ingreso_cola_de_listos(nuevo_esi);
	printf( "\nNuevo ESI con" GREEN " %d rafagas " RESET, nuevo_esi->cant_rafagas);
	printf("(ID: %d, ESTIMACION: %d)\n", nuevo_esi->id, nuevo_esi->estimacion_anterior);
}

void ingreso_cola_de_listos(ESI* esi) {
	mover_esi(esi, cola_de_listos);
	if (hay_desalojo(config.algoritmo) || hayEsis == 0) {
		hayEsis = 1;
		replanificar();
	}
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

	s_wait(&lista_disponible);

	if (esi->cola_actual != NULL)
		list_remove(esi->cola_actual, esi->posicion);
	list_add(nueva_lista, esi);
	esi->cola_actual = nueva_lista;
	esi->posicion = list_size(nueva_lista) - 1;

	s_signal(&lista_disponible);

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
	printf("voy a jecutar el esi %d", esi->id);
	//mover_esi(esi, cola_de_listos);
	ejecutar_esi(esi);
}

void ejecutar_por_sjf() {
	ESI* esi = esi_rafaga_mas_corta();
	mover_esi(esi, cola_de_listos);
	ejecutar_esi(esi);
}

void sentencia_ejecutada() {
	//evento
}

float estimar(ESI* esi) {
	return (config.alfa_planif/100) * esi->rafaga_anterior + (1 - (config.alfa_planif/100)) * esi->estimacion_anterior;
} //alfa entre 0 y 100


ESI* esi_rafaga_mas_corta() {
	int cant_esis = list_size(cola_de_listos); //antes hacia un sort de lista, esto es mas performante (??
	for (int i = 0; i < cant_esis; i++) {
		ESI* a = list_get(cola_de_listos, i);
		for (int j = 0; j < cant_esis; j++) {
			ESI* b = list_get(cola_de_listos, j);
			if (estimar(a) < estimar(b)	&& a != esi_ejecutando) //si se estaba ejecutando no puedo devolver el mismo
				return a;
		}
	}
	return NULL;
}

ESI* esi_resp_ratio_mas_corto() {
	return NULL;
}
