#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <stdio.h>
#include <stdlib.h>
#include "Socket.h"
#include <commons/config.h>

typedef struct {
	int puerto_escucha, estimacion_inicial, puerto_coordinador, alfa_planif;
	char algoritmo_planif[7], ip_coordinador[16];
	char** claves_bloqueadas;
} ConfigPlanificador;

typedef struct {
	int puerto_escucha, cant_entradas, tamanio_entrada, retardo;
	char algoritmo_distrib[7];
} ConfigCoordinador;

typedef struct {
	int puerto_coordinador, puerto_planificador;
	char ip_coordinador[16], ip_planificador[16];
} ConfigESI;

typedef struct {
	char ip_coordinador[16], algoritmo_reemp[7], punto_montaje[50], nombre_instancia[20];
	int puerto_coordinador, intervalo_dump;
} ConfigInstancia;

ConfigPlanificador cargar_config_planificador();
ConfigCoordinador cargar_config_coordinador();
ConfigESI cargar_config_esi();
ConfigInstancia cargar_config_inst();
t_config* cargar_archivo(Modulo modulo, char** campos);
ConfigPlanificador config_predeterminada_planif();
ConfigCoordinador config_predeterminada_coord();
ConfigESI config_predeterminada_esi();
ConfigInstancia config_predeterminada_inst();
char* ruta_modulo(Modulo modulo);
int faltan_campos(t_config* config, char** campos);

#endif
