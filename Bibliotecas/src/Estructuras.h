#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include "Macros.h"
#include <commons/collections/list.h>

typedef enum { S_GET, S_SET, S_STORE } TipoSentencia;

typedef struct {
	int id;
	float estimacion_anterior; //la inicial esta dada por arch de config
	int sentencias_totales;
	int tiempo_esperado; //para hrrn
	float sentencias_restantes; //para sjf
	float sentencias_ejecutadas; //para sjf
	float response_ratio; //para hrrn
	t_list* cola_actual;
	int socket_planif;
	char nombre[LARGO_ESI];
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
	char instancia[LARGO_INSTANCIA];
	ESI* esi_duenio;
	t_list* esis_esperando;
} t_clave;

typedef struct {
	int rafagas;
	char nombre[40];
} t_nuevo_esi;

typedef enum {
	no_exitoso=0,
	exitoso,
	bloqueado,
	dudoso
} ResultadoEjecucion;

typedef enum {
	error = 0,
	conectar_coord_planif,
	conectar_esi_planif,
	sentencia_coordinador,
	nuevo_esi,
	esi_listo_para_ejecutar, //planif -> esi (cuando ya esta en cola listos)
	instancia_desconectada,
	ejecutar_proxima_sentencia,
	ejecutar_sentencia_coordinador,
	preguntar_recursos_planificador,
	recurso_disponible,
	ejecucion_ok,
	ejecucion_bloqueado,
	ejecucion_no_ok,
	no_hay_mas_sentencias, //el esi le avisa al coordinador
	terminar_esi, //coord -> planif
	config_inst, //coord -> inst
	ejecutar_sentencia_instancia, //envia SET o STORE a la instancia
	compactar,
	compactacion_ok,
	esi_bloqueado,
	error_sentencia,
	resultado_ejecucion,
	cerrar_conexion_coord,
	cerrar_conexion_esi,
	podes_seguir,
	clave_guardada_en_instancia, //coord -> planif
	consulta_simulacion,
	inicializacion_instancia,
	verificar_conexion, //coordi checkea que la instancia este conectada
	esi_matado,
	aviso_bloqueo_clave,
	abortar_esi
} Accion;

typedef struct {
	int socket;
	int entradas_disponibles;
} instancia_Estado_Conexion;

#endif


