#include <commons/config.h>

#ifndef INSTANCIA_H_
#define INSTANCIA_H_

#define LARGO_IP 16
#define MAX_ALG 5
#define MAX_RUTA 50
#define MAX_NOMBRE 20

typedef struct{
	char ip_coordinador[LARGO_IP];
	int puerto_coordinador;
	char algoritmo_reemplazo[MAX_ALG];
	char punto_montaje[MAX_RUTA];
	char nombre_instancia[MAX_NOMBRE];
	int intervalo_dump;
} Configuracion;


Configuracion* cargar_configuracion(char* ruta);
int estan_todos_los_campos(t_config* config, char** campos);
t_config* crear_archivo_configuracion(char* ruta, char** campos);
t_config* crear_prueba_configuracion(char* algoritmo_reemplazo);

#endif /* INSTANCIA_H_ */
