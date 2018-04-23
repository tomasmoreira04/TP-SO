#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../Bibliotecas/src/Socket.c"
#include "commons/config.h"
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include "commons/string.h"
#include "Coordinador.h"

int main() {
	Configuracion* configuracion = cargar_configuracion("Configuracion.cfg");
	mostrar_por_pantalla_config(configuracion);

	char* buffer = "ESI";
	int socket_server = conexion_con_servidor("127.0.0.1", "9034");
	int longitud;
	longitud = strlen(buffer) + 1;
	send(socket_server, &longitud, sizeof(int), 0);
	send(socket_server, buffer, longitud, 0);
	/*while(1){
	 int tamanio = strlen(j);
	 recv(socket_server, j, tamanio, 0);
	 printf("\nel valor %s\n", j);
	 }*/

	/*void* conectar_planificador() {

	 return NULL;
	 }*/
	return 0;
}


Configuracion* cargar_configuracion(char* ruta) {
	char* campos[6] = { "PUERTO_ESCUCHA","ALGORITMO_DISTRIBUCION", "CANTIDAD_ENTRADAS", "TAMANIO_ENTRADAS", "RETARDO" };
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
	config_set_value(config, "PUERTO_ESCUCHA", "8000");
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

