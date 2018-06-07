#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../Bibliotecas/src/Socket.c"
#include "../../Bibliotecas/src/Configuracion.c"
#include "../../Bibliotecas/src/Estructuras.h"
#include "../../Bibliotecas/src/Color.h"
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include "commons/string.h"
#include "Coordinador.h"
#include "commons/collections/list.h"


// Diferenciacion de operacion ESI
// Direnciar quien se conecta
// Guardar con ID instancia
// TOMI SE LA COME


t_list *lista_Instancias;
ConfigCoordinador configuracion;
int socket_plan; //esto cambiar tal vez
t_dictionary *instancias_Claves;



int main(int argc, char* argv[]) {
	int banderaPlanificador=0;

	instancias_Claves= dictionary_create();


	configuracion = cargar_config_coordinador(argv[1]);
	crear_log_operacion();
	log_info(log_operaciones, "Se ha cargado la configuracion inicial del Coordinador");
	mostrar_por_pantalla_config(configuracion);
	lista_Instancias = list_create();

	int listener = crear_socket_de_escucha(configuracion.puerto_escucha);

	int nuevo_socket, modulo;

	while(1){
		if(banderaPlanificador==0){
			nuevo_socket = aceptar_nueva_conexion(listener);
			recv(nuevo_socket, &modulo, sizeof(int), 0);//HS
			crear_hilo(nuevo_socket, modulo);

			int socket_server = conexion_con_servidor(configuracion.ip_planificador, configuracion.puerto_planificador);
			socket_plan = socket_server; //pasar ocmo parametro, no global, pero por ahora lo hago asi
			handShake(socket_server, coordinador);

			banderaPlanificador++;
		}
		nuevo_socket = aceptar_nueva_conexion(listener);
		recv(nuevo_socket, &modulo, sizeof(int), 0);//HS
		crear_hilo(nuevo_socket, modulo);
	}
	destruir_log_operacion();
	return 0;
}

int buscarEnLista(int valor){
	int socket;
	Nodo_Instancia* instancia = malloc( sizeof(Nodo_Instancia) );
	for( int i=0;i<  (list_size(lista_Instancias));i++ ){
		instancia = list_get(lista_Instancias , i);
		if(instancia->inst_ID==valor){
			socket=valor;
			break;
		}
	}
	free( instancia );
	return socket;
}

//ACCIONES DE LOS HILOS
void *rutina_instancia(void * arg) {
	int socket_INST = (int)arg;
	printf("\n------------------------------------\n");
	printf("NUEVA INSTANCIA EJECUTADA || ");

	Nodo_Instancia* nuevaInstancia = malloc(sizeof(Nodo_Instancia));
	nuevaInstancia->socket = socket_INST;

	if(list_is_empty(lista_Instancias)){
		nuevaInstancia->inst_ID = 5;
	} else {
		Nodo_Instancia* aux = list_get(lista_Instancias, (lista_Instancias->elements_count-1));
		nuevaInstancia->inst_ID = (aux->inst_ID)+1;
		free(aux);
	}

	list_add(lista_Instancias,(void*)nuevaInstancia);
	printf("ID:%d || SOCKET: %d\n", nuevaInstancia->inst_ID, nuevaInstancia->socket);


	configurar_instancia(socket_INST,nuevaInstancia->inst_ID);
	return NULL;
}

void configurar_instancia(int socket,int id){
	int* dim = malloc(sizeof(int)*3);//RECIBE 3
	memcpy(dim,&configuracion.cant_entradas,sizeof(int));
	memcpy(dim+1,&configuracion.tamanio_entrada,sizeof(int));
	memcpy(dim+2,&id,sizeof(int));

	enviarMensaje(socket,config_inst,dim,sizeof(int)*3);
	printf("\nputo el que lee\n");
}

void *rutina_ESI(void* argumento) {
	int socket_esi = *(int*)(&argumento);
	void* stream;
	printf(BLUE "\nEjecutando ESI (socket %d)..." RESET, socket_esi);

	while (recibirMensaje(socket_esi, &stream) == ejecutar_sentencia_coordinador) {

		t_sentencia* sentencia = (t_sentencia*)stream;
		char* recurso = (char*)sentencia->clave;
		//ENVIAR SENTENCIA
		enviarMensaje(socket_plan, preguntar_recursos_planificador, recurso, sizeof(recurso));
		int resultado_ejecucion = 0;

		int disponible = recibirMensaje(socket_plan, &stream); //el planif me da el OK, entonces ejecuto una sentencia del esi

		if (disponible) {
		//aca ejecutar sentencia esi en instancia
			switch(sentencia->tipo){
				case S_GET:
				{
					if( (dictionary_has_key(instancias_Claves , sentencia->clave) )==false ){
						//Persistir
						dictionary_put(instancias_Claves, sentencia->clave , (-1) );//VERIFICAR SI ES EN VARIABLE
					}
					break;
				}
				case S_SET:
				{
					int claveSentencia=(int*) (dictionary_get(instancias_Claves , sentencia->clave));
					if( claveSentencia == (-1)  ){
						//ALGORITMO Y ASIGNAR, modificar claveSentencia
					}

					int socketEncontrado=buscarEnLista(sentencia->clave);
					//VALIDAR INSTANCIA CONECTADA
					//MANDAR A INSTANCIA

					enviarMensaje(socketEncontrado,S_SET,sentencia,sizeof(t_sentencia));
					break;
				}
				case S_STORE:
				{
					int claveSentencia=(int*) (dictionary_get(instancias_Claves , sentencia->clave));
					//VALIDADA
					enviarMensaje(socket,S_STORE,sentencia,sizeof(t_sentencia));
					//MANDAR A INSTANCIA
					break;
				}
				default:{
					log_error(log_operaciones,"Error al identificar la operacion en el coordinador");
					break;
				}
			}
			resultado_ejecucion = 1; //1 = ok se ejecuto bien
		}
		enviarMensaje(socket_plan, ejecucion_ok, (void *)resultado_ejecucion, sizeof(int));
	}
	//sale del while -> no hay mas sentencias
	enviarMensaje(socket_plan, terminar_esi, NULL, 0);
	return NULL;
}

void crear_hilo(int nuevo_socket, int modulo) {
	pthread_attr_t attr;
	pthread_t hilo;
	//Hilos detachables con manejo de errores tienen que ser logs
	int  res = pthread_attr_init(&attr);
	if (res != 0) {
		log_error(log_operaciones, "Error en los atributos del hilo");
	}
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (res != 0) {
		log_error(log_operaciones, "Error en el seteado del estado de detached");
	}
	res = (modulo == instancia) ? pthread_create (&hilo ,&attr, rutina_instancia , (void *)nuevo_socket)
			:pthread_create (&hilo ,&attr, rutina_ESI, (void*)nuevo_socket);
	if (res != 0) {
		log_error(log_operaciones, "Error en la creacion del hilo");
	}
	log_info(log_operaciones, "Se ha creado un hilo con la rutina ESI");
	pthread_attr_destroy(&attr);
}

void mostrar_por_pantalla_config(ConfigCoordinador config) {
	puts("----------------PROCESO COORDINADOR--------------");
	printf("PUERTO: %i\n", config.puerto_escucha);
	printf("ALGORITMO: %s\n", config.algoritmo_distrib);
	printf("CANTIDAD DE ENTRADAS: %i\n", config.cant_entradas);
	printf("TAMANIO DE ENTRADAS: %i\n", config.tamanio_entrada);
	printf("RETARDO: %i\n", config.retardo);
}

void guardar_en_log(int id_esi, char* sentencia) {
	FILE* log_operacion = fopen(LOG_PATH, "a+");
	fprintf(log_operacion,"%d %s %s %s",id_esi,"		",sentencia,"\n");
	fclose(log_operacion);
}

void mostrar_archivo(char* path) {
	FILE *f = fopen(path, "r");
	if(f == NULL)
	  printf("Error al abrir el archivo\n");
	else
		while(!feof(f))
			printf("%c", getc(f));
	fclose(f);
}

void crear_log_operacion() {
	log_operaciones = log_create("operaciones_coordinador.log", "coordinador", 0, 1);
}

void destruir_log_operacion() {
	log_destroy(log_operaciones);
}
