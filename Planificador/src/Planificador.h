#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>

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

typedef struct {
	int32_t puerto_escucha;
	char algoritmo_planificacion[MAX_ALG];
	int32_t estimacion_inicial;
	char ip_coordinador[LARGO_IP];
	int32_t puerto_coordinador;
	char* claves_bloqueadas[LARGO_CLAVE];
} Configuracion;

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

void* conectar_coordinador();
Configuracion* cargar_configuracion(char* ruta);
t_config* crear_prueba_configuracion(char* algoritmo_planificacion);
t_config* crear_archivo_configuracion(char* ruta, char** campos);
int estan_todos_los_campos(t_config* config, char** campos);
int parsear_algoritmo(char* valor);
Configuracion* leer_archivo_configuracion(t_config* archivo);
void separar_claves(char** claves);
void RecibirConexiones();
void ingreso_cola_de_listos();
void movimiento_entre_estados(ESI* esi, int);
void cargar_datos_de_esi(ESI* esi);

int _es_esi(ESI* a, ESI* b);
void mover_esi(ESI* esi, t_list* nueva_lista);
t_list* lista_por_numero(int numero);
void inicializar_estructuras();
void destruir_estructuras();
void ejecutar_por_fifo();

