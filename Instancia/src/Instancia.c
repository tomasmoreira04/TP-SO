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
#include "commons/bitarray.h"

ConfigInstancia config;

//ESTRUCTURAS----
t_dictionary* tablaEntradas;
t_bitarray* disponibles;


//VARIABLES GLOBALES----
char* storage;
int32_t cantEntradas;
int32_t tamEntrada;
int32_t cantEntradasDisp;
ConfigInstancia config;

int main(int argc, char* argv[]) {
	char* nombre = argv[1];
	config = cargar_config_inst(nombre);

	int socketServer = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador); //usar conf->puerto_coordinador

	//enviar nombre
	handShake(socketServer, instancia);

	//------------RECIBIR DIMENSIONES Y CREAR STORAGE

	int* dim;

	enviarMensaje(socketServer, 8, &config.nombre_instancia , 20);

		if (config_inst == recibirMensaje(socketServer,(void*)&dim)){

			memcpy(&cantEntradas,dim,sizeof(int));
			memcpy(&tamEntrada,dim+1,sizeof(int));
			free(dim);

			printf(CYAN "\n------------INSTANCIA------------\n");
			printf("\nCANT ENTRADAS: %i \nTAM ENTRADAS: %i \n",cantEntradas, tamEntrada);


			cantEntradasDisp = cantEntradas;
			storage = malloc(sizeof(char)*cantEntradas*tamEntrada);

		} else {
			printf(RED "\nFATAL ERROR AL RECIBIR CONFIG DEL COORDINADOR\n");
			exit(0);
			}
	//------------------------------------------------------

	//-------------INICIALIZAR

	char* bitarray = malloc(sizeof(char)*cantEntradas);
	tablaEntradas = dictionary_create();

	disponibles = bitarray_create(bitarray,cantEntradas);
	limpiarArray(0,cantEntradas);
	printf(GREEN "\nEsperando ordenes pacificamente...\n");

	//--------------------------------------

	//------------INGORAR..PRUEBA (?

	/*
	t_sentencia* sentencia = malloc(sizeof(sentencia));
		strcpy(sentencia->clave,"K400");
		sentencia->id_esi=3;
		sentencia->tipo=S_STORE;
		strcpy(sentencia->valor,"H000000H");

		almacenarValor(sentencia->clave, sentencia->valor);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia->clave);
	*/

	//--------------------------------

	//------------RECIBIR MENSAJES

		while(1){
			void* stream;
			Accion accion = recibirMensaje(socketServer, &stream);

			switch(accion){
				case ejecutar_sentencia_instancia:
					ejecutarSentencia((t_sentencia*)stream);
					avisar(socketServer, ejecucion_ok);
					break;

				case compactar:
					break;
			}
			if (accion == error) {
				printf("\nse desconecto el coordinador\n");
				break;
			}
		}

	//---------------------------------------------------

	bitarray_destroy(disponibles);
	free(storage);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

//------PARA PRUEBAS NOMAS
void mostrarArray(char* bitarray){
	int i;
	for(i=0;i<cantEntradas;i++){
		int bit = bitarray_test_bit(disponibles,i);
		printf("%d ", bit);
	}
}

void mostrarValor(char* clave){
	Reg_TablaEntradas* registro;
	registro = dictionary_get(tablaEntradas,clave);

	char* valor=malloc(sizeof(registro->tamanio+1));
	memcpy(valor, storage+(tamEntrada*registro->entrada),registro->tamanio);

	printf("\nEl valor de la clave %s es: %s",clave,valor);
	printf("\n");
	free(valor);
}


//------------------------

void ejecutarSentencia(t_sentencia* sentencia){

	switch(sentencia->tipo){

	case S_SET:
		almacenarValor(sentencia->clave,sentencia->valor);
		printf("\nSe ejecuto un SET correctamente\n");
		break;

	case S_STORE:
		persistirValor(sentencia->clave);
		printf("\nSe ejecuto un STORE correctamente\n");
		break;
	}
}

void persistirValor(char* clave){

	char* path = malloc(string_length(clave)+strlen(config.punto_montaje)+5);
	strcpy(path,config.punto_montaje);
	strcat(path,clave);
	strcat(path,".txt");



	char* valor = devolverValor(clave);

	FILE* arch = fopen(path,"w+");

		fputs(valor,arch);

	fclose(arch);

	//free(valor);
	free(path);
}

char* devolverValor(char* clave){								//devuelve valor acordarse de liberarlo
	Reg_TablaEntradas* registro;
	registro = dictionary_get(tablaEntradas,clave);

	char* valor=malloc(sizeof(char)*registro->tamanio);
	memcpy(valor, storage+(tamEntrada*registro->entrada),registro->tamanio);

	return valor;
}


void almacenarValor(char* clave, char* valor){

	int tamEnBytes = string_length(valor)+1;
	int tamEnEntradas = 1+((tamEnBytes-1)/tamEntrada);			//supuesto redondeo para arriba

	if(dictionary_has_key(tablaEntradas,clave)){
		liberarEntradas(clave);
	}

	if(tamEnEntradas <= cantEntradasDisp){

		int posInicialLibre = buscarEspacioLibre(tamEnEntradas);
		memcpy(storage+(tamEntrada*posInicialLibre),valor,tamEnBytes);

		Reg_TablaEntradas* registro = malloc(sizeof(Reg_TablaEntradas));
		registro->entrada = posInicialLibre;
		registro->tamanio = tamEnBytes;
		memcpy(registro->clave,clave,40);
		dictionary_put(tablaEntradas,clave,registro);
		cantEntradasDisp-=tamEnEntradas;

	} else {
		//se tiene que reemplazar

	}

}

void liberarEntradas(char* clave){
	Reg_TablaEntradas* registro  = dictionary_remove(tablaEntradas,clave);
	int tamEnEntradas = 1+((registro->tamanio-1)/tamEntrada);
	int desde= registro->entrada;
	int hasta= registro->entrada+tamEnEntradas;
	limpiarArray(desde,hasta);
	cantEntradasDisp+=tamEnEntradas;
}

void limpiarArray(int desde, int hasta){
	int i;
	for(i=desde;i<hasta;i++){
		bitarray_clean_bit(disponibles,i);
	}
}


int buscarEspacioLibre(int entradasNecesarias){
	int posInicialLibre=0, i,contador=0,compactado=0;

	do{
	for(i=0;i<disponibles->size && contador<entradasNecesarias;i++){
		if (!bitarray_test_bit(disponibles,i)){
			contador++;}										//busca espacios libres CONTIGUOS
		else{
			contador=0;
			posInicialLibre=i+1;}
	}

	if (contador<entradasNecesarias){							//Compacta en caso de ser necesario (porque hay espacios pero no son contiguos)
		//compactar();
		compactado=0;
	}else{
		compactado=1;
	}

	}while(!compactado);

	for(i=posInicialLibre;i<posInicialLibre+contador;i++){
		bitarray_set_bit(disponibles,i);						//ocupa las entradas
	}

	return posInicialLibre;
}






