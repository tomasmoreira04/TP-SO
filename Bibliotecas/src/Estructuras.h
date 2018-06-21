#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include "Macros.h"
#include <commons/collections/list.h>

typedef enum { S_GET, S_SET, S_STORE } TipoSentencia;

typedef struct {
	int id;
	float estimacion_anterior; //la inicial esta dada por arch de config
	int rafagas_totales;
	int tiempo_esperado; //para hrrn
	float rafagas_restantes; //para sjf
	float response_ratio; //para hrrn
	t_list* claves;
	t_list* cola_actual;
	int posicion; //en la cola, en vez de filtrar la lista, ya se el indice -> mas rapido
	int socket_planif;
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
	podes_seguir,
	clave_guardada_en_instancia //coord -> planif

} Accion;

typedef enum {
	conectada = 1,
	desconectada=2,
	chingada=3

} estado_de_la_instancia;


typedef struct {
	int socket;
	estado_de_la_instancia estadoConexion;//1 Conectado, 2 Se Chingo la wea y se fue al carajo
} instancia_Estado_Conexion;

#endif


