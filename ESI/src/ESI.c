#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../../Bibliotecas/src/Socket.c"
#include "../../Bibliotecas/src/Configuracion.c"
#include "commons/string.h"
#include "commons/collections/list.h"
#include "ESI.h"
#include <parsi/parser.h>

//Clonen repo y instalen el parsi en su VM: https://github.com/sisoputnfrba/parsi
//VER https://github.com/sisoputnfrba/parsi/blob/master/src/parsi/parser.h para entender funciones

int main() {
	ConfigESI config = cargar_config_esi();
	int planificador = conexion_con_servidor(config.ip_planificador, config.puerto_planificador);
	int coordinador = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador);
	handShake(coordinador, esi);

	FILE* script = cargar_script("./script.esi");
	int rafagas = cantidad_de_sentencias(script);
	informar_nuevo_esi(planificador, rafagas);

	leer_sentencias(planificador, "./script.esi"); //si paso puntero a FILE no me anda el getline xD, asi que abro de nuevo

	fclose(script);
	return EXIT_SUCCESS;
}

void informar_nuevo_esi(int socket, int rafagas) {
	enviarMensaje(socket, nuevo_esi, &rafagas, sizeof(rafagas));
}

void leer_sentencias(int planificador, char* ruta) {
	FILE* script = cargar_script(ruta);
	char* linea = NULL;
	size_t largo = 0;
	ssize_t leidas = 0;
	while ((leidas = getline(&linea, &largo, script)) != -1) {
		t_esi_operacion operacion = parse(linea);
		if(operacion.valido){
			ejecutar_operacion(operacion);
			enviarMensaje(planificador, instruccion_esi, &operacion, sizeof(operacion));
			destruir_operacion(operacion);
		} else {
			printf(RED "\nNo se pudo interpretar " CYAN "%s\n" RESET, linea);
			exit(EXIT_FAILURE);
		}
	}
	fclose(script);
	if (linea)
		free(linea);
}

void GET_CLAVE(t_esi_operacion operacion) {
	printf("GET\tclave: <%s>\n", operacion.argumentos.GET.clave);
}

void SET_CLAVE_VALOR(t_esi_operacion operacion) {
	printf("SET\tclave: <%s>\tvalor: <%s>\n", operacion.argumentos.SET.clave, operacion.argumentos.SET.valor);
}

void STORE_CLAVE(t_esi_operacion operacion) {
	printf("STORE\tclave: <%s>\n", operacion.argumentos.STORE.clave);
}

void ejecutar_operacion(t_esi_operacion operacion) {
	switch(operacion.keyword){
		case GET:
			GET_CLAVE(operacion);
		    break;
		case SET:
		    SET_CLAVE_VALOR(operacion);
		    break;
		case STORE:
		    STORE_CLAVE(operacion);
		    break;
	}
}

FILE* cargar_script(char* ruta) {
	FILE* archivo = fopen(ruta, "r");
    if (archivo == NULL){
        printf(RED "\nERROR AL ABRIR EL SCRIPT DEL ESI\n" RESET);
        exit(EXIT_FAILURE);
    }
    return archivo;
}

int cantidad_de_sentencias(FILE* script) {
	int sentencias = 1, caracter;
	while(!feof(script)) {
	  caracter = fgetc(script);
	  if(caracter == '\n')
	    sentencias++;
	}
	return sentencias;
}
