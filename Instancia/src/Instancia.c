#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../../Bibliotecas/src/Estructuras.h"
#include "../../Bibliotecas/src/Socket.c"
#include "../../Bibliotecas/src/Configuracion.c"
#include "Instancia.h"

#include "commons/string.h"
#include "commons/config.h"
#include "commons/bitarray.h"
#include "commons/collections/dictionary.h"
#include "commons/collections/list.h"
#include "semaphore.h"

ConfigInstancia config;

//ESTRUCTURAS----
t_bitarray* disponibles;
t_list* reemplazos;
t_list* lista_Claves;
t_list* tabla_de_entradas;
t_list* claves_iniciales;

//VARIABLES GLOBALES----
char* storage;
char* bitarray;
int32_t cantEntradas;
int32_t tamEntrada;
int32_t cantEntradasDisp;
ConfigInstancia config;
int socketServer;
pthread_t thread_dump;

pthread_mutex_t semaforo_compactacion = PTHREAD_MUTEX_INITIALIZER;
sem_t compactacion_espera; //contador de inst conectadas

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL);

	char* nombre = argv[1];
	config = cargar_config_inst(nombre);
	imprimir_configuracion();
	socketServer = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador);
	handShake(socketServer, instancia);
	configurar_entradas();
	inicializar_estructuras();
	recuperar_claves(config.punto_montaje); //"Esta info. debe ser recuperada al momento de iniciar una instancia"
	cargar_claves_iniciales();
	signal(SIGINT, terminar_programa);
	crear_hilo(hilo_dump, NULL);
	rutina_principal();

	return 0;
}

void terminar_programa(int sig) {
	printf(GREEN"\nResultados finales:"RESET);
	mostrar_storage();
	destruirlo_todo();
	pthread_cancel(thread_dump);
	pthread_join(thread_dump, NULL);
	exit(0);
}

void ordenar_reemplazos(int algoritmo) {
	if (algoritmo == LRU)
		list_sort(reemplazos, (void*)comparadorMayorTiempo);
	else if (algoritmo == BSU)
		list_sort(reemplazos, (void*)comparadorMayorTam);
}

int numero_entrada(Nodo_Reemplazo* nodo) {
	return buscar_entrada(nodo->clave)->entrada;
}

char* valor_entrada(int entrada) { //devuelve lo que ENTRÓ en esa entrada, no el valor total
	char* retorno = malloc(tamEntrada + 1);
	memcpy(retorno, storage + entrada * tamEntrada, tamEntrada);
	retorno[tamEntrada] = '\0';
	return retorno;
}

int es_reemplazable(int indice) { //este indice es DEL STORAGE, no de la tabla
	char* entrada = valor_entrada(indice);
	char* valor;

	int es_entrada(Nodo_Reemplazo* nodo) {
		Reg_TablaEntradas* reg = buscar_entrada(nodo->clave);
		valor = devolver_valor(nodo->clave);
		int resultado = strcmp(entrada, valor) == 0 && indice == reg->entrada;
		free(valor);
		return resultado;
	}

	int resultado = list_any_satisfy(reemplazos, (void*)es_entrada);
	free(entrada);
	return resultado;
}

void mostrar_storage() {
	Nodo_Reemplazo* nodo_reemplazo = list_get(reemplazos, 0);
	char* color = RESET;
	ordenar_reemplazos(config.algoritmo_reemp); //lo ordeno antes de imprimir
	printf("\n");
	int puntero_reemplazo = nodo_reemplazo != NULL ? numero_entrada(nodo_reemplazo) : -1;
	for (int i = 0; i < cantEntradas; i++) {
		int es_puntero = puntero_reemplazo == i;
		char* valor = valor_entrada(i);
		printf(YELLOW"\n[%d]\t"RESET, i);
		if (es_reemplazable(i)) color = RED;
		if (es_puntero) 		color = GREEN;
		printf("%s%s"RESET, color, valor);
		free(valor);
		if (es_puntero)	printf(GREEN"\t<<< REEMPLAZO"RESET);
		color = RESET;
	}
	printf("\n");
}

void configurar_entradas() {
	int bytes = strlen(config.nombre_instancia) + 1;
	enviarMensaje(socketServer, config_inst, &config.nombre_instancia, bytes);
	void* dim;
	if (recibirMensaje(socketServer, &dim) == config_inst) {
		memcpy(&cantEntradas,(int*)dim,sizeof(int));
		memcpy(&tamEntrada,(int*)dim+1,sizeof(int));
		free(dim);
		printf(MAGENTA"\nConfiguracion recibida del coordinador:");
		printf(YELLOW"\nCantidad de entradas: "RED"%d"RESET, cantEntradas);
		printf(YELLOW"\nTamaño de entradas: "RED"%d\n"RESET, tamEntrada);
		cantEntradasDisp = cantEntradas;
		storage = calloc(sizeof(char)*cantEntradas*tamEntrada, tamEntrada);
	} else {
		printf(RED "\nERROR AL RECIBIR CONFIG DEL COORDINADOR\n"RESET);
		terminar_programa(0);
	}
}

void inicializar_estructuras() {
	lista_Claves = list_create();
	claves_iniciales = list_create();
	tabla_de_entradas = list_create();
	reemplazos = list_create();
	bitarray = malloc(sizeof(char) * cantEntradas);
	disponibles = bitarray_create(bitarray, cantEntradas);
	limpiarArray(0, cantEntradas);
}

int recuperar_claves(char* ruta) {
	struct dirent *dp;
	DIR *fd_directorio;
	char nombre_archivo[100];

	if ((fd_directorio = opendir(ruta)) == NULL) {
		printf(RED"\nNo existe el punto de montaje"GREEN" %s"RED", se creara al momento de hacer DUMP"RESET, ruta);
		closedir(fd_directorio);
		return 0;
	}
	printf(CYAN"\nBuscando claves en"GREEN" %s...\n"RESET, ruta);

	while ((dp = readdir(fd_directorio)) != NULL) {
		struct stat stbuf ;
		sprintf(nombre_archivo, "%s%s", ruta, dp->d_name); //concateno ruta + nombre arch
		if(stat(nombre_archivo, &stbuf ) == -1 ) {
			printf(RED"/nNo se pudo hacer STAT de archivo %s: ", nombre_archivo) ;
			continue;
		}

	  if (( stbuf.st_mode & S_IFMT ) == S_IFDIR)
		  continue; //son directorios (raro) -> no los tengo en cuenta
	  else
		  cargar_clave_montaje(nombre_archivo, dp->d_name);
	 }

	closedir(fd_directorio);
	return 1;
}

void cargar_claves_iniciales() {
	printf(YELLOW"\n\nAlmacenando claves iniciales..."RESET);
	for (int i = 0; i < list_size(claves_iniciales); i++) {
		t_clave_inicial* c = list_get(claves_iniciales, i);
		almacenarValor(c->clave, c->valor);
		printf("\nAlmacenada clave %s", c->clave);
	}
}

void cargar_clave_montaje(char* archivo, char* clave) {
	FILE* file = fopen(archivo, "r");
	char line[256];
	if (file) {
		fgets(line, sizeof(line), file);
		char* valor = line;
		printf(GREEN"\nSe encontro clave"CYAN" %s"GREEN" con valor"RED" %s"GREEN" en montaje."RESET, clave, valor);
		t_clave_inicial* c = malloc(sizeof(t_clave_inicial));
		c->clave = strdup(clave);
		c->valor = strdup(valor);
		list_add(claves_iniciales, c);

		if(!laListaLoContiene(clave))
			list_add(lista_Claves, strdup(clave));
	}
	fclose(file);
}

void* rutina_sentencia(void* arg) {
	t_sentencia* sentencia = (t_sentencia*)arg;
	ejecutarSentencia(sentencia);
	return NULL;
}

t_sentencia copiar_sentencia(t_sentencia* stream) {
	t_sentencia s;
	strcpy(s.clave, stream->clave);
	strcpy(s.valor, stream->valor);
	s.id_esi = stream->id_esi;
	s.tipo = stream->tipo;
	return s;
}

void rutina_principal() {

	printf(GREEN "\nEsperando ordenes pacificamente...\n"RESET);

		while(1){
			void* stream;
			Accion accion = recibirMensaje(socketServer, &stream);

			pthread_mutex_lock(&semaforo_compactacion);

			switch(accion){
				case ejecutar_sentencia_instancia: {
					t_sentencia sent = copiar_sentencia(stream);
					crear_hilo(hilo_sentencia, &sent);
					break;
				}
				case compactar:
					crear_hilo(hilo_compactar, NULL);
					break;
				case verificar_conexion:
					break;
				case error:
				default:
					printf(RED "\nError!\n"RESET);
					printf(RED "\nSe desconecto el coordinador!\n"RESET);
					//free(stream);
					terminar_programa(0);
			}

			free(stream);

			pthread_mutex_unlock(&semaforo_compactacion);

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
	char* val = devolver_valor(clave);
	printf("\nEl valor de la clave %s es: %s\n", clave, val);
	free(val);
}

void mostrarListaReemplazos(t_list* list){
	printf("Lista de reemplazos:\n");
	for(int i = 0; i < list_size(list); i++){
		Nodo_Reemplazo* nodo = list_get(list, i);
		printf("[%d]\t", i);
		if (config.algoritmo_reemp == LRU)
			printf("[T = "GREEN"%d"RESET"]\t", nodo->ultimaRef);
		else if (config.algoritmo_reemp == BSU)
			printf("[B = "GREEN"%d"RESET"]\t", nodo->tamanio);
		char* v = devolver_valor(nodo->clave);
		printf("%s\n", v);
		free(v);
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

void clavesInicialDestroyer(t_clave_inicial* nodo){
	free(nodo->clave);
	free(nodo->valor);
	free(nodo);
}


//--------------MANEJO LISTAS-------------------

t_list* duplicarLista(t_list* self) {
	t_list* duplicated = list_create();
	for(int i = 0; i < list_size(self); i++){
		Nodo_Reemplazo* nodo = list_get(self , i);
		Nodo_Reemplazo* nodo2 = malloc(sizeof(Nodo_Reemplazo));
		nodo2->clave = strdup(nodo->clave);
		nodo2->tamanio = nodo->tamanio;
		nodo2->ultimaRef = nodo->ultimaRef;
		list_add(duplicated, nodo2);
	}
	return duplicated;
}

bool comparadorMayorTam(Nodo_Reemplazo* nodo1, Nodo_Reemplazo* nodo2){
	return (nodo1->tamanio>=nodo2->tamanio);
}

bool comparadorMayorTiempo(Nodo_Reemplazo* nodo1, Nodo_Reemplazo* nodo2){
	return (nodo1->ultimaRef >= nodo2->ultimaRef);
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

int* bits_disponibles(t_bitarray* bitarray) {
	int* disp = malloc(sizeof(int) * cantEntradas);
	for (int i = 0; i < cantEntradas; i++)
		disp[i] = bitarray_test_bit(bitarray, i);
	return disp;
}

void* compactacion() {

	printf(RED"\nCOMPACTANDO...\n"RESET);

	pthread_mutex_lock(&semaforo_compactacion);
	int* disponibles_antes = bits_disponibles(disponibles);
	compact();
	int* disponibles_ahora = bits_disponibles(disponibles);
	printf(GREEN"\nSE HA COMPACTADO CON EXITO!\n"RESET);

	imprimir_almacenamiento(disponibles_antes, disponibles_ahora);
	free(disponibles_antes);
	free(disponibles_ahora);

	avisar(socketServer, compactacion_ok);
	sem_post(&compactacion_espera);
	pthread_mutex_unlock(&semaforo_compactacion);

	return NULL;
}


void compact() {
    int i, inicioAnterior, libre, cont;

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
                entrada_a_mover = list_find(tabla_de_entradas, (void*)proxima_entrada);
                if (entrada_a_mover != NULL) {
                	inicioAnterior = entrada_a_mover->entrada;
                    int entradas_que_ocupa = 1 + ((entrada_a_mover->tamanio - 1) / tamEntrada);

                    for (cont = 0; cont < entradas_que_ocupa; cont++){
                    	bitarray_clean_bit(disponibles, inicioAnterior + cont);
                        bitarray_set_bit(disponibles, libre + cont);
                    }
                    char* copia = calloc(entradas_que_ocupa * tamEntrada, sizeof(char));
                    memcpy(copia, storage + inicioAnterior * tamEntrada, entrada_a_mover->tamanio);
                    limpiarStorage(inicioAnterior, inicioAnterior + entradas_que_ocupa);
                    memcpy(storage + libre * tamEntrada, copia, entrada_a_mover->tamanio);
                    entrada_a_mover->entrada = libre;
                    free(copia);
                }
                else
                    i = cantEntradas;
        }
    }
}


void imprimir_almacenamiento(int* anterior, int* actual) {
	printf(CYAN"\nAlmacenamiento:\n\n");
	const int espacio = tamEntrada;
	char* a = "ESTADO ANTERIOR";
	char* b = "ESTADO_ACTUAL";
	printf(YELLOW"\t%*s", espacio / 2 + strlen(a) / 2, a);
	printf("\t\t\t");
	printf("%*s"RESET, espacio / 2 + strlen(b) / 2, b);
	for (int i = 0; i < cantEntradas; i++) {
		printf("\n\t");
		imprimir_espacio(anterior[i]);
		printf("\t\t\t");
		imprimir_espacio(actual[i]);
	}
	printf("\n\n"RESET);
}

void imprimir_espacio(int bit) {
	char* color = bit == 0 ? RED : GREEN;
	printf("%s", color);
	for (int j = 0; j < tamEntrada; j++)
		printf("#");
}

void aumentarTiempoRef(){
	for(int i = 0; i < list_size(reemplazos); i++){
		Nodo_Reemplazo* remp = list_get(reemplazos, i);
		remp->ultimaRef++;
	}
}

int indice_reemplazo(char* clave){
	for(int i = 0; i < list_size(reemplazos); i++) {
		Nodo_Reemplazo* remp = list_get(reemplazos,i);
		if (string_equals_ignore_case(remp->clave, clave))
			return i;
	}
	return -1;
}

void ejecutarSentencia(t_sentencia* sentencia){

	switch(sentencia->tipo){

	case S_SET:
		aumentarTiempoRef();
		almacenarValor(sentencia->clave, sentencia->valor);
		printf(GREEN "\nSET de clave "CYAN"%s"GREEN" con valor"RED" %s."RESET, sentencia->clave, sentencia->valor);
		if (config.mostrar_storage) {
			mostrar_storage();
			printf("\n");
			mostrarListaReemplazos(reemplazos);
		}
		if(!laListaLoContiene(sentencia->clave))
			list_add(lista_Claves, strdup(sentencia->clave));
		enviarMensaje(socketServer, ejecucion_ok, &cantEntradasDisp, sizeof(int));
		break;

	case S_STORE: {
		aumentarTiempoRef();
		printf(GREEN "\nSTORE de clave "CYAN"%s."RESET, sentencia->clave);
		int result = persistirValor(sentencia->clave);
		if (result != 0)
			enviarMensaje(socketServer, ejecucion_ok, &cantEntradasDisp, sizeof(int));
		else {
			printf(RED"\nNo se pudo persistir valor de clave %s, se abortara ESI."RESET, sentencia->clave);
			enviarMensaje(socketServer, ejecucion_no_ok, &cantEntradasDisp, sizeof(int));
		}
		break;
	}
	default:
		printf(RED "\nTipo de sentencia no valida!\n" RESET);
		terminar_programa(0);
		break;
	}
}

int existe_entrada(char* clave) {
	return buscar_entrada(clave) != NULL;
}

void nuevo_registro(char* clave, int entrada, int tamanio) {
	Reg_TablaEntradas* registro = malloc(sizeof(Reg_TablaEntradas));
	registro->entrada = entrada;
	registro->tamanio = tamanio;
	registro->clave = malloc(strlen(clave) + 1);
	strcpy(registro->clave,clave);
	list_add(tabla_de_entradas, registro);
}

char* agregar_barra_cero(char* valor, int tamanio) { //necesito tamanio para saber hasta donde grabar, sin el /0 se rompe el string
	char* copia = malloc(tamanio + 1);
	memcpy(copia, valor, tamanio);
	copia[tamanio] = '\0';
	return copia;
}

void nuevo_nodo_reemplazo(char* clave, int tamanio) {
	Nodo_Reemplazo* remp = malloc(sizeof(Nodo_Reemplazo));
	remp->clave = strdup(clave);
	remp->tamanio = tamanio;
	remp->ultimaRef = 0;
	list_add(reemplazos,remp);
}

void limpiar_entrada(int entrada) {
	for (int i = 0; i < tamEntrada; i++)
		memcpy(storage + (tamEntrada * entrada) + i, "\0", 1);
}

int es_atomico(char* valor) {
	return strlen(valor) <= tamEntrada;
}

int existe_nodo(char* clave) {
	return indice_reemplazo(clave) != -1;
}

void almacenarValor(char* clave, char* valor){

	int tamEnBytes = string_length(valor);
	int tamEnEntradas = 1 + (tamEnBytes - 1) / tamEntrada;			//redondeo para arriba

	if(existe_entrada(clave))
		liberarEntradas(clave, false); //libero la entrada, pero el reemplazo sigue

	if(tamEnEntradas <= cantEntradasDisp) {
		sem_init(&compactacion_espera, true, 1);
		int posInicialLibre = buscarEspacioLibre(tamEnEntradas);
		limpiar_entrada(posInicialLibre);
		memcpy(storage + (tamEntrada * posInicialLibre), valor, tamEnBytes);
		nuevo_registro(clave, posInicialLibre, tamEnBytes);
		cantEntradasDisp -= tamEnEntradas;

		if (es_atomico(valor) && !existe_nodo(clave))
			nuevo_nodo_reemplazo(clave, tamEnBytes);

	} else {
		if(tamEnEntradas <= list_size(reemplazos)) {
			printf(YELLOW "\nReemplazo por clave %s"RESET, clave);
			reemplazarValor(clave, valor, tamEnEntradas);
		} else {
			printf(RED "\nNo existen suficientes entradas de reemplazo para ubicar valor de clave: %s!\n\n"RESET,clave);
			terminar_programa(0);
		}
	}
}

int persistirValor(char* clave){
	char* path = malloc(strlen(clave) + strlen(config.punto_montaje) + 1);

	struct stat st = {0};
	if (stat(config.punto_montaje, &st) == -1)
		mkdir(config.punto_montaje, 0777);

	strcpy(path, config.punto_montaje);
	strcat(path, clave);

	char* valor = devolver_valor(clave);

	if (valor != NULL) {
		FILE* arch = fopen(path, "w+");
		fputs(valor, arch);
		fclose(arch);
	} else {
		free(valor);
		free(path);
		return 0;
	}

	free(valor);
	free(path);
	return 1;
}

t_list* reemplazoSegunAlgoritmo(int cantNecesita){

	t_list* seleccionados;

	switch(config.algoritmo_reemp){

	case CIRC:
		seleccionados = list_take_and_remove(reemplazos, cantNecesita);
		break;

	case LRU:
		ordenar_reemplazos(LRU);
		seleccionados = list_take_and_remove(reemplazos, cantNecesita);
		break;

	case BSU:
		ordenar_reemplazos(BSU);
		seleccionados = list_take_and_remove(reemplazos, cantNecesita);
		break;
	}

	return seleccionados;
}

void reemplazarValor(char* clave, char* valor, int tamEnEntradas){

	//t_list* paraReemplazar = reemplazoSegunAlgoritmo(tamEnEntradas - cantEntradasDisp);
	t_list* paraReemplazar = reemplazoSegunAlgoritmo(tamEnEntradas);

	int i;
	for(i=0;i<list_size(paraReemplazar);i++){
		Nodo_Reemplazo* remp = list_get(paraReemplazar,i);
		liberarEntradas(remp->clave, true); //libero las entradas y el nodo lo elimino
	}

	list_clean_and_destroy_elements(paraReemplazar,(void*) nodoRempDestroyer);
	free(paraReemplazar);
	almacenarValor(clave, valor);
}

char* devolver_valor(char* clave) {
	Reg_TablaEntradas* registro = buscar_entrada(clave);
	if (registro != NULL) {
		char* valor = malloc(sizeof(char) * registro->tamanio);
		memcpy(valor, storage + (tamEntrada * registro->entrada), registro->tamanio);
		char* con = agregar_barra_cero(valor, registro->tamanio);
		free(valor);
		return con;
	}
	return NULL;
}

void limpiarStorage(int desde, int hasta) {
	for (int i = desde * tamEntrada; i < hasta * tamEntrada; i++)
	   storage[i] = '\0';
}

void liberarEntradas(char* clave, int borrar_nodo){
	Reg_TablaEntradas* registro = borrar_devolver_entrada(clave);
	int tamEnEntradas = 1 + (registro->tamanio - 1) / tamEntrada;
	int desde = registro->entrada;
	int hasta = registro->entrada + tamEnEntradas;
	limpiarArray(desde, hasta);
	limpiarStorage(desde, hasta);
	cantEntradasDisp += tamEnEntradas;


	if (existe_nodo(clave)) {
		int indice = indice_reemplazo(clave);
		Nodo_Reemplazo* nodo = list_get(reemplazos, indice);
		nodo->ultimaRef = 0;
		if(borrar_nodo)
			list_remove_and_destroy_element(reemplazos, indice, (void*)nodoRempDestroyer);
	}

	regTablaDestroyer(registro);
}

void limpiarArray(int desde, int hasta){
	for(int i = desde; i < hasta; i++)
		bitarray_clean_bit(disponibles, i);
}

int buscarEspacioLibre(int entradasNecesarias){

	int posInicialLibre = 0, contador = 0;

	sem_wait(&compactacion_espera);

	for(int i = 0; i < disponibles->size && contador < entradasNecesarias; i++){
		if (bitarray_test_bit(disponibles,i) == 0) //espacio libre
			contador++;
		else {
			contador = 0;
			posInicialLibre = i + 1;
		}
	}

	if (contador < entradasNecesarias) {
		avisar(socketServer, compactar);
		printf(RED"\n\nAvisado el coordinador de compactacion."RESET);
		return buscarEspacioLibre(entradasNecesarias);
	}

	for(int i = posInicialLibre; i < posInicialLibre + contador; i++)
		bitarray_set_bit(disponibles, i);						//ocupa las entradas

	return posInicialLibre;
}

void destruirlo_todo(){
	free(bitarray);
	bitarray_destroy(disponibles);

	list_clean_and_destroy_elements(tabla_de_entradas, (void*)regTablaDestroyer);
	free(tabla_de_entradas);

	list_clean_and_destroy_elements(reemplazos,(void*)nodoRempDestroyer);					//elimina elementos de la lista
	free(reemplazos);																		//elimina la lista en si

	list_clean_and_destroy_elements(claves_iniciales, (void*)clavesInicialDestroyer);
	free(claves_iniciales);

	list_clean_and_destroy_elements(lista_Claves, free);
	free(lista_Claves);

	free(storage);

}

//--------------------------------------------------------------------------------------------------------------------------------------------


void crear_hilo(HiloInstancia tipo, t_sentencia* sentencia) {
	pthread_attr_t attr;
	pthread_t hilo;
	int res;
	pthread_attr_init(&attr);
	switch(tipo) {
	case hilo_sentencia:
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		res = pthread_create (&hilo ,&attr, rutina_sentencia, sentencia);
		break;
	case hilo_dump:
		res = pthread_create (&thread_dump, &attr, rutina_Dump, NULL);
		break;
	case hilo_compactar:
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		res = pthread_create (&hilo, &attr, compactacion, NULL);
		break;
	}
	if (res != 0) {
		printf( "\nError en la creacion del hilo\n");
	}
	pthread_attr_destroy(&attr);
}

void *rutina_Dump(void * arg) {
	while(1){
		usleep(config.intervalo_dump * 1000 * 1000);
		pthread_mutex_lock(&semaforo_compactacion);
		guardarLaWea();
		pthread_mutex_unlock(&semaforo_compactacion);
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
	printf(GREEN"\nHaciendo dump..."RESET);

	for(int i=0;i<(list_size(lista_Claves));i++)
	{
		char* clave = (char*)list_get(lista_Claves, i);
		if(existe_entrada(clave))
			persistirValor(clave);
	}
}

void imprimir_configuracion() {
	printf(CYAN"\nCoordinador: "YELLOW"%s : %d"RESET, config.ip_coordinador, config.puerto_coordinador);
	printf(GREEN"\n\nConfiguracion cargada con exito:"RESET);
	printf(GREEN"\nAlgoritmo: "RED"%s", algoritmo(config.algoritmo_reemp));
	printf(GREEN"\nMontaje: "RED"%s", config.punto_montaje);
	printf(GREEN"\nNombre: "RED"%s", config.nombre_instancia);
	printf(GREEN"\nIntervalo de dump: "RED"%d"RESET, config.intervalo_dump);
	printf(GREEN"\nMostrar storage: "RED"%s\n"RESET, string_mostrar_storage());
}

char* string_mostrar_storage() {
	return config.mostrar_storage ? "SI" : "NO";
}

char* algoritmo(AlgoritmoInst alg) {
	switch (alg) {
		case CIRC: 			return "CIRC";
		case LRU: 			return "LRU";
		case BSU:default: 	return "BSU";
	}
}


