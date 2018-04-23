
#include <commons/config.h>
#include <commons/string.h>

#define LARGO_CLAVE 20
#define MAX_CLAVES 50
#define LARGO_VALOR 20
#define LARGO_IP 16

typedef struct {
	char clave[LARGO_CLAVE];
	char valor[LARGO_VALOR];
} Clave;

typedef struct {
	int32_t puerto_escucha;
	char algoritmo_planificacion[6];
	int32_t estimacion_inicial;
	char ip_coordinador[LARGO_IP];
	int32_t puerto_coordinador;
	char* claves_bloqueadas[LARGO_CLAVE];
} Configuracion;

void* conectar_coordinador();
Configuracion* cargar_configuracion(char* ruta);
t_config* crear_prueba_configuracion(char* algoritmo_planificacion);
t_config* crear_archivo_configuracion(char* ruta, char** campos);
int estan_todos_los_campos(t_config* config, char** campos);
int parsear_algoritmo(char* valor);
Configuracion* leer_archivo_configuracion(t_config* archivo);
void separar_claves(char** claves);
void RecibirConecciones();
