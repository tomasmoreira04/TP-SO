#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include "Macros.h"
#include <commons/collections/list.h>

typedef struct {
	int id;
	int estimacion_anterior; //la inicial esta dada por arch de config
	int rafaga_anterior;
	int cant_rafagas;
	char* claves[LARGO_CLAVE];
	t_list* cola_actual;
	int posicion; //en la cola, en vez de filtrar la lista, ya se el indice -> mas rapido
} ESI;

typedef struct {
	char clave[LARGO_CLAVE];
	char valor[LARGO_VALOR];
	int tipo;
} t_sentencia;

typedef struct {
	ESI* esi;
	char* clave;
	t_list esis_esperando; //ordenada por fifo
} t_bloqueo;

typedef enum {
	error = 0,
	conectar_coord_planif,
	nuevo_esi,
	instruccion_esi,
} Accion;

#endif

