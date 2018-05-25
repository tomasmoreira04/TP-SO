#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include "Socket.h"
#include "Color.h"
#include "Configuracion.h"

ConfigPlanificador cargar_config_planificador() {
	char* campos[] = { "PUERTO_ESCUCHA", "ESTIMACION_INICIAL", "PUERTO_COORDINADOR", "ALFA_PLANIFICACION", "ALGORITMO_PLANIFICACION", "IP_COORDINADOR", "CLAVES_BLOQUEADAS"};
	t_config* config = cargar_archivo(planificador, campos);
	if (config == NULL)
		return config_predeterminada_planif();
	ConfigPlanificador configuracion;
	configuracion.puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
	configuracion.estimacion_inicial = config_get_int_value(config, "ESTIMACION_INICIAL");
	configuracion.puerto_coordinador = config_get_int_value(config, "PUERTO_COORDINADOR");
	configuracion.alfa_planif = config_get_int_value(config, "ALFA_PLANIFICACION");
	strcpy(configuracion.algoritmo_planif, config_get_string_value(config, "ALGORITMO_PLANIFICACION"));
	strcpy(configuracion.ip_coordinador, config_get_string_value(config, "IP_COORDINADOR"));
	configuracion.claves_bloqueadas = config_get_array_value(config, "CLAVES_BLOQUEADAS");
	config_destroy(config);
	return configuracion;
}

ConfigCoordinador cargar_config_coordinador() {
	char* campos[] = { "PUERTO_ESCUCHA","ALGORITMO_DISTRIBUCION", "CANTIDAD_ENTRADAS", "TAMANIO_ENTRADAS", "RETARDO", "IP_PLANIFICADOR", "PUERTO_PLANIFICADOR" };
	t_config* config = cargar_archivo(coordinador, campos);
	if (config == NULL)
		return config_predeterminada_coord();
	ConfigCoordinador configuracion;
	configuracion.puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
	configuracion.puerto_planificador = config_get_int_value(config, "PUERTO_PLANIFICADOR");
	configuracion.retardo = config_get_int_value(config, "RETARDO");
	configuracion.cant_entradas = config_get_int_value(config, "CANTIDAD_ENTRADAS");
	configuracion.tamanio_entrada = config_get_int_value(config, "TAMANIO_ENTRADAS");
	strcpy(configuracion.algoritmo_distrib, config_get_string_value(config, "ALGORITMO_DISTRIBUCION"));
	strcpy(configuracion.ip_planificador, config_get_string_value(config, "IP_PLANIFICADOR"));
	config_destroy(config);
	return configuracion;
}

ConfigESI cargar_config_esi() {
	char* campos[] = { "PUERTO_COORDINADOR", "PUERTO_PLANIFICADOR", "IP_COORDINADOR", "IP_PLANIFICADOR" };
	t_config* config = cargar_archivo(esi, campos);
	if (config == NULL)
		return config_predeterminada_esi();
	ConfigESI configuracion;
	configuracion.puerto_coordinador = config_get_int_value(config, "PUERTO_COORDINADOR");
	configuracion.puerto_planificador = config_get_int_value(config, "PUERTO_PLANIFICADOR");
	strcpy(configuracion.ip_planificador, config_get_string_value(config, "IP_PLANIFICADOR"));
	strcpy(configuracion.ip_coordinador, config_get_string_value(config, "IP_COORDINADOR"));
	config_destroy(config);
	return configuracion;
}

ConfigInstancia cargar_config_inst() {
	char* campos[] = { "IP_COORDINADOR","PUERTO_COORDINADOR","ALGORITMO_REEMPLAZO", "PUNTO_MONTAJE", "NOMBRE_INSTANCIA", "INTERVALO_DUMP" };
	t_config* config = cargar_archivo(instancia, campos);
	if (config == NULL)
		return config_predeterminada_inst();
	ConfigInstancia configuracion;
	configuracion.puerto_coordinador = config_get_int_value(config, "PUERTO_COORDINADOR");
	configuracion.intervalo_dump = config_get_int_value(config, "INTERVALO_DUMP");
	strcpy(configuracion.ip_coordinador, config_get_string_value(config,"IP_COORDINADOR"));
	strcpy(configuracion.algoritmo_reemp, config_get_string_value(config, "ALGORITMO_REEMPLAZO"));
	strcpy(configuracion.punto_montaje, config_get_string_value(config,"PUNTO_MONTAJE"));
	strcpy(configuracion.nombre_instancia, config_get_string_value(config, "NOMBRE_INSTANCIA"));
	config_destroy(config);
	return configuracion;
}

ConfigPlanificador config_predeterminada_planif() {
	ConfigPlanificador config;
	config.puerto_escucha = 9034;
	config.estimacion_inicial = 5;
	config.puerto_coordinador = 9035;
	config.alfa_planif = 50;
	strcpy(config.algoritmo_planif, "FIFO");
	strcpy(config.ip_coordinador, "127.0.0.1");
	config.claves_bloqueadas = NULL;
	return config;
}

ConfigCoordinador config_predeterminada_coord() {
	ConfigCoordinador config;
	config.puerto_escucha = 9035;
	config.puerto_planificador = 9034;
	strcpy(config.ip_planificador, "127.0.0.1");
	config.cant_entradas = 20;
	config.tamanio_entrada = 100;
	config.retardo = 300;
	strcpy(config.algoritmo_distrib, "EL");
	return config;
}

ConfigESI config_predeterminada_esi() {
	ConfigESI config;
	config.puerto_coordinador = 9035;
	config.puerto_planificador = 9034;
	strcpy(config.ip_coordinador, "127.0.0.1");
	strcpy(config.ip_planificador, "127.0.0.1");
	return config;
}

ConfigInstancia config_predeterminada_inst() {
	ConfigInstancia config;
	config.puerto_coordinador = 9035;
	config.intervalo_dump = 10;
	strcpy(config.algoritmo_reemp, "BSU");
	strcpy(config.punto_montaje, "/home/utnso/instancias/");
	strcpy(config.nombre_instancia, "Instancia");
	strcpy(config.ip_coordinador, "127.0.0.1");
	return config;
}

t_config* cargar_archivo(Modulo modulo, char** campos) {
	char* ruta = ruta_modulo(modulo);
	t_config* config = config_create(ruta);
	if(config == NULL || faltan_campos(config, campos)) {
		printf(YELLOW "\n¡ARCHIVO DE CONFIGURACIÓN INCORRECTO! (faltan campos o no existe el archivo)");
		printf("\nSe ha cargado la configuracion por default\n" RESET);
		return NULL;
	}
	printf(GREEN "\nConfiguración cargada con exito.\n" RESET);
	return config;
}

char* ruta_modulo(Modulo modulo) {
	if (modulo == planificador)	return "ConfigPlanificador.cfg";
	if (modulo == coordinador)	return "ConfigCoordinador.cfg";
	if (modulo == esi)			return "ConfigESI.cfg";
								return "ConfigInstancia.cfg";
}

int faltan_campos(t_config* config, char** campos) {
	for (int i = 0; i < config_keys_amount(config); i++)
		if(!config_has_property(config, campos[i]))
			return 1;
	return 0;
}
