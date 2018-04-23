#include "Instancia.h"


Configuracion* leerArchivoConfiguracion(t_config* arch){
	Configuracion* conf = malloc(sizeof(Configuracion));
	strcpy(conf->ip_coordinador, config_get_string_value(arch,"IP_COORDINADOR"));
	conf->puerto_coordinador = config_get_int_value(arch, "PUERTO_COORDINADOR");
	strcpy(conf->algoritmo_reemplazo, config_get_string_value(arch, "ALGORITMO DE REEMPLAZO"));
	strcpy(conf->path, config_get_string_value(arch, "PATH"));
	strcpy(conf->nombre, config_get_string_value(arch, "NOMBRE"));
	conf->intervalo_dump = config_get_int_value(arch, "INTERVALO DE DUMP");
	return conf;
}




void main() {
	char * j=malloc(50);
	int socketServer = conexion_con_servidor("127.0.0.1","9034");
	while(1){
		int tamano=strlen(j);
		recv(socketServer,&tamano,sizeof(int),0);
		recv(socketServer,j,tamano,0);

		printf("\nel valor %s\n", j);
	}
}
