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
	char* inst_ID;
	int socket;
	int entradas_desocupadas;
}Nodo_Instancia;

t_log* log_operaciones;

void conectar_con_planificador(int planificador);
void mostrar_por_pantalla_config(ConfigCoordinador config);
void crear_log_operacion();
void destruir_log_operacion();
void mostrar_archivo(char* path);
void guardar_en_log(int id_esi, char* sentencia);
void *rutina_instancia(void * arg);
void *rutina_ESI(void * arg);
void crear_hilo(int nuevo_socket, int modulo);
int buscarEnLista(int valor);
void equitative_load(char* claveSentencia);
void contador_EQ(int cantidadDeInstancias);//ASIGNA EL EQ AL CUAL TIENE QUE ASIGNAR

void least_space_used(char*);

void key_explicit(char* claveSentencia);

char* formatear_mensaje_esi(int, TipoSentencia, char*, char*);
void configurar_instancia(int socket);
void destruir_estructuras_globales();
char* aplicar_algoritmo(char* clave, char* valor);
int clave_tiene_instancia(char* clave);
void avisar_guardado_planif(char* instancia, char* clave);

int estadoDeInstancia(char * instancia);//DEVUELVE EL ESTADO DE LA INSTANCIA A LA CUAL SE INTENTA GUARDAR



#endif /* COORDINADOR_H_ */
