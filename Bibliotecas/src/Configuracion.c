#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include "Socket.h"
#include "Color.h"
#include "Configuracion.h"

ConfigGlobal cargar_config_global() {
	char* campos[] = { "IP_PLANIFICADOR", "PUERTO_PLANIFICADOR", "IP_COORDINADOR", "PUERTO_COORDINADOR" };
	t_config* config = cargar_archivo("Conexion.cfg", campos);
	ConfigGlobal configuracion;
	configuracion.puerto_planificador = config_get_int_value(config, "PUERTO_PLANIFICADOR");
	configuracion.puerto_coordinador = config_get_int_value(config, "PUERTO_COORDINADOR");
	strcpy(configuracion.ip_coordinador, config_get_string_value(config, "IP_COORDINADOR"));
	strcpy(configuracion.ip_planificador, config_get_string_value(config, "IP_PLANIFICADOR"));
	config_destroy(config);
	return configuracion;
}

ConfigPlanificador cargar_config_planificador(char* ruta) {

	char* campos[] = { "ESTIMACION_INICIAL", "ALFA_PLANIFICACION", "ALGORITMO_PLANIFICACION", "CLAVES_BLOQUEADAS", "MOSTRAR_ESTIMACION"};
	t_config* config = cargar_archivo(ruta, campos);
	ConfigPlanificador configuracion;
	ConfigGlobal global = cargar_config_global();
	configuracion.puerto_escucha = global.puerto_planificador;
	strcpy(configuracion.ip_coordinador, global.ip_coordinador);
	configuracion.puerto_coordinador = global.puerto_coordinador;
	configuracion.estimacion_inicial = (float)config_get_double_value(config, "ESTIMACION_INICIAL");
	configuracion.alfa_planif = config_get_int_value(config, "ALFA_PLANIFICACION");
	configuracion.algoritmo = numero_algoritmo_p(config_get_string_value(config, "ALGORITMO_PLANIFICACION"));
	configuracion.claves_bloqueadas = config_get_array_value(config, "CLAVES_BLOQUEADAS");
	char* linea_con_claves = config_get_string_value(config, "CLAVES_BLOQUEADAS");
	configuracion.n_claves = numero_claves(linea_con_claves);
	configuracion.mostrar_estimacion = procesar_config_estimacion(config_get_string_value(config, "MOSTRAR_ESTIMACION"));
	config_destroy(config);
	return configuracion;
}

ConfigCoordinador cargar_config_coordinador(char* ruta) {
	char* campos[] = { "ALGORITMO_DISTRIBUCION", "CANTIDAD_ENTRADAS", "TAMANIO_ENTRADAS", "RETARDO" };
	t_config* config = cargar_archivo(ruta, campos);
	ConfigCoordinador configuracion;
	ConfigGlobal global = cargar_config_global();
	configuracion.puerto_escucha = global.puerto_coordinador;
	configuracion.puerto_planificador = global.puerto_planificador;
	strcpy(configuracion.ip_planificador, global.ip_planificador);
	configuracion.retardo = config_get_int_value(config, "RETARDO");
	configuracion.cant_entradas = config_get_int_value(config, "CANTIDAD_ENTRADAS");
	configuracion.tamanio_entrada = config_get_int_value(config, "TAMANIO_ENTRADAS");
	configuracion.algoritmo = numero_algoritmo_c(config_get_string_value(config, "ALGORITMO_DISTRIBUCION"));
	config_destroy(config);
	return configuracion;
}

ConfigESI cargar_config_esi() {
	ConfigESI configuracion;
	ConfigGlobal global = cargar_config_global();
	configuracion.puerto_coordinador = global.puerto_coordinador;
	configuracion.puerto_planificador = global.puerto_planificador;
	strcpy(configuracion.ip_planificador, global.ip_planificador);
	strcpy(configuracion.ip_coordinador, global.ip_coordinador);
	return configuracion;
}

ConfigInstancia cargar_config_inst(char* ruta) {
	char* campos[] = { "ALGORITMO_REEMPLAZO", "NOMBRE_INSTANCIA", "PUNTO_MONTAJE", "INTERVALO_DUMP", "MOSTRAR_STORAGE" };
	t_config* config = cargar_archivo(ruta, campos);
	ConfigInstancia configuracion;
	ConfigGlobal global = cargar_config_global();
	configuracion.puerto_coordinador = global.puerto_coordinador;
	strcpy(configuracion.ip_coordinador, global.ip_coordinador);
	configuracion.intervalo_dump = config_get_int_value(config, "INTERVALO_DUMP");
	configuracion.algoritmo_reemp = numero_algoritmo_i(config_get_string_value(config, "ALGORITMO_REEMPLAZO"));
	strcpy(configuracion.nombre_instancia, config_get_string_value(config, "NOMBRE_INSTANCIA"));
	strcpy(configuracion.punto_montaje, config_get_string_value(config,"PUNTO_MONTAJE"));
	configuracion.mostrar_storage = procesar_config_storage(config_get_string_value(config, "MOSTRAR_STORAGE"));
	config_destroy(config);
	return configuracion;
}

int procesar_config_storage(char* valor_campo) {
	return strcmp(valor_campo, "SI") == 0 ? 1 : 0;
}

int procesar_config_estimacion(char* valor_campo) {
	return strcmp(valor_campo, "SI") == 0 ? 1 : 0;
}

char* ruta_config(char* nombre) {
	char* ruta = malloc(LARGO_RUTA);
	strcpy(ruta, "../../Configuraciones/");
	strcat(ruta, nombre);
	return ruta;
}

void imprimir_error() {
	printf(YELLOW "\n¡ARCHIVO DE CONFIGURACIÓN INCORRECTO! (faltan campos o no existe el archivo)"RESET);
	exit(0);
}

t_config* cargar_archivo(char* nombre_archivo, char** campos) {
	char* ruta = ruta_config(nombre_archivo);
	t_config* config = config_create(ruta);
	free(ruta);
	if(config == NULL || faltan_campos(config, campos)) {
		imprimir_error();
		return NULL;
	}
	return config;
}

int faltan_campos(t_config* config, char** campos) {
	for (int i = 0; i < config_keys_amount(config); i++)
		if(!config_has_property(config, campos[i]))
			return 1;
	return 0;
}

AlgoritmoPlanif numero_algoritmo_p(char* nombre) {
	if (strcmp(nombre, "SJF-CD") == 0)
		return sjf_cd;
	if (strcmp(nombre, "SJF-SD") == 0)
		return sjf_sd;
	if (strcmp(nombre, "HRRN") == 0)
		return hrrn;
	return fifo;
}

AlgoritmoCoord numero_algoritmo_c(char* nombre) {
	if (strcmp(nombre, "KE") == 0)
		return ke;
	if (strcmp(nombre, "LSU") == 0)
		return lsu;
	return el;
}

AlgoritmoInst numero_algoritmo_i(char* nombre) {
	if (strcmp(nombre, "LRU") == 0)
		return LRU;
	if (strcmp(nombre, "BSU") == 0)
		return BSU;
	return CIRC;
}

int numero_claves(char* linea) { //estilo: [clave1,clave2,clave3]
	int comas = 0; //veces que aparece la coma
	int hay_clave = 0;
	for (int i = 0; linea[i] != '\0'; i++) {
		if (linea[i] == ',')
			comas++;
		if (linea[i] != '[' && linea[i] != ']')
			hay_clave = 1;
	}
	if (hay_clave)
		return comas + 1;
	return 0;
}
