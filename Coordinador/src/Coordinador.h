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
} Nodo_Instancia;

t_log* log_operaciones;


void imprimir_cfg_en_log();

void imprimir_semaforo();
void logear_info(char* output);
void handler();
int enviar_check_conexion_instancia(int socket);
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

void eliminar_instancia(char* nombre_instancia);
void nueva_instancia(int socket, char* nombre);
int existe_instancia(char* nombre);
char* formatear_mensaje_esi(int, TipoSentencia, char*, char*);
void configurar_instancia(int socket);
void imprimir_cfg_en_log();
void finalizar_proceso();
void destruir_estructuras_globales();
char* aplicar_algoritmo(t_sentencia sentencia);
int clave_tiene_instancia(char* clave);
char* instancia_con_mas_espacio();
void avisar_guardado_planif(char* instancia, char* clave);

void actualizar_instancia(char* instancia, int socket);
void actualizar_entradas(char* instancia, int entradas);

void nodo_inst_conexion_destroyer(instancia_Estado_Conexion* inst);
int indice_instancia_por_nombre(char* nombre);
char* instancia_por_socket(int socket);
void reintentar_sentencia(t_sentencia sentencia);
void modificar_clave(char* clave, char* instancia);

//abstracciones
void procesar_pedido_instancia(Accion operacion, char* instancia, int esi);
void realizar_sentencia(t_sentencia sentencia);
void procesar_permiso_planificador(Accion mensaje, t_sentencia sentencia, int socket_esi);

void esperar_compactacion(int cantidad);
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
void destruir_lista(t_list* lista);

void imprimir_configuracion();
char* algoritmo(AlgoritmoCoord alg);

#endif /* COORDINADOR_H_ */
