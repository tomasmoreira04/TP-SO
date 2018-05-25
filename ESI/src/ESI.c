#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../../Bibliotecas/src/Socket.c"
#include "../../Bibliotecas/src/Configuracion.c"
#include "../../Bibliotecas/src/Estructuras.h"
#include "commons/string.h"
#include "commons/collections/list.h"
#include "ESI.h"
#include <parsi/parser.h>

//Clonen repo y instalen el parsi en su VM: https://github.com/sisoputnfrba/parsi
//VER https://github.com/sisoputnfrba/parsi/blob/master/src/parsi/parser.h para entender funciones

int main(int argc, char* argv[]) {
	ConfigESI config = cargar_config_esi();
	char* ruta = ruta_script(argv[1]);

	int planificador = conexion_con_servidor(config.ip_planificador, config.puerto_planificador);
	int coordinador = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador);

	handShake(coordinador, esi);

	FILE* script = cargar_script(ruta);
	int rafagas = cantidad_de_sentencias(script);
	informar_nuevo_esi(planificador, rafagas);

	leer_sentencias(planificador, coordinador, ruta); //si paso puntero a FILE no me anda el getline xD, asi que abro de nuevo

	fclose(script);
	return EXIT_SUCCESS;
}

char* ruta_script(char* argumento) { //se pasa solo el nombre del archivo, sin el .esi ni la carpeta
	if (argumento == NULL)
		return "scripts/script.esi"; //default
	char* ruta = malloc(LARGO_RUTA);
	string_append(&ruta, "../scripts/");
	string_append(&ruta, argumento);
	string_append(&ruta, ".esi");
	return ruta;
}

void informar_nuevo_esi(int socket, int rafagas) {
	enviarMensaje(socket, nuevo_esi, &rafagas, sizeof(rafagas));
}

//hago esto porque el struct que te dan en el parser es una mierda :)
t_sentencia convertir_operacion(t_esi_operacion a) { //necesario porque los char* estan en un union de structs y son inaccesibles si casteo desde void*
	t_sentencia b;
	if (a.keyword == GET) {
		b.tipo = GET;
		strcpy(b.clave, a.argumentos.GET.clave);
	}
	else if (a.keyword == STORE) {
		b.tipo = STORE;
		strcpy(b.clave, a.argumentos.STORE.clave);
	}
	else {
		b.tipo = SET;
		strcpy(b.clave, a.argumentos.SET.clave);
		strcpy(b.valor, a.argumentos.SET.valor);
	}
	return b;
}

void leer_sentencias(int planificador, int coordinador, char* ruta) {
	FILE* script = cargar_script(ruta);
	void* stream;
	char* linea = NULL;
	size_t largo = 0;
	ssize_t leidas = 0;

	//esperar que el planif me ejecute
	//int accion = recibirMensaje(planificador, NULL);

	while ((leidas = getline(&linea, &largo, script)) != -1) {

			t_esi_operacion operacion = parse(linea);
			if(operacion.valido){
				t_sentencia sentencia = convertir_operacion(operacion);;
				//le aviso al coord que tengo que ejecutar mi sentencia
				enviarMensaje(coordinador, ejecutar_sentencia_coordinador, &sentencia, sizeof(sentencia));

				ejecutar_operacion(operacion); //imprime nada mas en el ESI
				destruir_operacion(operacion);
			} else {
				printf(RED "\nNo se pudo interpretar " CYAN "%s\n" RESET, linea);
				exit(EXIT_FAILURE);
			}

	}
	enviarMensaje(coordinador, no_hay_mas_sentencias, NULL, 0);
	fclose(script);
	if (linea)
		free(linea);
}

void GET_CLAVE(t_esi_operacion operacion) {
	printf("GET\t" GREEN "%s\n" RESET, operacion.argumentos.GET.clave);
}

void SET_CLAVE_VALOR(t_esi_operacion operacion) {
	printf("SET\t" GREEN "%s\t" CYAN "%s\n" RESET, operacion.argumentos.SET.clave, operacion.argumentos.SET.valor);
}

void STORE_CLAVE(t_esi_operacion operacion) {
	printf("STORE\t" GREEN "%s\n" RESET, operacion.argumentos.STORE.clave);
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
