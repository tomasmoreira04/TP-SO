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
#include "ESI.h"
int cantidadDeSentencias();

int main() {
	Configuracion* configuracion = cargar_configuracion("Configuracion.cfg");
	char * j=malloc(50);
	int socketPlanificador= conexion_con_servidor("127.0.0.1","9034");
	int socketCoordinador= conexion_con_servidor("127.0.0.1","9035");
	handShake(socketCoordinador,esi);
	int rafagas=cantidadDeSentencias();
	//enviarMensaje(socketPlanificador,18,rafagas,sizeof(int));//FALTA DEFINIR ACCION Y EL RECV
	return 0;
}

int cantidadDeSentencias(){
	FILE * f;
	f=fopen("Script.txt","r");
	char *valor=malloc(5);
	char *aux=malloc(5);
	strcpy(valor," ");
	strcpy(aux," ");
	int i=0;
	if (f==NULL)
			perror ("Error opening file");
    do
    {
    	*aux=*valor;
    	*valor = fgetc (f);
    	if (*valor == '\n') i++;
    }
    while (*valor != EOF);
    if(*aux!='\n')  i++;
	fclose(f);
	free(valor);
	free(aux);
	return i;
}

Configuracion* cargar_configuracion(char* ruta) {
	char* campos[4] = { "IP_PLANIFICADOR","PUERTO_PLANIFICADOR", "IP_COORDINADOR", "PUERTO_COORDINADOR" };
	t_config* archivo = crear_archivo_configuracion(ruta, campos);
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ip_planificador, config_get_string_value(archivo,"IP_PLANIFICADOR"));
	configuracion->puerto_planificador = config_get_int_value(archivo, "PUERTO_PLANIFICADOR");
	strcpy(configuracion->ip_coordinador, config_get_string_value(archivo,"IP_COORDINADOR"));
	configuracion->puerto_coordinador = config_get_int_value(archivo,"PUERTO_COORDINADOR");
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
		return crear_prueba_configuracion();
	return config;
}

t_config* crear_prueba_configuracion() {
	char* ruta = "Configuracion.cfg";
	fclose(fopen(ruta, "w"));
	t_config *config = config_create(ruta);
	config_set_value(config, "IP_COORDINADOR", "127.0.0.1");
	config_set_value(config, "PUERTO_COORDINADOR", "8000");
	config_set_value(config, "IP_PLANIFICADOR", "127.0.0.2");
	config_set_value(config, "PUERTO_PLANIFICADOR", "9034");
	config_save(config);
	return config;
}


