/*
 * Coordinador.h
 *
 *  Created on: 22 abr. 2018
 *      Author: utnso
 */

#ifndef COORDINADOR_H_
#define COORDINADOR_H_

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

#define PORT 9034

typedef struct{
	int puerto;
	char algoritmo[3];
	int cant_entradas;
	int tamanio_entradas;
	int retardo;
}Configuracion;

Configuracion* leerArchivoConfiguracion(t_config* arch);
void mostrarPorPantallaConfig(Configuracion* config);


#endif /* COORDINADOR_H_ */
