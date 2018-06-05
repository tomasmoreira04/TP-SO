#ifndef COORDINADOR_H_
#define COORDINADOR_H_

#define LOG_PATH "../Log.txt"
#include "commons/config.h"
#include "commons/log.h"

typedef struct{
	int32_t cant_entradas;
	int32_t tam_entradas;
} Dimensiones_Inst;

typedef struct{
	int inst_ID;
	int socket;
}Nodo_Instancia;

t_log * log_operaciones;

void conectar_con_planificador(int planificador);
void mostrar_por_pantalla_config(ConfigCoordinador config);
void crear_log_operacion();
void destruir_log_operacion();
void mostrar_archivo(char* path);
void guardar_en_log(int id_esi, char* sentencia);
void *rutina_instancia(void * arg);
void *rutina_ESI(void * arg);
void crear_hilo(int nuevo_socket, int modulo);

void configurar_instancia(int socket,int id);

#endif /* COORDINADOR_H_ */
