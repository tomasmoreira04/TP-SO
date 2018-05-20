#ifndef COORDINADOR_H_
#define COORDINADOR_H_

#define MAX_ALG 4
#define LOG_PATH "../Log.txt"
#include "commons/config.h"

typedef struct{
	int32_t cant_entradas;
	int32_t tam_entradas;
} Dimensiones_Inst;

typedef struct{
	int inst_ID;
	int socket;
}Nodo_Instancia;


void mostrar_por_pantalla_config(ConfigCoordinador config);
void crear_log_operacion();
void mostrar_archivo(char* path);
void guardar_en_log(int id_esi, char* sentencia);
void *rutina_instancia(void * arg);
void *rutina_ESI(void * arg);
void crear_hilo(int nuevo_socket, int modulo);
int enviar(char* mensaje, int socket);

void configurar_instancia(int socket);

#endif /* COORDINADOR_H_ */
