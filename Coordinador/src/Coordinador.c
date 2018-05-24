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
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include "commons/string.h"
#include "Coordinador.h"
#include "commons/collections/list.h"

t_list *lista_Instancias;
ConfigCoordinador configuracion;

int main() {
	configuracion = cargar_config_coordinador();
	mostrar_por_pantalla_config(configuracion);
	lista_Instancias = list_create();

	int listener = crear_socket_de_escucha(configuracion.puerto_escucha);
	int socket_server = conexion_con_servidor("127.0.0.1", 9034);

	int bytes = enviar("ESI", socket_server);
	int nuevo_socket, modulo;

	while(1){
		nuevo_socket = aceptar_nueva_conexion(listener);
		recv(nuevo_socket, &modulo, sizeof(int), 0);//HS
		crear_hilo(nuevo_socket, modulo);
	}
	return 0;
}

//ACCIONES DE LOS HILOS
void *rutina_instancia(void * arg) {
	int socket_INST = (int)arg;
	printf("\n------------------------------------\n");
	printf("NUEVA INSTANCIA EJECUTADA || ");

	Nodo_Instancia* nuevaInstancia = malloc(sizeof(Nodo_Instancia));
	nuevaInstancia->socket = socket_INST;

	if(list_is_empty(lista_Instancias)){
		nuevaInstancia->inst_ID = 0;
	} else {
		Nodo_Instancia* aux = list_get(lista_Instancias, (lista_Instancias->elements_count-1));
		nuevaInstancia->inst_ID = (aux->inst_ID)+1;
		free(aux);
		}

	list_add(lista_Instancias,(void*)nuevaInstancia);
	printf("ID:%d || SOCKET: %d\n", nuevaInstancia->inst_ID, nuevaInstancia->socket);

	configurar_instancia(socket_INST);
	return NULL;
}

void configurar_instancia(int socket){
	configuracion = cargar_config_coordinador();

	int* dim = malloc(sizeof(int)*2);
	memcpy(dim,&configuracion.cant_entradas,sizeof(int));
	memcpy(dim+1,&configuracion.tamanio_entrada,sizeof(int));

	enviarMensaje(socket,1,dim,sizeof(int)*2);
}


void *rutina_ESI(void * arg) {
	int socket_CPU = (int)arg;
	return NULL;
}

void crear_hilo(int nuevo_socket, int modulo) {
	pthread_attr_t attr;
	pthread_t hilo;
	//Hilos detachables con manejo de errores tienen que ser logs
	int  res = pthread_attr_init(&attr);
	if (res != 0) {
		printf("Error en los atributos del hilo");
	}
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (res != 0) {
		printf("Error en el seteado del estado de detached");
	}
	res = (modulo == instancia) ? pthread_create (&hilo ,&attr, rutina_instancia , (void *)nuevo_socket)
			:pthread_create (&hilo ,&attr, rutina_ESI , (void *)nuevo_socket);
	if (res != 0) {
		printf("Error en la creacion del hilo");
	}
	pthread_attr_destroy(&attr);
}

int enviar(char* mensaje, int socket) {
	int longitud = strlen(mensaje) + 1;
	send(socket, &longitud, sizeof(int), 0);
	return send(socket, mensaje, longitud, 0);
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
	FILE* log_operacion = fopen(LOG_PATH, "a+");
	fprintf(log_operacion, "%s", "ESI		OPERACION\n\n");
	fclose(log_operacion);
}

