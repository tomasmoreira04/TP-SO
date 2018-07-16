#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../../Bibliotecas/src/Estructuras.h"
#include "../../Bibliotecas/src/Socket.c"
#include "../../Bibliotecas/src/Configuracion.c"
#include "../../Bibliotecas/src/Semaforo.c"
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
t_list* lista_Claves;

t_list* tabla_de_entradas;


//VARIABLES GLOBALES----
char* storage;
int32_t cantEntradas;
int32_t tamEntrada;
int32_t cantEntradasDisp;
ConfigInstancia config;
int socketServer;
int compactado = 1;

pthread_mutex_t semaforo_compactacion = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char* argv[]) {

	lista_Claves = list_create();
	tabla_de_entradas = list_create();

	char* nombre = argv[1];
	config = cargar_config_inst(nombre);

	socketServer = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador);

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
	disponibles = bitarray_create(bitarray, cantEntradas);
	limpiarArray(0,cantEntradas);

	//tablaEntradas = dictionary_create();
	reemplazos = list_create();

	//aca crear hilo principal

	crear_hilo(hilo_dump, NULL);
	rutina_principal();

	free(bitarray);
	destruirlo_todo();
	return 0;
}

void* rutina_sentencia(t_sentencia* sentencia) {
	ejecutarSentencia(sentencia);
	enviarMensaje(socketServer,ejecucion_ok,&cantEntradasDisp,sizeof(int));
	return NULL;
}

void rutina_principal() {

	printf(GREEN "\nEsperando ordenes pacificamente...\n"RESET);

		while(1){
			void* stream;
			Accion accion = recibirMensaje(socketServer, &stream);

			switch(accion){
				case ejecutar_sentencia_instancia:
					crear_hilo(hilo_sentencia, (t_sentencia*)stream);
					break;

				case compactar:
					crear_hilo(hilo_compactar, NULL);
					break;

				case error:
				default:
					printf(RED "\nError!\n"RESET );
					printf(RED "\nSe desconecto el coordinador!\n"RESET);
					break;
			}
		}
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

Reg_TablaEntradas* buscar_entrada(char* clave) {
	for (int i = 0; i < list_size(tabla_de_entradas); i++) {
		Reg_TablaEntradas* e = list_get(tabla_de_entradas, i);
		if (strcmp(e->clave, clave) == 0)
			return e;
	}
	return NULL;
}

Reg_TablaEntradas* borrar_devolver_entrada(char* clave) {
	for (int i = 0; i < list_size(tabla_de_entradas); i++) {
		Reg_TablaEntradas* e = list_get(tabla_de_entradas, i);
		if (strcmp(e->clave, clave) == 0)
			return list_remove(tabla_de_entradas, i);
	}
	return NULL;
}

void mostrarValor(char* clave){
	//Reg_TablaEntradas* registro = dictionary_get(tablaEntradas,clave);
	Reg_TablaEntradas* registro = buscar_entrada(clave);

	char* valor=malloc((sizeof(char)*registro->tamanio)+1);
	memcpy(valor, storage+(tamEntrada*registro->entrada),registro->tamanio);

	printf("\nEl valor de la clave %s es: %s\n",clave,valor);
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
			if(string_equals_ignore_case(nodo->clave,nodo2->clave))
				list_remove_and_destroy_element(reemplazos,j,(void*) nodoRempDestroyer);
		}
	}

}

void* compactacion() {

	s_wait(&semaforo_compactacion);

	compactado = 0;
	const int microsegundo = 1 * 1000 * 1000;
	printf(RED"\nCOMPACTANDO...\n"RESET);
	//usleep(5 * microsegundo); //5 segundos

	//imprimir_almacenamiento();
	mostrarArray(NULL);
	compact();
	printf(GREEN"\nSE HA COMPACTADO CON EXITO!\n"RESET);
	//imprimir_almacenamiento();
	mostrarArray(NULL);

	compactado = 1;
	avisar(socketServer, compactacion_ok);

	s_signal(&semaforo_compactacion);

	return NULL;
}

int compact() {
    int i, inicioAnterior, libre, cont;
    //traductor_marco* procesoAMover;
    Reg_TablaEntradas* entrada_a_mover;

    int ordenar_menor_mayor(Reg_TablaEntradas* entrada1, Reg_TablaEntradas* entrada2){
        return (entrada1->entrada < entrada2->entrada);
    }

    int proxima_entrada(Reg_TablaEntradas* entrada){
        return (entrada->entrada >= i);
    }

    list_sort(tabla_de_entradas, (void*)ordenar_menor_mayor);

    for(i=0; i < cantEntradas; i++){
        if (!bitarray_test_bit(disponibles,i)){
                libre = i;
                // busque el proximo proceso ocupado a mover
                entrada_a_mover = list_find(tabla_de_entradas, (void*)proxima_entrada);
                if (entrada_a_mover != NULL) {
                        inicioAnterior = entrada_a_mover->entrada;
                        int entradas_que_ocupa = 1 + ((entrada_a_mover->tamanio - 1) / tamEntrada);
                        for (cont = 0; cont < entradas_que_ocupa; cont++){
                            bitarray_clean_bit(disponibles, inicioAnterior + cont);                       //porque ahora va a estar vacia
                            bitarray_set_bit(disponibles, libre + cont);                                  //Ahora va a estar ocupada
                        }
                    //memcpy(archivoSwap+ libre * datosSwap->tamPagina, archivoSwap+inicioAnterior * datosSwap->tamPagina, procesoAMover->paginas * datosSwap->tamPagina);        //Modifique los marcos, ahora copio los datos
                    memcpy(storage + libre * tamEntrada, storage + inicioAnterior * tamEntrada, entrada_a_mover->tamanio);        //Modifique los marcos, ahora copio los datos
                    entrada_a_mover->entrada = libre;
                }
                else
                    i = cantEntradas;                         //No hay mas procesos para mover => salgo del ciclo, no necesito buscar mas
        }
    }
    return 1;
}


void imprimir_almacenamiento() {
	printf(CYAN"\nAlmacenamiento:\n");
	for (int i = 0; i < cantEntradas; i++) {
		int bit = bitarray_test_bit(disponibles, i);
		char* color = bit == 0 ? RED : GREEN;
		printf("\n%s", color);
		for (int j = 0; j < tamEntrada; j++)
			printf("#");
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
		printf(GREEN "\nSe ejecuto un SET correctamente, de clave %s con valor %s.\n"RESET, sentencia->clave, sentencia->valor);
		mostrarListaReemplazos(reemplazos);
		mostrarArray(NULL);
		if(!laListaLoContiene(sentencia->clave))
			list_add(lista_Claves,sentencia->clave);
		break;

	case S_STORE:
		aumentarTiempoRef();
		persistirValor(sentencia->clave);
		printf(GREEN "\nSe ejecuto un STORE correctamente, de clave %s.\n"RESET, sentencia->clave);
		break;

	default:
		printf(RED "\nTipo de sentencia no valida!\n" RESET);
		exit(0);
		break;
	}
}

int existe_entrada(char* clave) {
	return buscar_entrada(clave) != NULL;
}

void almacenarValor(char* clave, char* valor){

	int tamEnBytes = string_length(valor)+1;
	int tamEnEntradas = 1+((tamEnBytes-1)/tamEntrada);			//redondeo para arriba

	/*if(dictionary_has_key(tablaEntradas,clave))
		liberarEntradas(clave);*/

	if(existe_entrada(clave))
		liberarEntradas(clave);

	if(tamEnEntradas <= cantEntradasDisp){

		int posInicialLibre = buscarEspacioLibre(tamEnEntradas);
		strcpy(storage+(tamEntrada*posInicialLibre), valor);

		Reg_TablaEntradas* registro = malloc(sizeof(Reg_TablaEntradas));
		registro->entrada = posInicialLibre;
		registro->tamanio = tamEnBytes;
		registro->clave = malloc(strlen(clave)+1);
		strcpy(registro->clave,clave);
		//dictionary_put(tablaEntradas,clave,registro);
		list_add(tabla_de_entradas, registro);
		cantEntradasDisp -= tamEnEntradas;

		if (tamEnEntradas == 1){								//Si el valor es atomico, se selecciona como valor de reemplazo
			Nodo_Reemplazo* remp = malloc(sizeof(Nodo_Reemplazo));
			remp->clave = strdup(clave);
			remp->tamanio = tamEnBytes;
			remp->ultimaRef = 0;
			list_add(reemplazos,remp);
		}

	} else {

		if(tamEnEntradas <= list_size(reemplazos)){
			printf(YELLOW "\nEl valor de la clave %s reemplazara %d valor(es) existente(s)\n"RESET,clave,tamEnEntradas);
			reemplazarValor(clave,valor,tamEnEntradas);

		} else{

			printf(RED "\nNo existen suficientes entradas de reemplazo para ubicar valor de clave: %s!\n\n"RESET,clave);
			exit(0); //por ahora exit, pero enviar mensaje de fallo supongo
			//SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
			return;
		}
	}

}

void persistirValor(char* clave){
	char* path = malloc(string_length(clave)+strlen(config.punto_montaje)+5);

	struct stat st = {0};
	if (stat(path, &st) == -1)
		mkdir(path, 0777);

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
	//Reg_TablaEntradas* registro = dictionary_get(tablaEntradas, clave);
	Reg_TablaEntradas* registro = buscar_entrada(clave);

	//char* valor;/* = malloc(sizeof(char)*registro->tamanio);*/
	char* valor = strdup(storage+(tamEntrada*registro->entrada));

	return valor;
}


void liberarEntradas(char* clave){
	//Reg_TablaEntradas* registro  = dictionary_remove(tablaEntradas,clave);
	Reg_TablaEntradas* registro = borrar_devolver_entrada(clave);
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
	int posInicialLibre = 0, i, contador = 0;
	int ya_avise = 0;

	do {
		for(i=0;i<disponibles->size && contador<entradasNecesarias;i++){
			if (bitarray_test_bit(disponibles,i) == 0) //espacio libre
				contador++;
			else{
				contador=0;
				posInicialLibre= i + 1;
			}
		}

	if (contador < entradasNecesarias) {
		compactado = 0;						//Compacta en caso de ser necesario (porque hay espacios pero no son contiguos)
		if (ya_avise == 0) {
			avisar(socketServer, compactar);
			//s_wait(&semaforo_compactacion);
			while(compactado != 1);
			ya_avise = 1;
		}
	}
	else
		compactado = 1;
	} while(compactado == 0);

	for(int i = posInicialLibre; i < posInicialLibre + contador; i++)
		bitarray_set_bit(disponibles, i);						//ocupa las entradas

	return posInicialLibre;
}

void destruirlo_todo(){
	bitarray_destroy(disponibles);
	free(storage);
	//dictionary_destroy_and_destroy_elements(tablaEntradas,(void*)regTablaDestroyer);
	list_clean_and_destroy_elements(reemplazos,(void*)nodoRempDestroyer);					//elimina elementos de la lista
	free(reemplazos); 																			//elimina la lista en si
}

//--------------------------------------------------------------------------------------------------------------------------------------------



void crear_hilo(HiloInstancia tipo, t_sentencia* sentencia) {
	pthread_attr_t attr;
	pthread_t hilo;
	int  res = pthread_attr_init(&attr);
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	switch(tipo) {
	case hilo_sentencia:
		res = pthread_create (&hilo ,&attr, rutina_sentencia, sentencia);
		break;
	case hilo_dump:
		res = pthread_create (&hilo ,&attr, rutina_Dump, NULL);
		break;
	case hilo_compactar:
		res = pthread_create (&hilo ,&attr, compactacion, NULL);
		break;
	}
	if (res != 0) {
		printf( "\nError en la creacion del hilo\n");
	}
	pthread_attr_destroy(&attr);
}

void *rutina_Dump(void * arg) {
	while(1){
		usleep(config.intervalo_dump);
		guardarLaWea();
	}
	return NULL;
}

int laListaLoContiene(char * clave){
	for(int i = 0; i < list_size(lista_Claves); i++){
		int resultado = strcmp(clave, (char*)list_get(lista_Claves, i));
		if(resultado == 0)
			return 1;
	}
	return 0;
}

void guardarLaWea()
{
	for(int i=0;i<(list_size(lista_Claves));i++)
	{
		char* clave = (char*)list_get(lista_Claves, i);
		if(existe_entrada(clave))
			persistirValor(clave);
	}
}





