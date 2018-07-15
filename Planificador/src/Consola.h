#ifndef CONSOLA_H
#define CONSOLA_H

#include "../../Bibliotecas/src/Macros.h"
#include "../../Bibliotecas/src/Estructuras.h"

extern int file_descriptors[2];
extern FILE* outPlanif;

typedef enum {
	_pause,
	_continue,
	deadlock,
	unlock,
	list,
	_kill,
	status,
	block
} Comando;

typedef struct Operacion {
	Comando id;
	char* nombre;
	char* param1;
	char* param2;
} Operacion;

typedef struct {
	int esi_bloqueante;
	int esi_bloqueado;
	char clave_necesaria[LARGO_CLAVE];
} t_deadlock;


void iniciar_terminal();
void* iniciar_consola();
Operacion crear_operacion(char* linea);
Comando id_operacion(char* nombre);
int parametros_necesarios(Comando comando);
void validar_operacion(Operacion operacion);
void ejecutar_comando(Operacion comando);
void pausar_planificacion();
void continuar_planificacion();

t_deadlock* clave_que_necesita(ESI* a, ESI* b);
void imprimir_deadlock(t_deadlock* deadlock);
void mostrar_deadlocks();
t_list* buscar_deadlocks();


void desbloquear_clave(char* clave);
void listar_esis_recurso(char* clave);
void matar_esi(char* id);
void estado_clave(char* clave);
void bloquear_esi_en_clave(char* clave, char* id_esi);
void _imprimir_claves_esi(t_list* claves);
ESI* _obtener_esi(int id);
ESI* _buscar_esi(t_list* lista, int id);


char* consultar_simulacion(char* clave);
#endif
