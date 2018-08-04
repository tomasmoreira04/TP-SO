#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <stdio.h>
#include <stdlib.h>
#include "Socket.h"
#include <commons/config.h>
#include "Macros.h"

typedef enum { sjf_sd, sjf_cd, hrrn, fifo } AlgoritmoPlanif;
typedef enum { el, lsu, ke } AlgoritmoCoord;
typedef enum {CIRC, LRU, BSU} AlgoritmoInst;

typedef struct {
	int puerto_escucha,puerto_coordinador, alfa_planif, algoritmo;
	float estimacion_inicial;
	char ip_coordinador[LARGO_IP];
	char** claves_bloqueadas;
	int n_claves;
	int mostrar_estimacion;
} ConfigPlanificador;

typedef struct {
	int puerto_escucha, cant_entradas, tamanio_entrada, retardo, puerto_planificador, algoritmo;
	char ip_planificador[LARGO_IP];
} ConfigCoordinador;

typedef struct {
	int puerto_coordinador, puerto_planificador;
	char ip_coordinador[LARGO_IP], ip_planificador[LARGO_IP];
} ConfigESI;

typedef struct {
	char ip_coordinador[LARGO_IP], punto_montaje[LARGO_RUTA], nombre_instancia[LARGO_NINSTANCIA];
	int puerto_coordinador, intervalo_dump, algoritmo_reemp, mostrar_storage;
} ConfigInstancia;

typedef struct {
	char ip_coordinador[LARGO_IP], ip_planificador[LARGO_IP];
	int puerto_coordinador, puerto_planificador;
} ConfigGlobal;

ConfigGlobal cargar_config_global();
ConfigPlanificador cargar_config_planificador();
ConfigCoordinador cargar_config_coordinador();
ConfigESI cargar_config_esi();
ConfigInstancia cargar_config_inst();
t_config* cargar_archivo(char* ruta, char** campos);
ConfigPlanificador config_predeterminada_planif();
ConfigCoordinador config_predeterminada_coord();
ConfigESI config_predeterminada_esi();
ConfigInstancia config_predeterminada_inst();
char* ruta_modulo(Modulo modulo);
AlgoritmoPlanif numero_algoritmo_p(char* nombre);
AlgoritmoCoord numero_algoritmo_c(char* nombre);
AlgoritmoInst numero_algoritmo_i(char* nombre);
int faltan_campos(t_config* config, char** campos);
void imprimir_error();
int numero_claves(char* linea);
int procesar_config_storage(char* valor_campo);

#endif
