#ifndef COORDINADOR_H_
#define COORDINADOR_H_


#define MAX_ALG 4
#define LOGPATH "../Log.txt"

typedef struct{
	int32_t puerto_escucha;
	char algoritmo_distribucion[MAX_ALG];
	int32_t cantidad_entradas;
	int32_t tamanio_entradas;
	int32_t retardo;
} Configuracion;

Configuracion* cargar_configuracion(char* ruta);
int estan_todos_los_campos(t_config* config, char** campos);
void mostrar_por_pantalla_config(Configuracion* config);
t_config* crear_archivo_configuracion(char* ruta, char** campos);
t_config* crear_prueba_configuracion(char* algoritmo_distribucion);
void crearLogOp();
void mostrarArchivo(char* path);
void guardarEnLog(int idEsi, char* sentencia);

#endif /* COORDINADOR_H_ */
