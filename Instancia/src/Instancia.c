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
#include "../../Bibliotecas/src/Socket.c"
#include "commons/string.h"
#include "commons/config.h"
#include "Instancia.h"

int main() {
	Configuracion* configuracion = cargar_configuracion("Configuracion.cfg");

	char * j = malloc(50);
	int socketServer = conexion_con_servidor("127.0.0.1", "9034"); //usar conf->puerto_coordinador
	while(1){
		int tamano=strlen(j);
		recv(socketServer,&tamano,sizeof(int),0);
		recv(socketServer,j,tamano,0);

		printf("\nel valor %s\n", j);
	}
	return 0;
}

Configuracion* cargar_configuracion(char* ruta) {
	char* campos[6] = { "IP_COORDINADOR","PUERTO_COORDINADOR","ALGORITMO_REEMPLAZO", "PUNTO_MONTAJE", "NOMBRE_INSTANCIA", "INTERVALO_DUMP" };
	t_config* archivo = crear_archivo_configuracion(ruta, campos);
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ip_coordinador, config_get_string_value(archivo,"IP_COORDINADOR"));
	configuracion->puerto_coordinador = config_get_int_value(archivo, "PUERTO_COORDINADOR");
	strcpy(configuracion->algoritmo_reemplazo, config_get_string_value(archivo, "ALGORITMO_REEMPLAZO"));
	strcpy(configuracion->punto_montaje, config_get_string_value(archivo,"PUNTO_MONTAJE"));
	strcpy(configuracion->nombre_instancia, config_get_string_value(archivo, "NOMBRE_INSTANCIA"));
	configuracion->intervalo_dump = config_get_int_value(archivo, "INTERVALO_DUMP");
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
		return crear_prueba_configuracion("CIRC");
	return config;
}

t_config* crear_prueba_configuracion(char* algoritmo_reemplazo) {
	char* ruta = "Configuracion.cfg";
	fclose(fopen(ruta, "w"));
	t_config *config = config_create(ruta);
	config_set_value(config, "IP_COORDINADOR", "127.0.0.1");
	config_set_value(config, "PUERTO_COORDINADOR", "8000");
	config_set_value(config, "ALGORITMO_REEMPLAZO", algoritmo_reemplazo);
	config_set_value(config, "PUNTO_MONTAJE", "/home/utnso/instancia1/");
	config_set_value(config, "NOMBRE_INSTANCIA", "Instancia1");
	config_set_value(config, "INTERVALO_DUMP", "10");
	config_save(config);
	return config;
}
