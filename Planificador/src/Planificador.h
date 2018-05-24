#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include "../../Bibliotecas/src/Configuracion.h"
#include "../../Bibliotecas/src/Estructuras.h"

t_list* cola_de_listos;
t_list* cola_de_bloqueados;
t_list* cola_de_finalizados;
t_list* lista_claves_bloqueadas;
t_dictionary* estimaciones_actuales;

enum movimientos_entre_estados {
	hacia_listos = 1,
	hacia_ejecutando = 2,
	hacia_bloqueado = 3,
	hacia_finalizado = 4
};

int ultimo_id;

void recibir_conexiones();
int conectar_con_coordinador(int listener);
void recibir_mensajes(int socket, int listener, int coordinador);
void procesar_mensaje_coordinador(int coordinador);
void procesar_mensaje_esi(int socket);
void proceso_nuevo(int rafagas);
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
