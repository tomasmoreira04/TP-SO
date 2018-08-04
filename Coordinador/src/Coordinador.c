#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../Bibliotecas/src/Socket.c"
#include "../../Bibliotecas/src/Configuracion.c"
#include "../../Bibliotecas/src/Estructuras.h"
#include "../../Bibliotecas/src/Color.h"
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include "commons/string.h"
#include "Coordinador.h"
#include "commons/collections/list.h"
#include <unistd.h>
#include "semaphore.h"

ConfigCoordinador configuracion;
int socket_plan; //esto cambiar tal vez
t_dictionary *instancias_Claves;
t_dictionary *lista_Instancias;
t_list *listaSoloInstancias;
int contadorEquitativeLoad;
int instancia_muerta;

int ejecutando = 0;

pthread_mutex_t semaforo_avisando_compactacion = PTHREAD_MUTEX_INITIALIZER;
sem_t semaforo_compactacion;
sem_t contador_instancias;

t_sentencia ultima_sentencia;

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL); //a veces el printf no andaba, se puede hacer esto o un fflush
	int banderaPlanificador = 0;

	sem_init(&contador_instancias, true, 0);

	contadorEquitativeLoad = 0;
	instancias_Claves= dictionary_create();
	listaSoloInstancias=list_create();
	configuracion = cargar_config_coordinador(argv[1]);

	imprimir_configuracion();
	crear_log_operaciones();
	log_info(log_operaciones, "Se ha cargado la configuracion inicial del Coordinador:");
	imprimir_cfg_en_log();
	lista_Instancias = dictionary_create();
	int listener = crear_socket_de_escucha(configuracion.puerto_escucha);
	int nuevo_socket, modulo;
	signal(SIGINT, finalizar_proceso);
	while(1){
		if(banderaPlanificador == 0){
			nuevo_socket = aceptar_nueva_conexion(listener);
			recv(nuevo_socket, &modulo, sizeof(int), 0);//HS
			crear_hilo(nuevo_socket, modulo);

			int socket_server = conexion_con_servidor(configuracion.ip_planificador, configuracion.puerto_planificador);
			socket_plan = socket_server; //pasar ocmo parametro, no global, pero por ahora lo hago asi
			avisar(socket_plan, conectar_coord_planif);
			log_info(log_operaciones, "Conectado con planificador");
			banderaPlanificador = 1;
		}
		nuevo_socket = aceptar_nueva_conexion(listener);
		recv(nuevo_socket, &modulo, sizeof(int), 0);//HS
		crear_hilo(nuevo_socket, modulo);
	}
	//nunca llega hasta aca
	log_info(log_operaciones, "Terminando la ejecucion del proceso..");

	return 0;
}

void actualizar_instancia(char* nombre, int nuevo_socket) {
	instancia_Estado_Conexion *instancia_conexion = dictionary_get(lista_Instancias, nombre);
	instancia_conexion->socket = nuevo_socket;
	configurar_instancia(nuevo_socket);
	printf(GREEN"\nLa instancia"CYAN" %s "GREEN"se ha vuelto a conectar en socket "RED" %d"RESET, nombre, nuevo_socket);
}

void *rutina_instancia(void * arg) {
	log_info(log_operaciones, "Se creo un hilo con rutina instancia");
	int socket_INST = (int)arg;
	void * stream;
	recibirMensaje(socket_INST, &stream);
	char* nombre_inst = strdup((char*)stream);

	if(!existe_instancia(nombre_inst))
		nueva_instancia(socket_INST, nombre_inst);
	else
		actualizar_instancia(nombre_inst, socket_INST);

	logear_info(string_from_format("Se conecto la %s", nombre_inst));

	free(nombre_inst);
	free(stream);
	return NULL;
}

void logear_info(char* output) {
	log_info(log_operaciones, output);
	free(output);
}

void nueva_instancia(int socket, char* nombre) {
	instancia_Estado_Conexion *instancia_conexion = malloc(sizeof(instancia_Estado_Conexion));
	instancia_conexion->socket = socket;
	instancia_conexion->entradas_disponibles = configuracion.cant_entradas;
	dictionary_put(lista_Instancias, nombre, instancia_conexion);
	list_add(listaSoloInstancias, strdup(nombre));
	configurar_instancia(socket);
	printf(GREEN"\nNueva instancia"CYAN" %s "GREEN"conectada en socket"RED" %d"RESET, nombre, socket);
	logear_info(string_from_format("Se creo una nueva instancia. Nombre: %s. Socket: %d", nombre, socket));

	sem_post(&contador_instancias);
}

void configurar_instancia(int socket){
	int* dim = malloc(sizeof(int)*2);//RECIBE 3
	memcpy(dim,&configuracion.cant_entradas,sizeof(int));
	memcpy(dim+1,&configuracion.tamanio_entrada,sizeof(int));
	enviarMensaje(socket, config_inst, dim,sizeof(int)*2);
	free(dim);
}

void *rutina_ESI(void* argumento) {
	int socket_esi = *(int*)(&argumento);
	int id_esi;

	log_info(log_operaciones, "Se ha creado un hilo con rutina ESI");
	printf(BLUE "\nNueva rutina ESI en socket"CYAN" %d."RESET, socket_esi);

	void* stream;
	while (recibirMensaje(socket_esi, &stream) == ejecutar_sentencia_coordinador) {
		t_sentencia sentencia = *(t_sentencia*)stream;
		free(stream);
		id_esi = sentencia.id_esi;
		enviarMensaje(socket_plan, sentencia_coordinador, &sentencia, sizeof(t_sentencia));
		int	accion = recibirMensaje(socket_plan, &stream);
		free(stream);
		usleep(configuracion.retardo * 1000);
		procesar_permiso_planificador(accion, sentencia, socket_esi);
	}
	//free(stream);

	printf(YELLOW"\nEl ESI %d ha terminado su rutina. Finalizando ESI...\n"RESET, id_esi);
	enviarMensaje(socket_plan, terminar_esi, &id_esi, sizeof(id_esi));
	return NULL;
}

void procesar_permiso_planificador(Accion mensaje, t_sentencia sentencia, int socket_esi) { //el planif me da el ok para ejecutar
	int resultado; //lo mando luego de ejecutar
	switch (mensaje) {
		case sentencia_coordinador:
			realizar_sentencia(sentencia);
			resultado = exitoso;
			break;
		case esi_bloqueado:
			//printf(YELLOW "\nESI bloqueado por el planificador" RESET);
			resultado = bloqueado; //antes era exitoso /?
			break;
		case error_sentencia:
		case error:
		default:
			printf(RED"\nError del planificador!" RESET);
			resultado = no_exitoso;
			break;
	}
	enviarMensaje(socket_esi, resultado_ejecucion, &resultado, sizeof(int));
}

void GET(t_sentencia sentencia) {
	if(!existe_clave(sentencia.clave)) {
		char* cero = "0";
		char* a = malloc(strlen(cero) + 1); //'0' + barra cero
		strcpy(a, cero);
		dictionary_put(instancias_Claves, sentencia.clave, a);
		logear_info(formatear_mensaje_esi(sentencia.id_esi, S_GET, sentencia.clave, NULL));
	}
}

void SET(t_sentencia sentencia) {

	char* instancia = buscar_instancia(sentencia.clave);

	if(!clave_tiene_instancia(sentencia.clave))
		instancia = aplicar_algoritmo(sentencia);

	instancia_Estado_Conexion* conexion = dictionary_get(lista_Instancias , instancia);
	int socket = conexion->socket;
	enviarMensaje(socket, ejecutar_sentencia_instancia, &sentencia, sizeof(t_sentencia));
	avisar_guardado_planif(instancia, sentencia.clave); //aviso de clave guardada en tal instancia
	void* cantidad;
	int operacion = recibirMensaje(socket, &cantidad);
	if (operacion != error) {
		actualizar_entradas(instancia, *((int*)cantidad));
		free(cantidad);
	}
	procesar_pedido_instancia(operacion, instancia, sentencia.id_esi);

}

void STORE(t_sentencia sentencia) {

	char* instancia = buscar_instancia(sentencia.clave);

	if(!clave_tiene_instancia(sentencia.clave))
		instancia = aplicar_algoritmo(sentencia);

	instancia_Estado_Conexion* conexion = dictionary_get(lista_Instancias, instancia);
	int socket = conexion->socket;
	enviarMensaje(socket, ejecutar_sentencia_instancia, &sentencia, sizeof(t_sentencia));
	void* cantidad;
	int operacion = recibirMensaje(socket, &cantidad);
	actualizar_entradas(instancia, *((int*)cantidad));
	if (operacion != error) {
		actualizar_entradas(instancia, *((int*)cantidad));
		free(cantidad);
	}
	procesar_pedido_instancia(operacion, instancia, sentencia.id_esi);
}

void realizar_sentencia(t_sentencia sentencia) {

	pthread_mutex_lock(&semaforo_avisando_compactacion);
	ejecutando = 1;
	int i;
	sem_getvalue(&contador_instancias, &i);
	if (i == 0) {
		printf(RED"\nEsperando que haya instancias..."RESET);
		sem_wait(&contador_instancias);
	}

	ultima_sentencia = sentencia; //por si falla la instancia, la vuelvo a hacer con otra

	imprimir_sentencia(sentencia);

	switch(sentencia.tipo) {
		case S_GET:		GET(sentencia); 	break;
		case S_SET: 	SET(sentencia); 	break;
		case S_STORE: 	STORE(sentencia); 	break;
		default: 		log_error(log_operaciones,"Error al identificar la operacion en el coordinador"); break;
	}

	if (i == 0)
		sem_post(&contador_instancias);

	pthread_mutex_unlock(&semaforo_avisando_compactacion);

	ejecutando = 0;
}



void procesar_pedido_instancia(Accion operacion, char* instancia, int esi) {

	switch(operacion) {
		case compactacion_ok:
		case ejecucion_ok:
			//printf(GREEN"\nLa instancia ejecuto la operacion de forma exitosa."RESET);
			logear_info(string_from_format("La %s ejecuto correctamente", instancia));
			break;
		case ejecucion_no_ok: {
			int id = esi;
			printf(RED"\nError de STORE por instancia desconectada, abortando ESI %d"RESET, id);
			enviarMensaje(socket_plan, abortar_esi, &id, sizeof(int));
			break;
		}
		case compactar:
			hilo_avisar_compactacion(); //creo un hilo que espera las compactaciones
			break;
		case instancia_desconectada:
		case error:
		default:
			printf(RED"\nLa instancia no pudo ejecutar la operacion."RESET);
			printf(RED"\nDesconectando "CYAN"%s."RESET, instancia);
			eliminar_instancia(instancia);
			reintentar_sentencia(ultima_sentencia);
			logear_info(string_from_format("Se desconecto la %s", instancia));
			break;
	}

}

void reintentar_sentencia(t_sentencia sentencia) {
	modificar_clave(sentencia.clave, "0"); //la clave queda libre para que la nueva inst la agarre
	pthread_mutex_unlock(&semaforo_avisando_compactacion);
	printf(GREEN"\n\nReintentando ultima sentencia con otra instancia..."RESET);
	realizar_sentencia(ultima_sentencia); //vuelvo a intentar con la lista de inst actualizada
}

void liberar_claves(char* instancia) {

	printf(RED"\nLiberando claves de instancia %s..."RESET, instancia);

	void lib_clave(char* clave, void* el) {

		char* inst = dictionary_get(instancias_Claves, clave);
		if (strcmp(inst, instancia) == 0) {
			dictionary_remove(instancias_Claves, clave);
		}
	}

	dictionary_iterator(instancias_Claves, (void*)lib_clave);
}

void eliminar_instancia(char* nombre_instancia) {
	int indice = indice_instancia_por_nombre(nombre_instancia);
	contadorEquitativeLoad = 0;
	list_remove(listaSoloInstancias, indice);
	//dictionary_remove(lista_Instancias, nombre_instancia);
	liberar_claves(nombre_instancia);

	sem_wait(&contador_instancias);
}

void* avisar_compactacion() {

	pthread_mutex_lock(&semaforo_avisando_compactacion);
	t_list* instancias = listaSoloInstancias;
	int cantidad = list_size(instancias);

	sem_init(&semaforo_compactacion, true, 0);

	for (int i = 0; i < cantidad; i++) {
		char* instancia = list_get(instancias, i);
		instancia_Estado_Conexion* conexion = dictionary_get(lista_Instancias, instancia);
		hilo_compactacion(conexion->socket); //creo un hilo para cada instancia
	}

	printf(RED"\nCOMPACTANDO INSTANCIAS\n"RESET);
	esperar_compactacion(cantidad);
	printf(GREEN"\nCOMPACTACION FINALIZADA\n"RESET);
	pthread_mutex_unlock(&semaforo_avisando_compactacion);
	return NULL;
}

void hilo_avisar_compactacion() {
	pthread_attr_t attr;
	pthread_t hilo;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (&hilo ,&attr, avisar_compactacion, NULL);
	pthread_attr_destroy(&attr);
}

void hilo_compactacion(int socket_instancia) {
	pthread_attr_t attr;
	pthread_t hilo;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (&hilo ,&attr, rutina_compactacion, (void *)socket_instancia);
	pthread_attr_destroy(&attr);
}

void* rutina_compactacion(void* sock) {
	int socket_instancia = (int)sock;
	void* stream;
	printf(GREEN"\nCompactando instancia en socket %d"RESET, socket_instancia);
	avisar(socket_instancia, compactar); //aviso a la instancia que debe compactar

	int resultado = recibirMensaje(socket_instancia, &stream); //espero que compacte
	free(stream);
	switch(resultado) {
	case ejecucion_ok:
	case compactacion_ok:
		printf(GREEN"\nTermino de compactar la instancia del socket %d"RESET, socket_instancia);
		sem_post(&semaforo_compactacion);
		break;
	case error:
	default:
		//printf("\nerror accion = %d", resultado);
		printf(RED"\nFallo una instancia al compactar (socket %d)"RESET, socket_instancia);
		eliminar_instancia(instancia_por_socket(socket_instancia));
		sem_post(&semaforo_compactacion);
		break;
	}
	return NULL;
}

void avisar_guardado_planif(char* instancia, char* clave) {
	int largo_clave = strlen(clave) + 1, largo_instancia = strlen(instancia) + 1;
	int tam_total = sizeof(int) * 2 + largo_clave + largo_instancia;
	void* respuesta = malloc(tam_total);
	memcpy(respuesta, &largo_clave, sizeof(int));
	memcpy(respuesta + sizeof(int), &largo_instancia, sizeof(int));
	memcpy(respuesta + sizeof(int) * 2, clave, largo_clave);
	memcpy(respuesta + sizeof(int) * 2 + largo_clave, instancia, largo_instancia);

	enviarMensaje(socket_plan, clave_guardada_en_instancia, respuesta, tam_total);
	free(respuesta);
}


int clave_tiene_instancia(char* clave) {
	char* inst = (char*)dictionary_get(instancias_Claves, clave);
	if (inst != NULL)
		return strcmp(inst, "0") == 0 ? 0 : 1;
	return 0;
}



char* aplicar_algoritmo(t_sentencia sentencia) { //DEVUELVE EL NOMBRE DE LA INSTANCIA ASIGNADA

	switch(configuracion.algoritmo) {
		case el:	equitative_load(sentencia.clave);	break;
		case lsu:	least_space_used(sentencia.clave);	break;
		case ke: 	key_explicit(sentencia.clave); 		break;
		default: 								break;
	}

	logear_info(formatear_mensaje_esi(sentencia.id_esi, S_SET, sentencia.clave, sentencia.valor));
	return (char*) dictionary_get(instancias_Claves , sentencia.clave);
}

void crear_hilo(int nuevo_socket, int modulo) {
	pthread_attr_t attr;
	pthread_t hilo;
	//Hilos detachables con manejo de errores tienen que ser logs
	int  res = pthread_attr_init(&attr);
	if (res != 0) {
		log_error(log_operaciones, "Error en los atributos del hilo func: crear_hilo");
	}
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (res != 0) {
		log_error(log_operaciones, "Error en el seteado del estado de detached. func: crear_hilo");
	}
	switch (modulo) {
	case instancia:
		res = pthread_create (&hilo ,&attr, rutina_instancia , (void *)nuevo_socket);
		break;
	case esi:
		res = pthread_create (&hilo ,&attr, rutina_ESI , (void *)nuevo_socket);
		break;
	case consola:
		res = pthread_create (&hilo ,&attr, rutina_consulta , (void *)nuevo_socket);
		break;
	default:
		break;
	}
	if (res != 0) {
		log_error(log_operaciones, "Error en la creacion del hilo");
	}

	pthread_attr_destroy(&attr);
}

void mostrar_archivo(char* path) {
	FILE *f = fopen(path, "r");
	if(f == NULL)
	  printf("Error al abrir el archivo\n");
	else
		while(!feof(f))
			printf("%c", getc(f));
	fclose(f);
}

void crear_log_operaciones() {
	log_operaciones = log_create("operaciones_coordinador.log", "coordinador", 0, 1);
}

void esperar_compactacion(int cantidad_conectadas) {
	for (int i = 0; i < cantidad_conectadas; i++)
		sem_wait(&semaforo_compactacion);
}

void modificar_clave(char* clave, char* instancia) {
	char* i = dictionary_remove(instancias_Claves, clave);
	free(i);
	char* inst = strdup(instancia);
	dictionary_put(instancias_Claves, clave, inst);
}

void actualizar_entradas(char* instancia, int entradas) {
	instancia_Estado_Conexion* aux = dictionary_get(lista_Instancias, instancia);
	aux->entradas_disponibles = entradas;
}

void equitative_load(char* claveSentencia){
	t_list* instancias = listaSoloInstancias;
	int cantidadInstancias = list_size(instancias) - 1;
	char* instancia = list_get(instancias, contadorEquitativeLoad);
	modificar_clave(claveSentencia, instancia);
	contador_EQ(cantidadInstancias);
}

void contador_EQ(int instancias){
	int contador = contadorEquitativeLoad;
	contadorEquitativeLoad++;
	if (contador + 1 > instancias)
		contadorEquitativeLoad = 0;
}

int indice_instancia_por_nombre(char* nombre) {
	for (int i = 0; i < list_size(listaSoloInstancias); i++)
		if (strcmp(list_get(listaSoloInstancias, i), nombre) == 0)
			return i;
	return -1;
}

char* instancia_por_socket(int socket) {
	t_list* l = listaSoloInstancias;
	for (int i = 0; i < list_size(l); i++) {
		char* inst = list_get(l, i);
		instancia_Estado_Conexion* estado = dictionary_get(lista_Instancias, inst);
		if (estado->socket == socket)
			return inst;
	}
	return NULL;
}

void key_explicit(char* claveSentencia){
	t_list* instancias_activas = listaSoloInstancias;
	int cantidad_instancias = list_size(instancias_activas);
	int comienzo = claveSentencia[0] - ('a' - 1);

	float asignacion_por_instancia = (float) 26  /  (float)cantidad_instancias;
	float multiplicador = 1;
	for(int j = 1; j <= 26; j++){
		if(asignacion_por_instancia * multiplicador < comienzo)
			multiplicador++;
		if(comienzo == j) {
			modificar_clave(claveSentencia, (char*)list_get(instancias_activas, multiplicador - 1 ));
			break;
		}
	}
}

void least_space_used(char* clave) {
	char* instancia = instancia_con_mas_espacio();
	modificar_clave(clave, instancia);
}

char* instancia_con_mas_espacio() {
	t_list* instancias_activas = listaSoloInstancias;
	int cantidad_instancias = list_size(instancias_activas);
	char* instancia = list_get(instancias_activas, 0); //la primera
	instancia_Estado_Conexion* conexion = dictionary_get(lista_Instancias, instancia); //conex de la primera

	for(int j = 1; j < cantidad_instancias; j++) {
		char* otra_instancia = list_get(instancias_activas, j);
		instancia_Estado_Conexion* otra_conexion = dictionary_get(lista_Instancias, otra_instancia);
		if(otra_conexion->entradas_disponibles > conexion->entradas_disponibles) { //si es mejor que la anterior, pasa a ser la seleccionada
			instancia = otra_instancia;
			conexion = otra_conexion;
		}
	}
	return instancia;
}

char* formatear_mensaje_esi(int id, TipoSentencia t, char* clave, char* valor) {

	char* formato_string = string_new();
	char* str_id = string_itoa(id);
	string_append(&formato_string, "El ESI ");
	string_append(&formato_string, str_id);
	string_append(&formato_string, " hizo un");
	switch(t) {
		case S_SET:
			string_append(&formato_string, " SET sobre la clave ");
 			string_append(&formato_string, clave);
			string_append(&formato_string, " con valor: ");
			string_append(&formato_string, valor);
			break;
		case S_GET:
			string_append(&formato_string, " GET de la clave ");
			string_append(&formato_string, clave);
			break;
		case S_STORE:
			string_append(&formato_string, " STORE de la clave ");
			string_append(&formato_string, clave);
			break;
		default:
			log_error(log_operaciones, "Error al formatear el log");
	}
	free(str_id);
	return formato_string;
}

void destruir_estructuras_globales() {
	log_destroy(log_operaciones);
	dictionary_destroy_and_destroy_elements(instancias_Claves, free);
	dictionary_destroy_and_destroy_elements(lista_Instancias, free);
	list_clean_and_destroy_elements(listaSoloInstancias, free);
	free(listaSoloInstancias);
}

void finalizar_proceso() {
	destruir_estructuras_globales();
	exit(0);
}

void imprimir_sentencia(t_sentencia sentencia) {
	char* tipo;
	switch (sentencia.tipo) {
		case S_GET: tipo = "GET"; break;
		case S_SET: tipo = "SET"; break;
		default: tipo = "STORE"; break;
	}
	printf("\nSentencia "CYAN"%s"RESET" del "GREEN"ESI %d"RESET" con clave "RED"%s"RESET, tipo, sentencia.id_esi, sentencia.clave);
}

char* buscar_instancia(char* clave) {
	return (char*)dictionary_get(instancias_Claves, clave);
}

void nodo_inst_conexion_destroyer(instancia_Estado_Conexion* inst) {
	free(inst);
}



////////////////////////////////////////////////////////////
//_______________________MAGIA____________________________//
//_________________________________________________________
//_________________________________________________________

void* rutina_consulta(void* argumento) {
	int socket_consola = (int)argumento;
	void* stream;
	Accion accion = recibirMensaje(socket_consola, &stream);

	switch(accion) {
		case consulta_simulacion: {
			char* instancia = "ERROR";
			instancia = simular_algoritmo((char*)stream);
			enviarMensaje(socket_consola, consulta_simulacion, instancia, strlen(instancia) + 1);
			break;
		}
		case aviso_bloqueo_clave: {
			int esi, largo;

			memcpy(&esi, stream, sizeof(int));
			memcpy(&largo, stream + sizeof(int), sizeof(int));
			char* clave = malloc(largo);
			memcpy(clave, stream + sizeof(int) * 2, largo);

			if (esi != 0)
				printf("\nEl planificador ha desbloqueado "GREEN"ESI %d"RESET" con clave"RED" %s"RESET, esi, clave);
			else
				printf("\nEl planificador ha desbloqueado la clave sin ESI"RED" %s"RESET, clave);

			if(!existe_clave(clave)) {
				char* cero = "0";
				char* a = malloc(strlen(cero) + 1); //'0' + barra cero
				strcpy(a, cero);
				dictionary_put(instancias_Claves, clave, a);
				logear_info(formatear_mensaje_esi(esi, S_GET, clave, NULL));
			}

			break;
		}
		default:
			break;
	}

	free(stream);

	return NULL;
}

char* simular_algoritmo(char* clave) {
	switch(configuracion.algoritmo) {
		case el:	return equitative_load_simulado(clave);
		case lsu:	return least_space_used_simulado(clave);
		case ke: 	return key_explicit_simulado(clave);
		default: 	return "ERROR";
	}
}

char* equitative_load_simulado(char* clave) {
	char* instancia = list_get(listaSoloInstancias, contadorEquitativeLoad);
	return instancia;
}

char* key_explicit_simulado(char* clave) {
	t_list* instancias_activas = listaSoloInstancias;
	int cantidad_instancias = list_size(instancias_activas);
	int comienzo = clave[0] - ('a' - 1);
	float asignacion_por_instancia = (float) 26  /  (float)cantidad_instancias;
	float multiplicador = 1;
	for (int j = 1; j <= 26; j++) {
		if(asignacion_por_instancia * multiplicador < comienzo)
			multiplicador++;
		if(comienzo == j)
			return (char*)list_get(instancias_activas, multiplicador - 1);
	}
	return NULL;
}

char* least_space_used_simulado(char* clave) {
	return instancia_con_mas_espacio();
}

int existe_clave(char* clave) {
	return dictionary_has_key(instancias_Claves, clave);
}

int existe_instancia(char* nombre) {
	return dictionary_has_key(lista_Instancias, nombre);
}

void imprimir_cfg_en_log() {
	logear_info(string_from_format("PUERTO_ESCUCHA: %d", configuracion.puerto_escucha));
	logear_info(string_from_format("PUERTO_PLANIF: %d", configuracion.puerto_planificador));
	logear_info(string_from_format("IP_PLANIF: %s", configuracion.ip_planificador));
	if(configuracion.algoritmo == el)
		log_info(log_operaciones, "ALGORITMO_DISTRIBUCION: EQ");
	if(configuracion.algoritmo == lsu)
		log_info(log_operaciones, "ALGORITMO_DISTRIBUCION: LSU");
	if(configuracion.algoritmo == ke)
		log_info(log_operaciones, "ALGORITMO_DISTRIBUCION: KE");
	logear_info(string_from_format("CANTIDAD_ENTRADAS: %d", configuracion.cant_entradas));
	logear_info(string_from_format("TAMANIO_ENTRADA: %d", configuracion.tamanio_entrada));
	logear_info(string_from_format("RETARDO: %d", configuracion.retardo));
}

void imprimir_configuracion() {
	printf(CYAN"\nPuerto de escucha: "YELLOW"%d"RESET, configuracion.puerto_escucha);
	printf(CYAN"\nPlanificador: "YELLOW"%s : %d"RESET, configuracion.ip_planificador, configuracion.puerto_planificador);
	printf(GREEN"\n\nConfiguracion cargada con exito:"RESET);
	printf(GREEN"\nAlgoritmo: "RED"%s", algoritmo(configuracion.algoritmo));
	printf(GREEN"\nEntradas: "RED"%d", configuracion.cant_entradas);
	printf(GREEN"\nTamaño: "RED"%d", configuracion.tamanio_entrada);
	printf(GREEN"\nRetardo: "RED"%d\n"RESET, configuracion.retardo);
}

char* algoritmo(AlgoritmoCoord alg) {
	switch (alg) {
		case lsu: 			return "LSU";
		case ke: 			return "KE";
		case el:default: 	return "EL";
	}
}



