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
#include "commons/collections/list.h"
#include "ESI.h"

int cantidadDeSentencias();

t_list *listaDeComandos;

int main() {
	ConfigESI config = cargar_config_esi();
	int socketPlanificador = conexion_con_servidor(config.ip_planificador, config.puerto_planificador);
	int socketCoordinador = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador);
	handShake(socketCoordinador, esi);
	//int* rafagas = malloc(sizeof(int));
	int rafagas = cantidadDeSentencias();
	//strcpy(rafagas, "noni");
	printf("\n\nLa cantidad de rafagas es de %d\n\n", rafagas);
	//char* aux=list_get(listaDeComandos,0);
	//te lo comento porque esto me tira seg fault
	//printf("\n\nLISTA CHEN: %s\n\n", aux);
	enviarMensaje(socketPlanificador,100,&rafagas,sizeof(int));//FALTA DEFINIR ACCION Y EL RECV
	//DESTRUIR LISTA
	return 0;
}

int cantidadDeSentencias(){
	//ARCHIVO
	FILE * f = fopen("Script.txt","r");
	char *valor=malloc(5);
	char *aux=malloc(5);
	char *contenidopri=malloc(200);
	strcpy(valor," ");
	strcpy(aux," ");
	strcpy(contenidopri,"");//TURBIO <- xD
	int i=0;
	int contador=0;
	//LISTA
	listaDeComandos=list_create();
	if (f==NULL)
			perror ("Error opening file");
    do
    {
    	*aux=*valor;
    	*valor = fgetc (f);

    	if (*valor == '\n'){
    		i++;
    		////contador++;
    		char *contenido=malloc(contador);
    		strcpy(contenido,contenidopri);
    		////printf("El contador es: %d \n",contador);
    		////fflush(stdin);
    		//printf("\nCONTENIDO: %s\n\n",contenido);
    		list_add(listaDeComandos, contenido);
    		strcpy(contenidopri,"");
    		//free(contenido);
    		contador=0;
    	}
    	else{
    		strcat(contenidopri,valor);
    		contador++;
    	}
    }
    while (*valor != EOF);
    if(*aux!='\n')  i++;
	fclose(f);
	free(aux);
	free(valor);
	free(contenidopri);
	return i;
}




