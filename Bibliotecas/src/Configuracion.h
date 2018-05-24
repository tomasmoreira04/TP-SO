#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <stdio.h>
#include <stdlib.h>
#include "Socket.h"
#include <commons/config.h>
#include "Macros.h"

typedef struct {
	int puerto_escucha, estimacion_inicial, puerto_coordinador, alfa_planif;
	char algoritmo_planif[LARGO_ALG], ip_coordinador[LARGO_IP];
	char** claves_bloqueadas;
} ConfigPlanificador;

typedef struct {
	int puerto_escucha, cant_entradas, tamanio_entrada, retardo;
	char algoritmo_distrib[LARGO_ALG];
} ConfigCoordinador;

typedef struct {
	int puerto_coordinador, puerto_planificador;
	char ip_coordinador[LARGO_IP], ip_planificador[LARGO_IP];
} ConfigESI;

typedef struct {
	char ip_coordinador[LARGO_IP], algoritmo_reemp[LARGO_ALG], punto_montaje[LARGO_RUTA], nombre_instancia[LARGO_NINSTANCIA];
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
