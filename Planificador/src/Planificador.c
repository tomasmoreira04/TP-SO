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
int socket_coordinador;

FILE* output; //salida en la otra ventanita

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
pthread_mutex_t sem_ejecutar = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t operando_claves = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char* argv[]) {
	inicializar_estructuras();
	config = cargar_config_planificador(argv[1]);
	imprimir_configuracion();
	bloquear_claves_iniciales(config.claves_bloqueadas, config.n_claves);
	pthread_t thread_consola, thread_output;
	pthread_create(&thread_consola, NULL, iniciar_consola, NULL);
	pthread_create(&thread_output, NULL, crear_ventana_output, NULL);
	ultimo_id = 0;

	recibir_conexiones();
	pthread_join(thread_consola, NULL);
	pthread_join(thread_output, NULL);
	destruir_estructuras();
	fclose(output);
}

void* crear_ventana_output() {
	const char* script = "\
			#/bin/bash \n\
			xterm -e tail -f consola & \n\
			";
	system(script);
	return NULL;
}


void bloquear_claves_iniciales(char** claves, int n) {
	char** bloqueadas = (char**)malloc(sizeof(char*) * (n + 1));
	for (int i = 0; i < n; i++) {
		char* copia = claves[i];
		bloqueadas[i] = malloc(sizeof(char*) * strlen(claves[i]));
		strcpy(bloqueadas[i], copia);
	}
	fprintf(output, GREEN"\nClaves inicialmente bloqueadas:"RESET);
	for (int i = 0; i < n; i++) {
		bloquear_clave(bloqueadas[i], NULL);
		fprintf(output, RED"\n%s"RESET, bloqueadas[i]);
	}
}

void recibir_conexiones() {
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	int listener = crear_socket_de_escucha(config.puerto_escucha);
	FD_SET(listener, &master);

	socket_coordinador = conectar_con_coordinador(listener);

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
					recibir_mensajes(i, listener, socket_coordinador);
				}
		}
	}
}

int conectar_con_coordinador(int listener) { //fijarse bien esto, conectar siempre primero el coordinador
	int socket_coord = aceptar_nueva_conexion(listener);
	fprintf(output, MAGENTA "\nCOORDINADOR " RESET "conectado en socket " GREEN "%d\n" RESET, socket_coord);
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
				case clave_guardada_en_instancia: {
					t_clave c = *(t_clave*)mensaje;
					actualizar_clave(c);
					break;
				}
				case terminar_esi:
					finalizar_esi(*(int*)mensaje);
					break;
				default:
					break;
			}
		if (accion == error) {
			fprintf(output, RED"\n\nEL COORDINADOR SE HA DESCONECTADO"RESET);
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
				proceso_nuevo(*(t_nuevo_esi*)mensaje, socket);
				break;
			case resultado_ejecucion:
				procesar_resultado(*(int*)mensaje);
				break;
			default:
				break;
		}
		if (accion == error) {
			fprintf(output, RED"\n\nEL ESI EN SOCKET %d SE HA DESCONECTADO"RESET, socket);
			break;
		}
	}
	free(mensaje);
	return NULL;
}

void actualizar_clave(t_clave respuesta) { //guardo en qué instancia se guardo esa clave
	char* clave = respuesta.clave;
	char* instancia = respuesta.instancia;
	t_clave* registro = buscar_clave_bloqueada(clave);
	strcpy(registro->instancia, instancia);
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

	s_wait(&ejecucion);
	switch (resultado) {
	case no_exitoso:
		fprintf(output, RED "\nEl esi no pudo ejecutar la sentencia.\n" RESET);
		break;
	case exitoso:
		fprintf(output, GREEN "\nEl esi pudo ejecutar la sentencia.\n" RESET);
		break;
	case bloqueado:
		fprintf(output, YELLOW "\nEl esi se ha bloqueado.\n" RESET);
		break;
	case dudoso:
		break;
	}
	s_signal(&ejecucion);
	ejecutar(false);
}

void ejecutar_esi(ESI* esi) {
	int id = esi->id;
	enviarMensaje(esi->socket_planif, ejecutar_proxima_sentencia, &id, sizeof(int));

	esi->cola_actual = NULL;
	esi->posicion = 0;

	s_wait(&ejecucion);
	esi_ejecutando = esi;
	if (config.algoritmo == hrrn)
		sumar_tiempo_hrrn();
	s_signal(&ejecucion);

}

void sumar_tiempo_hrrn() {
	t_list* lista = cola_de_listos;
	for (int i = 0; i < list_size(lista); i++)
		((ESI*)list_get(lista, i))->tiempo_esperado++;
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

	ESI* esi;
	for (int i = 0; i < list_size(cola_de_listos); i++){
		s_wait(&lista_disponible);
		esi = list_get(cola_de_listos, i);
		s_signal(&lista_disponible);
		if (esi->id == id)
			return esi;
	}

	for (int i = 0; i < list_size(cola_de_bloqueados); i++){
		s_wait(&lista_disponible);
		esi = list_get(cola_de_bloqueados, i);
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
	fprintf(output, RED "%s" YELLOW "%s "RESET "por parte del" GREEN " ESI %d" RESET, mensaje_error(tipo), clave, esi);
}

void GET(char* clave, ESI* esi, int coordinador) {
	if (esta_bloqueada(clave) == 1) {
		fprintf(output, YELLOW"\n(GET) de clave"RED" %s"YELLOW", pero se encuentra bloqueada, se bloqueará el "GREEN"ESI %d."RESET, clave, esi->id);
		nueva_solicitud_clave(clave, esi); //va bien esto
		bloquear_esi(esi);
		avisar(coordinador, esi_bloqueado);
	}
	else {
		bloquear_clave(clave, esi);
		fprintf(output, "\n(GET) La clave "GREEN"%s "RESET"se asigno a "GREEN"ESI %d"RESET, clave, esi->id);
		esi->rafagas_restantes--;
		avisar(coordinador, sentencia_coordinador);
	}
}

void SET(char* clave, char* valor, ESI* esi, int coordinador) {
	t_clave* c = buscar_clave_bloqueada(clave); //aca adentro hay mutex

	if (c->bloqueada != 1 || c->esi_duenio->id != esi->id) {
		error_operacion(error_clave_no_bloqueada, clave, esi->id);
		finalizar_esi_ref(esi);
		avisar(coordinador, error_sentencia);
	}
	else if (c == NULL) {
		error_operacion(error_clave_no_identificada, clave, esi->id);
		finalizar_esi_ref(esi);
		avisar(coordinador, error_sentencia);
	}
	else { //puedo hacer set
		strcpy(c->valor, valor);
		fprintf(output, "\n(SET) Se ha seteado la clave" GREEN " %s" RESET " con valor" MAGENTA " %s" RESET, c->clave, c->valor);
		esi->rafagas_restantes--;
		avisar(coordinador, sentencia_coordinador);
	}

}

void STORE(char* clave, ESI* esi, int coordinador) { //esi: esi que hace el pedido de store
	t_clave* c = buscar_clave_bloqueada(clave); //aca adentro hay mutex

	if (c->bloqueada != 1 || c->esi_duenio->id != esi->id) {
		error_operacion(error_clave_no_bloqueada, clave, esi->id);
		finalizar_esi_ref(esi);
		avisar(coordinador, error_sentencia);
	}
	else if (c == NULL) {
		error_operacion(error_clave_no_identificada, clave, esi->id);
		finalizar_esi_ref(esi);
		avisar(coordinador, error_sentencia);
	}
	else { //puedo hacer store
		liberar_clave(clave);
		fprintf(output, "\n(STORE) Se ha liberado la clave" GREEN " %s" RESET, c->clave);
		esi->rafagas_restantes--;
		avisar(coordinador, sentencia_coordinador);
	}
}

void finalizar_esi_ref(ESI* esi) {
	fprintf(output, YELLOW"\nFinalizando ESI %d."RESET, esi->id);

	if (config.algoritmo == sjf_sd || config.algoritmo == sjf_cd) {
		if (esi->rafagas_restantes > 0)
			fprintf(output, "\nSobraron" RED " %.2f " RESET "UT de la estimacion.", esi->rafagas_restantes);
		else if (esi->rafagas_restantes == 0)
			fprintf(output, "\nNo sobraron UT de la estimacion.");
		else fprintf(output, "\nFaltaron" RED " %.2f " RESET "UT en la estimacion.", esi->rafagas_restantes * -1);
	}

	mover_esi(esi, cola_de_finalizados);
	s_wait(&ejecucion);
	esi_ejecutando = NULL;
	s_signal(&ejecucion);
	liberar_recursos(esi);
	replanificar();
}

void finalizar_esi(int id_esi) {
	ESI* esi = obtener_esi(id_esi);
	finalizar_esi_ref(esi);
}

void liberar_recursos(ESI* esi) {
	t_list* claves = claves_de_esi(esi);
	for (int i = 0; i < list_size(claves); i++) {
		char* clave = list_get(claves, i);
		liberar_clave(clave);
	}
}

t_list* claves_de_esi(ESI* esi) {
	t_list* claves = list_create();
	for (int i = 0; i < list_size(lista_claves_bloqueadas); i++) {
		t_clave* clave = list_get(lista_claves_bloqueadas, i);
		if(clave!=NULL && clave->esi_duenio!=NULL){


			if (clave->esi_duenio->id == esi->id)
				list_add(claves, clave->clave);
		}
	}
	return claves;
}

void bloquear_clave(const char* clave, ESI* esi) {
	t_clave* entrada = buscar_clave_bloqueada(clave);
	s_wait(&operando_claves);
	if (entrada == NULL) { //no existe -> la creo
		entrada = malloc(sizeof(t_clave));
		entrada->bloqueada = 1;
		entrada->esi_duenio = esi;
		strcpy(entrada->instancia, "No asignada");
		entrada->esis_esperando = list_create();
		strcpy(entrada->clave, clave);
		strcpy(entrada->valor, "");
		list_add(lista_claves_bloqueadas, entrada);
	} else {
		entrada->bloqueada = 1;
		entrada->esi_duenio = esi;
	}
	s_signal(&operando_claves);
}

void nueva_solicitud_clave(char* clave, ESI* esi) {
	t_clave* entrada = buscar_clave_bloqueada(clave);
	if (entrada != NULL) {
		list_add(entrada->esis_esperando, esi);
		fprintf(output, GREEN"\nESI %d"RESET" agregado a la lista de espera de la clave"MAGENTA" %s"RESET, esi->id, clave);
	}
	else bloquear_clave(clave, esi);
}



int liberar_clave(char* clave) {
	t_clave* c = buscar_clave_bloqueada(clave);
	s_wait(&operando_claves);
	if (c != NULL) {
		ESI* esi_a_desbloquear = list_remove(c->esis_esperando, 0); //el primero en haberse bloqueado, y lo saco
		if (esi_a_desbloquear != NULL) {
			desbloquear_esi(esi_a_desbloquear);
			c->esi_duenio = esi_a_desbloquear;
			c->bloqueada = 1; //ahora esta bloqueada por el esi q taba esprando
			fprintf(output, "\nClave"RED" %s "RESET"liberada. Ahora pertenece a ESI %d", clave, esi_a_desbloquear->id);
		}
		else {
			c->bloqueada = 0; //queda libre en el aire
			c->esi_duenio = NULL;
			fprintf(output, "\nClave"RED" %s "RESET"desbloqueada y libre.", clave);
		}
		s_signal(&operando_claves);
		return 1;
	}
	s_signal(&operando_claves);
	return 0; //error
}

int esta_bloqueada(char* clave) {
	t_clave* bloq = buscar_clave_bloqueada(clave);
	if (bloq != NULL)
		return bloq->bloqueada == 1;
	return 0;
}

t_clave* buscar_clave_bloqueada(const char* clave) {
	s_wait(&operando_claves);
	for (int i = 0; i < list_size(lista_claves_bloqueadas); i++) {
		t_clave* c = list_get(lista_claves_bloqueadas, i);
		if (strcmp(clave, c->clave) == 0) {
			s_signal(&operando_claves);
			return c;
		}
	}
	s_signal(&operando_claves);
	return NULL;
}

void bloquear_esi(ESI* esi) {
	mover_esi(esi, cola_de_bloqueados);
	esi_ejecutando = NULL;
	replanificar();
}

void desbloquear_esi(ESI* esi) {
	ingreso_cola_de_listos(esi);
	fprintf(output, GREEN"\nSE HA DESBLOQUEADO EL ESI %d\n"RESET, esi->id);
}

void proceso_nuevo(t_nuevo_esi esi, int socket) {
	ESI* nuevo_esi = malloc(sizeof(ESI));
	nuevo_esi->estimacion_anterior = config.estimacion_inicial;
	s_wait(&numero_esi);
	nuevo_esi->id = ++ultimo_id;
	s_signal(&numero_esi);
	nuevo_esi->rafagas_totales = esi.rafagas;
	nuevo_esi->rafagas_restantes = estimar(nuevo_esi);
	nuevo_esi->tiempo_esperado = 0;
	nuevo_esi->socket_planif = socket;
	nuevo_esi->cola_actual = NULL;
	nuevo_esi->claves = list_create();
	imprimir_nuevo_esi(nuevo_esi, esi.nombre);
	ingreso_cola_de_listos(nuevo_esi);
}

void imprimir_nuevo_esi(ESI* esi, char* nombre) {
	fprintf(output, "\nNuevo ESI " GREEN "%d" RESET " con ", esi->id);
	if (config.algoritmo == fifo || config.algoritmo == hrrn)
		fprintf(output, RED"%d"RESET" de rafagas totales.", esi->rafagas_totales);
	else //sjf
		fprintf(output, RED"%.2f"RESET" de tiempo estimado.", esi->rafagas_restantes);
	fprintf(output, " Script: "CYAN"%s"RESET, nombre);
}


void ingreso_cola_de_listos(ESI* esi) {
	mover_esi(esi, cola_de_listos);
	replanificar();
}

void replanificar() {
	ejecutar(true);
}

int hay_desalojo(AlgoritmoPlanif algoritmo) {
	if (algoritmo == sjf_cd)
		return 1;
	return 0;
}

void ejecutar(int desalojar) {

	while (planificar != 1);

	s_wait(&sem_ejecutar);

	switch(config.algoritmo) {
	case fifo:
		ejecutar_por_fifo();
		break;
	case sjf_sd:
		ejecutar_por_sjf(false);
		break;
	case sjf_cd:
		ejecutar_por_sjf(desalojar);
		break;
	case hrrn:
		ejecutar_por_hrrn();
		break;
	}

	s_signal(&sem_ejecutar);
}

int _es_esi(ESI* a, ESI* b) {
	return a->id == b->id;
}

void mover_esi(ESI* esi, t_list* nueva_lista) {

	s_wait(&lista_disponible);

	if (esi != NULL) {
		if (esi->cola_actual != NULL)
			list_remove(esi->cola_actual, esi->posicion);
		list_add(nueva_lista, esi);
		esi->cola_actual = nueva_lista;
		esi->posicion = list_size(nueva_lista) - 1;
	}

	s_signal(&lista_disponible);

}

void inicializar_estructuras() {
	output = fopen("consola", "w");
	setbuf(stdout, NULL);
	setbuf(output, NULL);
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

void ejecutar_por_sjf(int desalojar) {
	ESI* esi;
	s_wait(&ejecucion);
	if (esi_ejecutando == NULL || desalojar) {
		 esi = esi_rafaga_mas_corta();
		 if (esi_ejecutando != NULL && esi->id != esi_ejecutando->id)
		 	mover_esi(esi_ejecutando, cola_de_listos);
	}
	else
		esi = esi_ejecutando;
	s_signal(&ejecucion);
	if (esi != NULL)
		ejecutar_esi(esi);
}

void ejecutar_por_hrrn() {
	ESI* esi;
	s_wait(&ejecucion);
	if (esi_ejecutando == NULL) {
		s_wait(&lista_disponible); //cambiar a otro semaforo para cola de listos

		calcular_response_ratios();
		esi = esi_resp_ratio_mas_alto();

		s_signal(&lista_disponible); //cambiar a otro semaforo para cola de listos
	}
	else
		esi = esi_ejecutando;
	s_signal(&ejecucion);
	if (esi != NULL)
		ejecutar_esi(esi);
}



float estimar(ESI* esi) {
	float a = config.alfa_planif / 100.0;
	int raf = esi->rafagas_totales;
	int est = esi->estimacion_anterior;

	return a * raf + (1 - a) * est;
}

float response_ratio(ESI* esi) {
	int raf = esi->rafagas_totales;
	int esp = esi->tiempo_esperado;
	return (float)(raf + esp)/raf;
}

void calcular_response_ratios() {
	fprintf(output, GREEN"\nCalculando response ratios...\n"RESET);
	for (int i = 0; i < list_size(cola_de_listos); i++) {
		ESI* esi = list_get(cola_de_listos, i);
		esi->response_ratio = response_ratio(esi);
		fprintf(output, GREEN"\nESI %d: "RED"%f", esi->id, esi->response_ratio);
	}
}

ESI* esi_resp_ratio_mas_alto() {
	ESI* mas_alto;
	ESI* otro_esi;
	int indice = 0;
	int cant = list_size(cola_de_listos);
	if (cant != 0) {
		mas_alto = list_get(cola_de_listos, 0);
		for (int i = 1; i < cant; i++) {
			otro_esi = list_get(cola_de_listos, i);
			if (otro_esi->response_ratio > mas_alto->response_ratio) {
				mas_alto = otro_esi;
				indice = i;
			}
		}
	}
	else {
		fprintf(output, RED"\nNo hay esis listos para calcular RR\n"RESET);
		return NULL;
	}
	fprintf(output, GREEN"\nESI elegido -> "RED"%d\n"RESET, mas_alto->id);
	return list_remove(cola_de_listos, indice);
}

ESI* esi_rafaga_mas_corta() {
	ESI* mas_corto = list_get(cola_de_listos, 0);
	if (esi_ejecutando != NULL)
		mas_corto = esi_ejecutando;
	int indice = 0;

	for (int i = 0; i < list_size(cola_de_listos); i++) {
		ESI* esi = list_get(cola_de_listos, i);
		if (esi != NULL) {
			if (esi->rafagas_restantes < mas_corto->rafagas_restantes) {
				mas_corto = esi;
				indice = i;
			}
		}
	}
	if (esi_ejecutando != NULL)
		if (mas_corto->id == esi_ejecutando->id)
			return mas_corto;
	return list_remove(cola_de_listos, indice);
}

ESI* primero_llegado() {
	int n = list_size(cola_de_listos);
	for (int i = 0; i < n; i++) {
		ESI* esi = list_get(cola_de_listos, i);
		if (esi != NULL) {
			esi->cola_actual = NULL;
			return list_remove(cola_de_listos, i);
		}
	}
	return NULL;
}

void imprimir_configuracion() {
	printf(CYAN"\nPuerto de escucha: "YELLOW"%d"RESET, config.puerto_escucha);
	printf(CYAN"\nCoordinador: "YELLOW"%s : %d"RESET, config.ip_coordinador, config.puerto_coordinador);
	printf(GREEN"\n\nConfiguracion cargada con exito:"RESET);
	printf(GREEN"\nAlgoritmo: "RED"%s", algoritmo(config.algoritmo));
	printf(GREEN"\nAlfa: "RED"%d", config.alfa_planif);
	printf(GREEN"\nEstimacion inicial: "RED"%f\n"RESET, config.estimacion_inicial);
}

char* algoritmo(AlgoritmoPlanif alg) {
	switch (alg) {
		case sjf_sd: 	return "SJF-SD";
		case sjf_cd: 	return "SJD-CD";
		case hrrn: 		return "HRRN";
		default: 		return "FIFO";
	}
}

