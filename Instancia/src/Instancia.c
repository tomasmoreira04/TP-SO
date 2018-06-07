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


int main(int argc, char* argv[]) {
	char* nombre = argv[1];
	ConfigInstancia config = cargar_config_inst(nombre);

	int socketServer = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador); //usar conf->puerto_coordinador

	//enviar nombre
	handShake(socketServer, instancia);

	//------------Recibir dimensiones y crear espacio de almacenamiento

	int id_instancia;
	int* dim;

		if (config_inst == recibirMensaje(socketServer,(void*)&dim)){

			memcpy(&cantEntradas,dim,sizeof(int));
			memcpy(&tamEntrada,dim+1,sizeof(int));
			memcpy(&id_instancia,dim+2,sizeof(int));
			free(dim);

			printf(BLUE "\n------INSTANCIA------\n");
			printf(BLUE "\nCANT ENTRADAS: %i \nTAM ENTRADAS: %i \n",cantEntradas, tamEntrada);

			cantEntradasDisp = cantEntradas;
			storage = malloc(sizeof(char)*cantEntradas*tamEntrada);

		} else {
			printf(RED "\nFATAL ERROR AL RECIBIR CONFIG DEL COORDINADOR\n");
			exit(0);
			}
	//------------------------------------------------------

	//-------------CREAR E INICIALIZAR ARRAY

	char* bitarray = malloc(sizeof(char)*cantEntradas);
	tablaEntradas = dictionary_create();

	disponibles = bitarray_create(bitarray,cantEntradas);
	limpiarArray(0,cantEntradas);

	//-----------------------------------------

	//------------RECIBIR MENSAJES

		while(1){
			t_sentencia* sentencia;
			Accion accion = recibirMensaje(socketServer,&sentencia);

			switch(accion){

				case ejecutar_sentencia_instancia:
					ejecutarSentencia(sentencia);
					break;

				case compactar:
					break;
			}
		}

	//---------------------------------------------------

	bitarray_destroy(disponibles);
	return 0;
}


void ejecutarSentencia(t_sentencia* sentencia){

	switch(sentencia->tipo){

	case S_SET:
		break;

	case S_STORE:
		break;

	}
}

void almacenarValor(char* clave, char* valor){

	int tamEnBytes = string_length(valor);

	int tamEnEntradas = 1+((tamEnBytes-1)/tamEntrada);			//supuesto redondeo para arriba

	if(dictionary_has_key(tablaEntradas,clave)){
		eliminarEntradas(clave);
	}

	if(tamEnEntradas <= cantEntradasDisp){

		int posInicialLibre = buscarEspacioLibre(tamEnEntradas);
		memcpy(storage+(tamEntrada*posInicialLibre),valor,tamEnBytes);

		Reg_TablaEntradas* registro = malloc(sizeof(Reg_TablaEntradas));
		registro->entrada = posInicialLibre;
		registro->tamanio = tamEnEntradas;
		dictionary_put(tablaEntradas,clave,registro);
		cantEntradasDisp-=tamEnEntradas;

	} else {
		//se tiene que reemplazar

	}

}

//esto esta para probar nomas
void mostrarArray(char* bitarray){
	int i;
	for(i=0;i<cantEntradas;i++){
		int bit = bitarray_test_bit(disponibles,i);
		printf("%d ", bit);
	}
}

void limpiarArray(int desde, int hasta){
	int i;
	for(i=desde;i<=hasta;i++){
		bitarray_clean_bit(disponibles,i);
	}
}

void eliminarEntradas(char* clave){
	Reg_TablaEntradas* registro  = dictionary_remove(tablaEntradas,clave);
	int desde= registro->entrada;
	int hasta= registro->entrada+registro->tamanio;
	limpiarArray(desde,hasta);
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










