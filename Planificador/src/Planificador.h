#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include "../../Bibliotecas/src/Configuracion.h"
#include "../../Bibliotecas/src/Estructuras.h"

t_list* cola_de_listos;
t_list* cola_de_bloqueados;
t_list* cola_de_finalizados;
t_list* lista_claves_bloqueadas;
t_dictionary* estimaciones_actuales;

typedef enum {
	error_tamanio_clave,
	error_clave_no_identificada,
	error_comunicacion,
	error_clave_inaccesible,
	error_clave_no_bloqueada
} ErrorOperacion;

int ultimo_id;

//funciones del servidor
void recibir_conexiones();
int conectar_con_coordinador(int listener);
void recibir_mensajes(int socket, int listener, int coordinador);
void procesar_mensaje_coordinador(int coordinador);
void procesar_mensaje_esi(int socket);
void proceso_nuevo(int rafagas, int socket);
void nueva_sentencia(t_sentencia sentencia);

//funciones de logica de esi
void ejecutar_esi();
ESI* obtener_esi(int id);
void bloquear_esi(ESI* esi);
void desbloquear_esi(ESI* esi);
t_clave* buscar_clave_bloqueada(char* clave);
int esta_bloqueada(char* clave);
void liberar_clave(char* clave);
void nueva_solicitud_clave(char* clave, ESI* esi);
void bloquear_clave(char* clave, ESI* esi);
void liberar_recursos(ESI* esi);
void finalizar_esi(ESI* esi);
void STORE(char* clave, ESI* esi);
void SET(char* clave, char* valor, ESI* esi);
void GET(char* clave, ESI* esi);
void error_operacion(ErrorOperacion tipo, char* clave, int esi);
char* mensaje_error(ErrorOperacion tipo);

void ingreso_cola_de_listos(ESI* esi);
void replanificar();
void movimiento_entre_estados(ESI* esi, int movimiento);
int hay_desalojo(int algoritmo);
void ejecutar(char* algoritmo);
int numero_algoritmo(char* nombre);
int _es_esi(ESI* a, ESI* b);
void mover_esi(ESI* esi, t_list* nueva_lista);
t_list* lista_por_numero(int numero);
void inicializar_estructuras();
void destruir_estructuras();
void ejecutar_por_fifo();
void ejecutar_por_sjf();
void sentencia_ejecutada();
float estimar(ESI* esi, float alfa);
ESI* esi_rafaga_mas_corta();
int _es_mas_corto(ESI* a, ESI* b);
ESI* esi_resp_ratio_mas_corto();
