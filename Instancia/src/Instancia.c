#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../../Bibliotecas/src/Estructuras.h"
#include "../../Bibliotecas/src/Socket.c"
#include "../../Bibliotecas/src/Configuracion.c"
#include "commons/string.h"
#include "commons/config.h"
#include "commons/collections/dictionary.h"
#include "Instancia.h"

t_dictionary* tablaEntradas;

char* storage;
int32_t cantEntradas;
int32_t tamEntradas;
int32_t cantEntradasDisp;

int main() {
	ConfigInstancia config = cargar_config_inst();

	int socketServer = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador); //usar conf->puerto_coordinador
	handShake(socketServer, instancia);
	tablaEntradas = dictionary_create();
	t_sentencia sentencia;


	int* dim;

	if (config_inst == recibirMensaje(socketServer,(void*)&dim)){
		memcpy(&cantEntradas,dim,sizeof(int));
		memcpy(&tamEntradas,dim+1,sizeof(int));

		printf(BLUE "\n------INSTANCIA------\n");
		printf(BLUE "\nCANT ENTRADAS: %i \nTAM ENTRADAS: %i \n",cantEntradas, tamEntradas);

		cantEntradasDisp = cantEntradas;
		storage = malloc(cantEntradas*tamEntradas);
	} else {
		printf(RED "\nFATAL ERROR AL CONFIGURAR INSTANCIA\n");
		exit(0);
		}

	while(ejecutar_sentencia_instancia == recibirMensaje(socketServer,(void*)&sentencia)){

		switch(sentencia.tipo){

		case S_SET:
			if(dictionary_has_key(tablaEntradas,sentencia.clave)){
				//proximamente
				}else{
				//almacenar nuevo
				}
			break;

		case S_STORE:
			break;
		}

	}

	return 0;
}

void almacenarNuevo(char* clave, char* valor, int tamEnBytes){

	int tamEnEntradas = 1+((tamEnBytes-1)/tamEntradas);

	if(tamEnEntradas<=cantEntradasDisp){

		int bytesOcupados = (cantEntradas - cantEntradasDisp)*tamEntradas;
		memcpy(storage+bytesOcupados,valor,tamEntradas*tamEnEntradas);

		Reg_TablaEntradas* registro = malloc(sizeof(Reg_TablaEntradas));

		registro->entrada = storage+bytesOcupados;
		registro->tamanio = tamEnEntradas;

		cantEntradasDisp-=tamEnEntradas;
		dictionary_put(tablaEntradas,clave,registro);

	} else{

		//algoritmo reemplazo

		}
}




