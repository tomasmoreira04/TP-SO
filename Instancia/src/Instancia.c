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

	int* dim;
	recibirMensaje(socketServer,(void*)&dim);
	memcpy(&cantEntradas,dim,sizeof(int));
	memcpy(&tamEntradas,dim+1,sizeof(int));

	printf("\n------INSTANCIA------\n");
	printf("\nCANT ENTRADAS: %i \nTAM ENTRADAS: %i \n",cantEntradas, tamEntradas);

	cantEntradasDisp = cantEntradas;

	storage = malloc(cantEntradas*tamEntradas);

	return 0;
}

void setValor(char* clave, char* valor, int tamEnBytes){

	if(dictionary_has_key(tablaEntradas,clave)){

		//proximamente

	}else{
		almacenarNuevo(clave,valor,tamEnBytes);
	}

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









