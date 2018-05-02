#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../Bibliotecas/src/Socket.c"
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include "commons/string.h"
#include "Coordinador.h"


int main() {
	Configuracion* configuracion = cargar_configuracion("Configuracion.cfg");
	mostrar_por_pantalla_config(configuracion);
	int listener = crear_socket_de_escucha(configuracion->puerto_escucha);
	int socket_server = conexion_con_servidor("127.0.0.1", "9034");
	int bytes = enviar("ESI", socket_server);
	int nuevo_socket, modulo;
	while(1){
		nuevo_socket = aceptar_nueva_conexion(listener);
		recv(nuevo_socket, &modulo, sizeof(int), 0);//HS
		crear_hilo(nuevo_socket, modulo);
	}
	return 0;
}

//ACCIONES DE LOS HILOS
void *rutina_instancia(void * arg) {
	int socket_INST = (int)arg;
	printf("Nueva Instancia ejecutada\n");
	configurar_instancia(socket_INST);
	return NULL;
}

void configurar_instancia(int socket){
	Configuracion* configuracion = cargar_configuracion("Configuracion.cfg");
	Dimensiones_Inst* dim = malloc(sizeof(dim));
	dim->cant_entradas = configuracion->cantidad_entradas;
	dim->tam_entradas = configuracion->tamanio_entradas;
	enviarMensaje(socket,1,(void*)dim,sizeof(Dimensiones_Inst));
	free(configuracion);
}


void *rutina_ESI(void * arg) {
	int socket_CPU = (int)arg;
	return NULL;
}

void crear_hilo(int nuevo_socket, int modulo) {
	pthread_attr_t attr;
	pthread_t hilo;
	//Hilos detachables con manejo de errores tienen que ser logs
	int  res = pthread_attr_init(&attr);
	if (res != 0) {
		printf("Error en los atributos del hilo");
	}
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (res != 0) {
		printf("Error en el seteado del estado de detached");
	}
	res = (modulo == instancia) ? pthread_create (&hilo ,&attr, rutina_instancia , (void *)nuevo_socket)
			:pthread_create (&hilo ,&attr, rutina_ESI , (void *)nuevo_socket);
	if (res != 0) {
		printf("Error en la creacion del hilo");
	}
	pthread_attr_destroy(&attr);
}

int enviar(char* mensaje, int socket) {
	int longitud = strlen(mensaje) + 1;
	send(socket, &longitud, sizeof(int), 0);
	return send(socket, mensaje, longitud, 0);
}

Configuracion* cargar_configuracion(char* ruta) {
	char* campos[5] = { "PUERTO_ESCUCHA","ALGORITMO_DISTRIBUCION", "CANTIDAD_ENTRADAS", "TAMANIO_ENTRADAS", "RETARDO" };
	t_config* archivo = crear_archivo_configuracion(ruta, campos);
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	configuracion->puerto_escucha = config_get_int_value(archivo, "PUERTO_ESCUCHA");
	strcpy(configuracion->algoritmo_distribucion, config_get_string_value(archivo, "ALGORITMO_DISTRIBUCION"));
	configuracion->cantidad_entradas = config_get_int_value(archivo, "CANTIDAD_ENTRADAS");
	configuracion->tamanio_entradas = config_get_int_value(archivo, "TAMANIO_ENTRADAS");
	configuracion->retardo = config_get_int_value(archivo, "RETARDO");
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
		return crear_prueba_configuracion("LSU");
	return config;
}

t_config* crear_prueba_configuracion(char* algoritmo_distribucion) {
	char* ruta = "Configuracion.cfg";
	fclose(fopen(ruta, "w"));
	t_config *config = config_create(ruta);
	config_set_value(config, "PUERTO_ESCUCHA", "9035");
	config_set_value(config, "ALGORITMO_DISTRIBUCION", algoritmo_distribucion);
	config_set_value(config, "CANTIDAD_ENTRADAS", "20");
	config_set_value(config, "TAMANIO_ENTRADAS", "100");
	config_set_value(config, "RETARDO", "300");
	config_save(config);
	return config;
}

void mostrar_por_pantalla_config(Configuracion* config) {
	puts("----------------PROCESO COORDINADOR--------------");
	printf("PUERTO: %i\n", config->puerto_escucha);
	printf("ALGORITMO: %s\n", config->algoritmo_distribucion);
	printf("CANTIDAD DE ENTRADAS: %i\n", config->cantidad_entradas);
	printf("TAMANIO DE ENTRADAS: %i\n", config->tamanio_entradas);
	printf("RETARDO: %i\n", config->retardo);
}

void guardar_en_log(int id_esi, char* sentencia) {
	FILE* log_operacion = fopen(LOG_PATH, "a+");
	fprintf(log_operacion,"%d %s %s %s",id_esi,"		",sentencia,"\n");
	fclose(log_operacion);
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

void crear_log_operacion() {
	FILE* log_operacion = fopen(LOG_PATH, "a+");
	fprintf(log_operacion, "%s", "ESI		OPERACION\n\n");
	fclose(log_operacion);
}

