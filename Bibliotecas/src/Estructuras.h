#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include "Macros.h"
#include <commons/collections/list.h>

typedef enum { S_GET, S_SET, S_STORE } TipoSentencia;

typedef struct {
	int id;
	int estimacion_anterior; //la inicial esta dada por arch de config
	int rafaga_anterior;
	int cant_rafagas;
	t_list* claves;
	//char* claves[MAX_CLAVES];
	t_list* cola_actual;
	int posicion; //en la cola, en vez de filtrar la lista, ya se el indice -> mas rapido
	int socket_p;
	int socket_c;
} ESI;

typedef struct {
	char clave[LARGO_CLAVE];
	char valor[LARGO_VALOR];
	TipoSentencia tipo;
	int id_esi;
} t_sentencia;


typedef struct {
	int bloqueada;
	char clave[LARGO_CLAVE];
	char valor[LARGO_VALOR];
	ESI* esi_duenio;
	t_list* esis_esperando;
} t_clave;

typedef enum {
	no_exitoso=0,
	exitoso,
	dudoso
} ResultadoEjecucion;

typedef enum {
	error = 0,
	conectar_coord_planif,
	conectar_esi_planif,
	sentencia_coordinador,
	nuevo_esi,
	ejecutar_proxima_sentencia,
	ejecutar_sentencia_coordinador,
	preguntar_recursos_planificador,
	recurso_disponible,
	ejecucion_ok,
	ejecucion_no_ok,
	no_hay_mas_sentencias, //el esi le avisa al coordinador
	terminar_esi, //coord -> planif
	config_inst, //coord -> inst
	ejecutar_sentencia_instancia, //envia SET o STORE a la instancia
	compactar,
	esi_bloqueado,
	error_sentencia,
	resultado_ejecucion,
	cerrar_conexion_coord,
	cerrar_conexion_esi,
	podes_seguir

} Accion;

#endif


