#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include "../../Bibliotecas/src/Configuracion.h"
#include "../../ESI/src/ESI.h"

#define LARGO_CLAVE 20
#define MAX_CLAVES 50
#define LARGO_VALOR 20
#define LARGO_IP 16
#define MAX_ALG 7

typedef struct {
	char clave[LARGO_CLAVE];
	char valor[LARGO_VALOR];
} Clave;

//hay que revisar si está bien esto
typedef struct {
	int id;
	int cola_actual;
	int estimacion_anterior; //la inicial esta dada por arch de config
	int rafaga_anterior;
	int cant_rafagas;
	char* claves[LARGO_CLAVE];
} ESI;

//POR AHORA VAN A SER LISTAS, luego vemos si lo metemos en otra estructura
t_list* cola_de_listos; //1
t_list* cola_de_bloqueados; //2
t_list* cola_de_finalizados; //3
t_list* lista_claves_bloqueadas;
t_dictionary* estimaciones_actuales;
////////////////////

enum movimientos_entre_estados {
	hacia_listos = 1,
	hacia_ejecutando = 2,
	hacia_bloqueado = 3,
	hacia_finalizado = 4
};

int ultimo_id;

void recibir_conexiones();
void procesar_instruccion_esi(int id_esi, t_esi_operacion mensaje);
void proceso_nuevo(ESI* esi, int mensaje);
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
