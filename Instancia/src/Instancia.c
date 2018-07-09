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
		strcpy(sentencia->valor,"AAAAAAA");

		ejecutarSentencia(sentencia);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia->clave);
		mostrarListaReemplazos(reemplazos);
		free(sentencia);

	t_sentencia* sentencia2 = malloc(sizeof(t_sentencia));
		strcpy(sentencia2->clave,"K500");
		sentencia2->id_esi=3;
		sentencia2->tipo=S_SET;
		strcpy(sentencia2->valor,"BBBBBBBB");

		ejecutarSentencia(sentencia2);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia2->clave);
		mostrarListaReemplazos(reemplazos);
		free(sentencia2);

	t_sentencia* sentencia3 = malloc(sizeof(t_sentencia));
		strcpy(sentencia3->clave,"K500");
		sentencia3->id_esi=3;
		sentencia3->tipo=S_SET;
		strcpy(sentencia3->valor,"CCCCCCCCC");

		ejecutarSentencia(sentencia3);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia3->clave);
		mostrarListaReemplazos(reemplazos);
		free(sentencia3);

	t_sentencia* sentencia4 = malloc(sizeof(t_sentencia));
		strcpy(sentencia4->clave,"K700");
		sentencia4->id_esi=3;
		sentencia4->tipo=S_SET;
		strcpy(sentencia4->valor,"DDDD");

		ejecutarSentencia(sentencia4);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia4->clave);
		mostrarListaReemplazos(reemplazos);
		free(sentencia4);


	t_sentencia* sentencia5 = malloc(sizeof(t_sentencia));
		strcpy(sentencia5->clave,"K800");
		sentencia5->id_esi=3;
		sentencia5->tipo=S_SET;
		strcpy(sentencia5->valor,"UUUU");

		ejecutarSentencia(sentencia5);

		mostrarArray(disponibles->bitarray);
		mostrarValor(sentencia5->clave);
		mostrarListaReemplazos(reemplazos);
		free(sentencia5);

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

				default:
					printf(RED "\nError!\n"RESET );
					break;
			}

			if(accion == error){
				printf(RED "\nSe desconecto el coordinador!\n"RESET);
				break;
			}
		}

	//---------------------------------------------------

	free(bitarray);
	destruirlo_todo();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

//-------------PARA PRUEBAS NOMAS-------------
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

void mostrarListaReemplazos(t_list* list){
	int i;
	printf("Lista de reemplazos:\n");
	for(i=0;i<list_size(list);i++){
	Nodo_Reemplazo* nodo = list_get(list,i);
	printf("%s t.ref= %d\n",nodo->clave, nodo->ultimaRef);
	}

}
//--------------DESTROYERS-------------------

void nodoRempDestroyer(Nodo_Reemplazo* nodo){
	free(nodo->clave);
	free(nodo);
}

void regTablaDestroyer(Reg_TablaEntradas* registro){
	free(registro->clave);
	free(registro);
}

//--------------MANEJO LISTAS-------------------

t_list* duplicarLista(t_list* self) {
	t_list* duplicated = list_create();
	int i;
	for(i=0;i<list_size(self);i++){
		Nodo_Reemplazo* nodo = list_get(self,i);

		Nodo_Reemplazo* nodo2 = malloc(sizeof(Nodo_Reemplazo));
		nodo2->clave = strdup(nodo->clave);
		nodo2->tamanio = nodo->tamanio;
		nodo2->ultimaRef = nodo->ultimaRef;
		list_add(duplicated,nodo2);
	}

	return duplicated;

}

bool comparadorMayorTam(Nodo_Reemplazo* nodo1, Nodo_Reemplazo* nodo2){
	return (nodo1->tamanio>=nodo2->tamanio);
}

bool comparadorMayorTiempo(Nodo_Reemplazo* nodo1, Nodo_Reemplazo* nodo2){
	return (nodo1->ultimaRef>=nodo2->ultimaRef);
}


void eliminarDeListaRemp(t_list* listaEliminar){
	int i,j;

	for(i=0;i<list_size(listaEliminar);i++){
	Nodo_Reemplazo* nodo = list_get(listaEliminar,i);

		for(j=0;j<list_size(reemplazos);j++){
		Nodo_Reemplazo* nodo2 = list_get(reemplazos,j);
			if(string_equals_ignore_case(nodo->clave,nodo2->clave)){
			list_remove_and_destroy_element(reemplazos,j,(void*) nodoRempDestroyer);
			}
		}

	}
}

void aumentarTiempoRef(){
	int i;
	for(i=0;i<list_size(reemplazos);i++){
		Nodo_Reemplazo* remp = list_get(reemplazos,i);
		remp->ultimaRef+=1;
	}
}

int buscarNodoReemplazo(char* clave){
	int i;
	int  IndexBuscado=-1;

	for(i=0;i<list_size(reemplazos);i++){
		Nodo_Reemplazo* remp = list_get(reemplazos,i);
		if (string_equals_ignore_case(remp->clave,clave))
			IndexBuscado = i;
	}
	return IndexBuscado;
}

//---------------------------------------------------------

void ejecutarSentencia(t_sentencia* sentencia){

	switch(sentencia->tipo){

	case S_SET:
		aumentarTiempoRef();
		almacenarValor(sentencia->clave,sentencia->valor);
		printf(GREEN "\nSe ejecuto un SET correctamente\n"RESET);
		break;

	case S_STORE:
		aumentarTiempoRef();
		persistirValor(sentencia->clave);
		printf(GREEN "\nSe ejecuto un STORE correctamente\n"RESET);
		break;

	default:
		printf(RED "\nTipo de sentencia no valida!\n" RESET);
		exit(0);
		break;
	}
}

void almacenarValor(char* clave, char* valor){

	int tamEnBytes = string_length(valor)+1;
	int tamEnEntradas = 1+((tamEnBytes-1)/tamEntrada);			//redondeo para arriba

	if(dictionary_has_key(tablaEntradas,clave))
		liberarEntradas(clave);


	if(tamEnEntradas <= cantEntradasDisp){

		int posInicialLibre = buscarEspacioLibre(tamEnEntradas);
		strcpy(storage+(tamEntrada*posInicialLibre),valor);

		Reg_TablaEntradas* registro = malloc(sizeof(Reg_TablaEntradas));
		registro->entrada = posInicialLibre;
		registro->tamanio = tamEnBytes;
		registro->clave = malloc(strlen(clave)+1);
		strcpy(registro->clave,clave);
		dictionary_put(tablaEntradas,clave,registro);
		cantEntradasDisp-=tamEnEntradas;

		if (tamEnEntradas == 1){								//Si el valor es atomico, se selecciona como valor de reemplazo
			Nodo_Reemplazo* remp = malloc(sizeof(Nodo_Reemplazo));
			remp->clave = strdup(clave);
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
}


t_list* reemplazoSegunAlgoritmo(int cantNecesita){

	t_list* seleccionados;
	t_list* duplicada;

	switch(config.algoritmo_reemp){

	case CIRC:
		seleccionados = list_take_and_remove(reemplazos,cantNecesita);
		break;

	case LRU:
		duplicada = duplicarLista(reemplazos);
		list_sort(duplicada,(void*)comparadorMayorTiempo);
		seleccionados = list_take_and_remove(duplicada,cantNecesita);
		eliminarDeListaRemp(seleccionados);

		list_clean_and_destroy_elements(duplicada,(void*) nodoRempDestroyer);
		free(duplicada);
		break;

	case BSU:
		duplicada = duplicarLista(reemplazos);
		list_sort(duplicada,(void*)comparadorMayorTam);
		seleccionados = list_take_and_remove(duplicada,cantNecesita);
		eliminarDeListaRemp(seleccionados);

		list_clean_and_destroy_elements(duplicada,(void*) nodoRempDestroyer);
		free(duplicada);
		break;
	}

	return seleccionados;
}

void reemplazarValor(char* clave, char* valor, int tamEnEntradas){

	t_list* paraReemplazar = reemplazoSegunAlgoritmo (tamEnEntradas);

	int i;
	for(i=0;i<list_size(paraReemplazar);i++){
	Nodo_Reemplazo* remp = list_get(paraReemplazar,i);
	liberarEntradas(remp->clave);
	}

	list_clean_and_destroy_elements(paraReemplazar,(void*) nodoRempDestroyer);
	free(paraReemplazar);
	almacenarValor(clave,valor);
}


char* devolverValor(char* clave){								//devuelve valor acordarse de liberarlo
	Reg_TablaEntradas* registro;
	registro = dictionary_get(tablaEntradas,clave);

	char* valor=malloc(sizeof(char)*registro->tamanio);
	strcpy(valor,storage+(tamEntrada*registro->entrada));

	return valor;
}


void liberarEntradas(char* clave){
	Reg_TablaEntradas* registro  = dictionary_remove(tablaEntradas,clave);
	int tamEnEntradas = 1+((registro->tamanio-1)/tamEntrada);
	int desde= registro->entrada;
	int hasta= registro->entrada+tamEnEntradas;
	limpiarArray(desde,hasta);
	cantEntradasDisp+=tamEnEntradas;

	if(buscarNodoReemplazo(clave)>=0)
		list_remove_and_destroy_element(reemplazos,buscarNodoReemplazo(clave),(void*)nodoRempDestroyer);

	regTablaDestroyer(registro);
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
	dictionary_destroy_and_destroy_elements(tablaEntradas,(void*)regTablaDestroyer);
	list_clean_and_destroy_elements(reemplazos,(void*)nodoRempDestroyer);						//elimina elementos de la lista
	free(reemplazos); 																			//elimina la lista en si
}





