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
#include "Instancia.h"

#include "commons/string.h"
#include "commons/config.h"
#include "commons/bitarray.h"
#include "commons/collections/dictionary.h"
#include "commons/collections/list.h"



ConfigInstancia config;

//ESTRUCTURAS----
t_dictionary* tablaEntradas;
t_bitarray* disponibles;
t_list* reemplazos;


//VARIABLES GLOBALES----
char* storage;
int32_t cantEntradas;
int32_t tamEntrada;
int32_t cantEntradasDisp;
ConfigInstancia config;

int main(int argc, char* argv[]) {
	char* nombre = argv[1];
	config = cargar_config_inst(nombre);

	int socketServer = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador);

	handShake(socketServer, instancia);
	enviarMensaje(socketServer, 8, &config.nombre_instancia , 20);

	//------------RECIBIR DIMENSIONES Y CREAR STORAGE

		void* dim;

		if (config_inst == recibirMensaje(socketServer,&dim)){

			memcpy(&cantEntradas,(int*)dim,sizeof(int));
			memcpy(&tamEntrada,(int*)dim+1,sizeof(int));
			free(dim);

			printf(CYAN "\n------------ INSTANCIA ------------\n");
			printf("\nCANT ENTRADAS: %i \nTAM ENTRADAS: %i \n"RESET,cantEntradas, tamEntrada);


			cantEntradasDisp = cantEntradas;
			storage = malloc(sizeof(char)*cantEntradas*tamEntrada);

		} else {
			printf(RED "\nFATAL ERROR AL RECIBIR CONFIG DEL COORDINADOR\n"RESET);
			exit(0);
			}
	//------------------------------------------------------

	//-------------INICIALIZAR

	char* bitarray = malloc(sizeof(char)*cantEntradas);
	disponibles = bitarray_create(bitarray,cantEntradas);
	limpiarArray(0,cantEntradas);

	tablaEntradas = dictionary_create();
	reemplazos = list_create();




	printf(GREEN "\nEsperando ordenes pacificamente...\n"RESET);

	//--------------------------------------

	//------------INGORAR..PRUEBA (?

/*
	t_sentencia* sentencia = malloc(sizeof(t_sentencia));
		strcpy(sentencia->clave,"K400");
		sentencia->id_esi=3;
		sentencia->tipo=S_SET;
		strcpy(sentencia->valor,"AAAAAAAA");

		ejecutarSentencia(sentencia);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia->clave);
		mostrarListaReemplazos();

	t_sentencia* sentencia2 = malloc(sizeof(t_sentencia));
		strcpy(sentencia2->clave,"K500");
		sentencia2->id_esi=3;
		sentencia2->tipo=S_SET;
		strcpy(sentencia2->valor,"BBBBBBBB");

		ejecutarSentencia(sentencia2);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia2->clave);
		mostrarListaReemplazos();

	t_sentencia* sentencia3 = malloc(sizeof(t_sentencia));
		strcpy(sentencia3->clave,"K600");
		sentencia3->id_esi=3;
		sentencia3->tipo=S_SET;
		strcpy(sentencia3->valor,"CCCCCCC");

		ejecutarSentencia(sentencia3);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia3->clave);
		mostrarListaReemplazos();

	t_sentencia* sentencia4 = malloc(sizeof(t_sentencia));
		strcpy(sentencia4->clave,"K700");
		sentencia4->id_esi=3;
		sentencia4->tipo=S_SET;
		strcpy(sentencia4->valor,"DDDD");

		ejecutarSentencia(sentencia4);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia4->clave);
		mostrarListaReemplazos();

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


	destruirlo_todo();
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
	Reg_TablaEntradas* registro = dictionary_get(tablaEntradas,clave);

	char* valor=malloc((sizeof(char)*registro->tamanio)+1);
	memcpy(valor, storage+(tamEntrada*registro->entrada),registro->tamanio);

	printf("\nEl valor de la clave %s es: %s",clave,valor);
	printf("\n");
	free(valor);
}

void mostrarListaReemplazos(){

	int i;
	printf("Lista de reemplazos:\n");
	for(i=0;i<list_size(reemplazos);i++){
	Nodo_Reemplazo* nodo = list_get(reemplazos,i);
	printf("%s\n",nodo->clave);
	}

}

//------------------------

void ejecutarSentencia(t_sentencia* sentencia){

	switch(sentencia->tipo){

	case S_SET:
		almacenarValor(sentencia->clave,sentencia->valor);
		printf(GREEN "\nSe ejecuto un SET correctamente\n"RESET);
		break;

	case S_STORE:
		persistirValor(sentencia->clave);
		printf(GREEN "\nSe ejecuto un STORE correctamente\n"RESET);
		break;
	}
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

	if(dictionary_has_key(tablaEntradas,clave))
		liberarEntradas(clave);


	if(tamEnEntradas <= cantEntradasDisp){

		int posInicialLibre = buscarEspacioLibre(tamEnEntradas);
		memcpy(storage+(tamEntrada*posInicialLibre),valor,tamEnBytes);

		Reg_TablaEntradas* registro = malloc(sizeof(Reg_TablaEntradas));
		registro->entrada = posInicialLibre;
		registro->tamanio = tamEnBytes;
		memcpy(registro->clave,clave,40);
		dictionary_put(tablaEntradas,clave,registro);
		cantEntradasDisp-=tamEnEntradas;

		if (tamEnEntradas == 1){								//Si el valor es atomico, se selecciona como valor de reemplazo
			Nodo_Reemplazo* remp = malloc(sizeof(Nodo_Reemplazo));
			remp->clave =clave;
			remp->tamanio = tamEnBytes;
			remp->ultimaRef = 0;
			list_add(reemplazos,remp);
		}

	} else {

		if(tamEnEntradas<=list_size(reemplazos)){
			printf(YELLOW "\nEl valor de la clave %s reemplazara %d valor(es) existente(s)\n"RESET,clave,tamEnEntradas);
			reemplazarValor(clave,valor,tamEnEntradas);

		} else{
			printf(RED "\nNo existen suficientes entradas de reemplazo para ubicar valor de clave: %s!\n\n"RESET,clave);
			exit(0); 											//por ahora exit, pero enviar mensaje de fallo supongo
			return;
		}
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

	free(path);
	//free(valor); 			//PORQUE ROMPE CON ESTO?!
}


t_list* reemplazoSegunAlgoritmo(int tamEnEntradas){

	switch(config.algoritmo_reemp){

	case CIRC:
		return list_take_and_remove(reemplazos,tamEnEntradas);
		break;

	case LRU:
		return list_take_and_remove(reemplazos,tamEnEntradas);
		break;

	case BSU:
		return list_take_and_remove(reemplazos,tamEnEntradas);
		break;
	}

	return 0;
}

void reemplazarValor(char* clave, char* valor, int tamEnEntradas){

	t_list* paraReemplazar = reemplazoSegunAlgoritmo (tamEnEntradas);

	while(list_size(paraReemplazar)!=0){
	Nodo_Reemplazo* remp = (Nodo_Reemplazo*)list_remove(paraReemplazar,0);
	liberarEntradas(remp->clave);
	free(remp);
	}

	free(paraReemplazar);
	almacenarValor(clave,valor);
}


void liberarEntradas(char* clave){
	Reg_TablaEntradas* registro  = dictionary_remove(tablaEntradas,clave);
	int tamEnEntradas = 1+((registro->tamanio-1)/tamEntrada);
	int desde= registro->entrada;
	int hasta= registro->entrada+tamEnEntradas;
	limpiarArray(desde,hasta);
	cantEntradasDisp+=tamEnEntradas;

	free(registro);
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

void destruirlo_todo(){
	bitarray_destroy(disponibles);
	free(storage);
	dictionary_destroy(tablaEntradas);
	list_destroy(reemplazos);  				//NO LIBERA LOS ELEMENTOS, NECESITO EL DESTROYER QUE NO SE QUE ES
}





