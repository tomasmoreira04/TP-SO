/*
 * Instancia.h
 *
 *  Created on: 22 abr. 2018
 *      Author: utnso
 */

#ifndef INSTANCIA_H_
#define INSTANCIA_H_

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


typedef struct{

	char ip_coordinador[20];
	int puerto_coordinador;
	char algoritmo_reemplazo[5];
	char* path;
	char* nombre;
	int intervalo_dump;

}Configuracion;


Configuracion* leerArchivoConfiguracion(t_config* arch);



#endif /* INSTANCIA_H_ */
