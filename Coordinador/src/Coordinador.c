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
#include "../../Bibliotecas/src/Semaforo.c"
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include "commons/string.h"
#include "Coordinador.h"
#include "commons/collections/list.h"
#include <unistd.h>


// Diferenciacion de operacion ESI
// Direnciar quien se conecta
// Guardar con ID instancia
// TOMI SE LA COME


//t_list *lista_Instancias;
ConfigCoordinador configuracion;
int socket_plan; //esto cambiar tal vez
t_dictionary *instancias_Claves;
t_dictionary *lista_Instancias;
t_list* lista_instancias_new;
t_list *listaSoloInstancias;
int contadorEquitativeLoad;

int main(int argc, char* argv[]) {
	int banderaPlanificador=0;
	contadorEquitativeLoad=0;
	instancias_Claves= dictionary_create();
	listaSoloInstancias=list_create();
	lista_instancias_new = list_create();

	configuracion = cargar_config_coordinador(argv[1]);

	crear_log_operacion();
	log_info(log_operaciones, "Se ha cargado la configuracion inicial del Coordinador");
	mostrar_por_pantalla_config(configuracion);
	lista_Instancias =dictionary_create();
	int listener = crear_socket_de_escucha(configuracion.puerto_escucha);
	int nuevo_socket, modulo;
	while(1){
		if(banderaPlanificador == 0){
			nuevo_socket = aceptar_nueva_conexion(listener);
			recv(nuevo_socket, &modulo, sizeof(int), 0);//HS
			crear_hilo(nuevo_socket, modulo);

			int socket_server = conexion_con_servidor(configuracion.ip_planificador, configuracion.puerto_planificador);
			socket_plan = socket_server; //pasar ocmo parametro, no global, pero por ahora lo hago asi
			avisar(socket_plan, conectar_coord_planif);
			banderaPlanificador = 1;
		}
		nuevo_socket = aceptar_nueva_conexion(listener);
		recv(nuevo_socket, &modulo, sizeof(int), 0);//HS
		crear_hilo(nuevo_socket, modulo);
	}
	destruir_estructuras_globales();
	return 0;
}

//ACCIONES DE LOS HILOS
void *rutina_instancia(void * arg) {
	int socket_INST = (int)arg;
	printf("\n------------------------------------\n");
	printf("NUEVA INSTANCIA EJECUTADA || ");
	void * stream;
	Nodo_Instancia* nodo = NULL;
	//RECIBIR NOMBRE DE INSTACIA
	recibirMensaje(socket_INST,&stream);
	char* nombre_inst = (char*)stream;
	//list_add(lista_Instancias,(void*)nuevaInstancia);
	printf("ID:%s || SOCKET: %d\n", nombre_inst , socket_INST);
	//VALIDAR QUE SEA EL UNICO EN EL DICCIONARIO
	if( (dictionary_has_key(lista_Instancias , nombre_inst ) ) == false ){
		printf("socket %d agregado a instancia %s", socket_INST, nombre_inst);

		int* sock = (int*)socket_INST;
		dictionary_put(lista_Instancias, nombre_inst , sock);
		/*>>>propuesta tomi
		 * para que sea mas facil el lsu
		nodo->socket = sock;
		strcpy(nodo->inst_ID, nombre_inst);
		nodo->entradas_desocupadas = configuracion.cant_entradas;
		list_add(lista_instancias_new, nodo);
		*
		 <<<*/
		list_add(listaSoloInstancias, nombre_inst);
	}
	configurar_instancia(socket_INST);
	return NULL;
}

void configurar_instancia(int socket){
	int* dim = malloc(sizeof(int)*2);//RECIBE 3
	memcpy(dim,&configuracion.cant_entradas,sizeof(int));
	memcpy(dim+1,&configuracion.tamanio_entrada,sizeof(int));
	enviarMensaje(socket,config_inst,dim,sizeof(int)*2);
	printf(YELLOW"\nNueva instancia configurada\n"RESET);
}

void *rutina_ESI(void* argumento) {
	int socket_esi = *(int*)(&argumento);
	int id_esi;
	void* stream;
	printf(BLUE "\nEjecutando ESI (socket %d)..." RESET, socket_esi);

	while (recibirMensaje(socket_esi, &stream) == ejecutar_sentencia_coordinador) {

		t_sentencia sentencia = *(t_sentencia*)stream;
		id_esi = sentencia.id_esi;

		printf("\nSentencia: tipo:%d -esi:%d -clave:%s\n", sentencia.tipo, sentencia.id_esi, sentencia.clave);

		enviarMensaje(socket_plan, sentencia_coordinador, &sentencia, sizeof(t_sentencia));
		printf("\nesperando respuesta planif\n");

		int sentencia_okey = recibirMensaje(socket_plan, &stream); //el planif me da el OK, entonces ejecuto una sentencia del esi

		usleep(1000000); //1 segundo
		//usleep(configuracion.retardo);

		if (sentencia_okey == sentencia_coordinador) {

			printf("\nestoy haciendo la sentencia\n");

			switch(sentencia.tipo){
				case S_GET:
				{
					printf("\nestoy un get\n");
					if( (dictionary_has_key(instancias_Claves , sentencia.clave) )==false ) {
						//Persistir
						dictionary_put(instancias_Claves, sentencia.clave , "0" );//VERIFICAR SI ES EN VARIABLE
						//char* msg_get = formatear_mensaje_esi(1, S_SET, sentencia->clave, NULL);
						//no se si funcionara lo de abajo, sino pasarle msg_get
						//el primer parametro debe ser el id del ESI, pongo 1 para que no rompa
						log_info(log_operaciones, formatear_mensaje_esi(1, S_GET, sentencia.clave, NULL));
					}
					break;
				}
				case S_SET:
				{
					printf(RED"\n%s\n"RESET, sentencia.valor);
					char* valor = (char*) dictionary_get(instancias_Claves , sentencia.clave);
					char* instancia = valor;
					printf("\nvalor del diction: %s\n", valor);

					if( ( strcmp(valor, "0") ) == 0 ){
						printf("\nentraste al strcmp de 0\n");
						//ALGORITMO Y ASIGNAR, modificar claveSentencia
						if(  strcmp(configuracion.algoritmo_distrib, "EL") == 0 ){
							printf("\nhaciendo equitav load\n");
							//log_info(log_operaciones, "Aplicando Equitative Load..");
							EquitativeLoad(sentencia.clave);
							//free(instanciaGuardada);
							instancia = (char*) dictionary_get(instancias_Claves , sentencia.clave);
							printf("%s", instancia);
							//log_info(log_operaciones, formatear_mensaje_esi(1, S_SET, sentencia.clave, sentencia.valor));
						}
						else{
							if( strcmp(configuracion.algoritmo_distrib,"LSU") == 0 ) {
								//KE
								//log_info(log_operaciones, "Aplicando Least Space Used..");
							}
							else{
								if((strcmp(configuracion.algoritmo_distrib,"KE"))==0){
									//LSU
									//log_info(log_operaciones, "Aplicando Key Explicit..");
								}
								else{
									//LOG DE ERROR DE ALGORITMO
									log_error(log_operaciones, "Se envio un algoritmo invalido");
								}
							}
						}
					}
					printf("\nbuscando socket de instancia %s\n", instancia);
					int socket = (int) dictionary_get(lista_Instancias , instancia);
					printf("\nSOCKET: %d\n", socket);

					//VALIDAR INSTANCIA CONECTADA
					//MANDAR A INSTANCIA
					enviarMensaje(socket, ejecutar_sentencia_instancia, &sentencia, sizeof(t_sentencia));
					void* mensajeInstancia;
					int operacionPedidaInstacia = recibirMensaje(socket, &mensajeInstancia);
					//COMPACTAR
					break;
				}
				case S_STORE:
				{
					int largoSentencia= strlen((char*) (dictionary_get(instancias_Claves , sentencia.clave)));
					char* instanciaGuardada=malloc(largoSentencia);
					strcpy(instanciaGuardada, (char*) (dictionary_get(instancias_Claves , sentencia.clave)) );


					//VALIDADA
					int socketEncontrado= (int) (dictionary_get(lista_Instancias , instanciaGuardada));
					enviarMensaje(socketEncontrado, ejecutar_sentencia_instancia,  &sentencia,sizeof(t_sentencia));
					//MANDAR A INSTANCIA
					break;
				}
				default:{
					log_error(log_operaciones,"Error al identificar la operacion en el coordinador");
					break;
				}
			}
			int variable = exitoso;
			printf("\nestoy mandando el exec ok al planif\n");
			enviarMensaje(socket_esi, ejecucion_ok, &variable, sizeof(int));
			printf("\nya mande el exec ok al planifa\n");

		}
		else if(sentencia_okey== esi_bloqueado){

		}
		else{
			printf(RED "\nERROR\n" RESET);
			int variable = no_exitoso;
			enviarMensaje(socket_esi, ejecucion_no_ok, &variable, sizeof(int));
			break;
		}
	}
	//sale del while -> no hay mas sentencias
	printf(YELLOW"\nfinalizando esi\n"RESET);
	enviarMensaje(socket_plan, terminar_esi, &id_esi, sizeof(id_esi));
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

/*void guardar_en_log(int id_esi, char* sentencia) {
	FILE* log_operacion = fopen(LOG_PATH, "a+");
	fprintf(log_operacion,"%d %s %s %s",id_esi,"		",sentencia,"\n");
	fclose(log_operacion);
}*/

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

/*void destruir_log_operacion() {
	log_destroy(log_operaciones);
}*/

void modificar_clave(char* clave, char* instancia) {
	dictionary_remove(instancias_Claves, clave);
	dictionary_put(instancias_Claves, clave, instancia);
}

void EquitativeLoad(char* claveSentencia){
	int cantidadInstancias = dictionary_size(lista_Instancias)-1;
	char* instancia = list_get(listaSoloInstancias, contadorEquitativeLoad);
	printf("\nEQLOAD obtuvo instancia: %s\n", instancia);

	modificar_clave(claveSentencia, instancia);

	if(contadorEquitativeLoad+1>cantidadInstancias){
		contadorEquitativeLoad = 0;
	}
	else{
		contadorEquitativeLoad=contadorEquitativeLoad+1;
	}
}

//ESTO FUNCIONA PARA LA LISTA PROPUESTA lista_instancias_new
void least_space_used(char* claveSentencia) {

	bool comparator_entradas_max(Nodo_Instancia* m, Nodo_Instancia* n) {
			return m->entradas_desocupadas >= n->entradas_desocupadas;
		}

	t_list* lista_aux = list_create();
	lista_aux = list_duplicate(lista_instancias_new);
	list_sort(lista_aux, (void*) comparator_entradas_max);
	Nodo_Instancia* nodo_maximo = list_get(lista_aux, 0);
	char* instancia_max;
	strcpy(instancia_max, nodo_maximo->inst_ID);
	modificar_clave(claveSentencia, instancia_max);
}

char* formatear_mensaje_esi(int id, TipoSentencia t, char* clave, char* valor) {

	char* formato_string = string_new();
	char* str_id = string_itoa(id);
	string_append(&formato_string, "El ESI ");
	string_append(&formato_string, str_id);
	string_append(&formato_string, " hizo un");
	printf("%d", t);
	switch(t) {
		case S_SET:
			string_append(&formato_string, " SET sobre ");
			string_append(&formato_string, clave);
			string_append(&formato_string, ":");
			string_append(&formato_string, valor);
			break;
		case S_GET:
			string_append(&formato_string, " GET de la clave ");
			string_append(&formato_string, clave);
			break;
		case S_STORE:
			string_append(&formato_string, " STORE de la clave ");
			string_append(&formato_string, clave);
			break;
		default:
			log_error(log_operaciones, "Error al formatear el log");
	}
	return formato_string;
}

void destruir_estructuras_globales() {
	log_destroy(log_operaciones);
	dictionary_destroy(instancias_Claves);
	dictionary_destroy(lista_Instancias);
	list_destroy(listaSoloInstancias);
	list_destroy(lista_instancias_new);
}
