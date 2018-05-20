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
#include "Instancia.h"


int main() {
	ConfigInstancia config = cargar_config_inst();


	int socketServer = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador); //usar conf->puerto_coordinador
	handShake(socketServer, instancia);


	Dimensiones_Inst* dimensiones = malloc(sizeof(Dimensiones_Inst));
	int accion = recibirMensaje(socketServer,&dimensiones);

	printf("\n------INSTANCIA------\n");
	printf("\nCANT ENTRADAS: %i \nTAM ENTRADAS: %i \n",dimensiones->cant_entradas, dimensiones->tam_entradas);

	///asignar memoria


	free(dimensiones);
	return 0;
}

