#ifndef COORDINADOR_H_
#define COORDINADOR_H_

#define MAX_ALG 4

typedef struct{
	int puerto_escucha;
	char algoritmo_distribucion[MAX_ALG];
	int cantidad_entradas;
	int tamanio_entradas;
	int retardo;
} Configuracion;

Configuracion* cargar_configuracion(char* ruta);
int estan_todos_los_campos(t_config* config, char** campos);
void mostrar_por_pantalla_config(Configuracion* config);
t_config* crear_archivo_configuracion(char* ruta, char** campos);
t_config* crear_prueba_configuracion(char* algoritmo_reemplazo);

#endif /* COORDINADOR_H_ */
