#include "Coordinador.h"

Configuracion* leerArchivoConfiguracion(t_config* arch) {
	Configuracion* conf = malloc(sizeof(Configuracion));
	conf->puerto = config_get_int_value(arch, "PUERTO");
	strcpy(conf->algoritmo, config_get_string_value(arch, "ALGORITMO"));
	conf->cant_entradas = config_get_int_value(arch, "CANTIDAD_ENTRADAS");
	conf->tamanio_entradas = config_get_int_value(arch, "TAMANIO_ENTRADAS");
	conf->retardo = config_get_int_value(arch, "RETARDO");
	return conf;
}

void mostrarPorPantallaConfig(Configuracion* config) {
	puts("----------------PROCESO COORDINADOR--------------");
	printf("PUERTO: %i\n", config->puerto);
	printf("ALGORITMO: %s\n", config->algoritmo);
	printf("CANTIDAD DE ENTRADAS: %i\n", config->cant_entradas);
	printf("TAMANIO DE ENTRADAS: %i\n", config->tamanio_entradas);
	printf("RETARDO: %i\n", config->retardo);

}

void main() {
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
}

