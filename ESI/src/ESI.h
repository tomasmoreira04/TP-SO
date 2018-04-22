/*
 * ESI.h
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#ifndef ESI_H_
#define ESI_H_

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

t_config* archivoConf;

typedef struct{
	char ip_planificador[20];
	int32_t puerto_planificador;
	char ip_coordinador[20];
	int32_t puerto_coordinador;

}Configuracion;

Configuracion* leerArchivoConfiguracion(t_config* archivo);
int estanTodosLosCampos(t_config* conf, char** campos);
t_config* crearArchivoConfiguracion(char* path, char** campos);

#endif /* ESI_H_ */
