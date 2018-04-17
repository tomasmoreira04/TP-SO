#include "ESI.h"


Configuracion* leerArchivoConfiguracion(t_config* archivo){
	Configuracion* arch = malloc(sizeof(Configuracion));
	strcpy(arch->ip_planificador, config_get_string_value(archivo,"IP_PLANIFICADOR"));
	arch->puerto_planificador = config_get_int_value(archivo, "PUERTO_PLANIFICADOR");
	strcpy(arch->ip_coordinador, config_get_string_value(archivo,"IP_COORDINADOR"));
	arch->puerto_coordinador = config_get_int_value(archivo,"PUERTO_COORDINADOR");
	return arch;
}

int estanTodosLosCampos(t_config* conf, char** campos){
	for(int i=0;i<config_keys_amount(conf);i++){
		if(!config_has_property(conf,campos[i])){
			return 0;
		}
	}
	return 1;
}

t_config* crearArchivoConfiguracion(char* path, char** campos){
	t_config* conf= config_create(path);
	if(!estanTodosLosCampos(conf, campos) || conf == NULL){
		puts("Archivo de Configuracion Invalido");
		exit(EXIT_FAILURE);
	}

	return conf;

}




void main() {
	char * j=malloc(50);
	int socketServer=conexionConServidor("127.0.0.1","9034");
	while(1){
		fflush(stdin);
		gets(j);
		int tamano=strlen(j)+1;
		printf("\ņ %s \n",j);
		send(socketServer,&tamano,sizeof(int),0);
		send(socketServer,j,tamano,0);
	}
}



