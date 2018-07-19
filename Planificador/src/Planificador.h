#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H

#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include "../../Bibliotecas/src/Configuracion.h"
#include "../../Bibliotecas/src/Estructuras.h"
#include "../../Bibliotecas/src/Socket.h"
#include <pthread.h>

typedef enum {
	error_tamanio_clave,
	error_clave_no_identificada,
	error_comunicacion,
	error_clave_inaccesible,
	error_clave_no_bloqueada
} ErrorOperacion;

extern ConfigPlanificador config;
extern t_list* cola_de_listos;
extern t_list* cola_de_bloqueados;
extern t_list* cola_de_finalizados;
extern t_list* lista_claves_bloqueadas;
extern ESI* esi_ejecutando;
extern t_dictionary* estimaciones_actuales;
extern int ultimo_id;
extern int planificar; //parametro para pausar/continuar la planificacion por comando


//funciones del servidor
void recibir_conexiones();
t_list* claves_de_esi(ESI* esi);
int conectar_con_coordinador(int listener);
void recibir_mensajes(int socket, int listener, int coordinador);
void* procesar_mensaje_coordinador(void*);
void* procesar_mensaje_esi(void*);
void proceso_nuevo(t_nuevo_esi esi, int socket);
void nueva_sentencia(t_sentencia sentencia, int coordinador);
void crear_hilo(int nuevo_socket, Modulo modulo);

//funciones de logica de esi
void ejecutar_esi();
void esperar_que_exista(int id_esi);
ESI* obtener_esi(int id);
void bloquear_esi(ESI* esi);
void desbloquear_esi(ESI* esi);
t_clave* buscar_clave_bloqueada(const char* clave);
int esta_bloqueada(char* clave);
ESI* primero_llegado();
int liberar_clave(char* clave);
void nueva_solicitud_clave(char* clave, ESI* esi);
void bloquear_clave(const char* clave, ESI* esi);
void liberar_recursos(ESI* esi);
void finalizar_esi(int id_esi);
void finalizar_esi_ref(ESI* esi);
void STORE(char* clave, ESI* esi, int coordinador);
void SET(char* clave, char* valor, ESI* esi, int coordinador);
void GET(char* clave, ESI* esi, int coordinador);
void procesar_resultado(ResultadoEjecucion resultado);
void error_operacion(ErrorOperacion tipo, char* clave, int esi);
char* mensaje_error(ErrorOperacion tipo);
void imprimir_nuevo_esi(ESI* esi, char* nombre);
void calcular_response_ratios();
void bloquear_claves_iniciales(char** claves, int n);
void copiar_lista_strings(char** destino, char** claves, int n);
void actualizar_clave(t_clave respuesta);

void ingreso_cola_de_listos(ESI* esi);
void sumar_tiempo_hrrn();
void replanificar();
void movimiento_entre_estados(ESI* esi, int movimiento);
int hay_desalojo(AlgoritmoPlanif algoritmo);
void ejecutar(int desalojar);
int _es_esi(ESI* a, ESI* b);
void mover_esi(ESI* esi, t_list* nueva_lista);
int ver_disponibilidad_clave(char* clave);

t_list* lista_por_numero(int numero);
void inicializar_estructuras();
void destruir_estructuras();
void ejecutar_por_fifo();
void ejecutar_por_sjf();
void ejecutar_por_hrrn();
void sentencia_ejecutada();
float estimar(ESI* esi);
ESI* esi_rafaga_mas_corta();
ESI* esi_resp_ratio_mas_alto();

#endif
