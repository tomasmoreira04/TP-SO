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

void imprimir_cfg_en_log();

void conectar_con_planificador(int planificador);
void mostrar_por_pantalla_config(ConfigCoordinador config);
void crear_log_operaciones();
void destruir_log_operaciones();
void mostrar_archivo(char* path);
void guardar_en_log(int id_esi, char* sentencia);
void *rutina_instancia(void * arg);
void *rutina_ESI(void * arg);
void crear_hilo(int nuevo_socket, int modulo);
int buscarEnLista(int valor);
void equitative_load(char* claveSentencia);
void contador_EQ(int cantidadDeInstancias);//ASIGNA EL EQ AL CUAL TIENE QUE ASIGNAR
void imprimir_sentencia(t_sentencia sentencia);
void least_space_used(char*);
char* buscar_instancia(char* clave);

void key_explicit(char* claveSentencia);

void cambiarEstadoInstancia(char *instanciaGuardada,estado_de_la_instancia accion);
void nueva_instancia(int socket, char* nombre);
int existe_instancia(char* nombre);
char* formatear_mensaje_esi(int, TipoSentencia, char*, char*);
void configurar_instancia(int socket);
void imprimir_cfg_en_log();
void destruir_estructuras_globales();
char* aplicar_algoritmo(t_sentencia sentencia);
int clave_tiene_instancia(char* clave);
char* instancia_con_mas_espacio();
void avisar_guardado_planif(char* instancia, char* clave);
void actualizar_instancia(char* instancia, int valor);
t_list* lista_instancias_activas();
int estadoDeInstancia(char * instancia);//DEVUELVE EL ESTADO DE LA INSTANCIA A LA CUAL SE INTENTA GUARDAR
void nodo_inst_conexion_destroyer(instancia_Estado_Conexion* inst);

//abstracciones
void procesar_pedido_instancia(Accion operacion, char* instancia, int esi);
void realizar_sentencia(t_sentencia sentencia);
void procesar_permiso_planificador(Accion mensaje, t_sentencia sentencia, int socket_esi);

void esperar_compactacion();
void* avisar_compactacion();
void* rutina_compactacion(void* sock);
void hilo_compactacion(int socket_instancia);
void hilo_avisar_compactacion();

//simulaciones planificador
void* rutina_consulta(void* argumento);
char* simular_algoritmo(char* clave);
char* equitative_load_simulado(char* clave);
char* key_explicit_simulado(char* clave);
char* least_space_used_simulado(char* clave);
int existe_clave(char* clave);

#endif /* COORDINADOR_H_ */
