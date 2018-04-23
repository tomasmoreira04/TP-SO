#include "commons/config.h"

#ifndef ESI_H_
#define ESI_H_

#define LARGO_IP 16

typedef struct{
	char ip_planificador[LARGO_IP];
	int32_t puerto_planificador;
	char ip_coordinador[LARGO_IP];
	int32_t puerto_coordinador;

} Configuracion;

Configuracion* cargar_configuracion(char* ruta);
t_config* crear_prueba_configuracion();
t_config* crear_archivo_configuracion(char* ruta, char** campos);
int estan_todos_los_campos(t_config* config, char** campos);

#endif /* ESI_H_ */
