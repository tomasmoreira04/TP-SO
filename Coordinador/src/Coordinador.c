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

int compresion;
int compresiones_finalizadas = 0;
ConfigCoordinador configuracion;
int socket_plan; //esto cambiar tal vez
t_dictionary *instancias_Claves;
t_dictionary *lista_Instancias;
t_list* lista_instancias_new;
t_list *listaSoloInstancias;
int contadorEquitativeLoad;

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL); //a veces el printf no andaba, se puede hacer esto o un fflush
	compresion=1;
	int banderaPlanificador=0;
	contadorEquitativeLoad=0;
	instancias_Claves= dictionary_create();
	listaSoloInstancias=list_create();
	//lista_instancias_new = list_create(); por ahora, esta deprecada

	configuracion = cargar_config_coordinador(argv[1]);
	crear_log_operaciones();
	log_info(log_operaciones, "Se ha cargado la configuracion inicial del Coordinador");
	lista_Instancias = dictionary_create();
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
	//RECIBIR NOMBRE DE INSTACIA
	recibirMensaje(socket_INST,&stream);
	char* nombre_inst = (char*)stream;
	//list_add(lista_Instancias,(void*)nuevaInstancia);
	printf("ID:%s || SOCKET: %d\n", nombre_inst , socket_INST);
	//VALIDAR QUE SEA EL UNICO EN EL DICCIONARIO
	if( (dictionary_has_key(lista_Instancias , nombre_inst ) ) == false ){
		printf("socket %d agregado a instancia %s", socket_INST, nombre_inst);

		instancia_Estado_Conexion *instancia_conexion = malloc(sizeof(instancia_Estado_Conexion));
		instancia_conexion->estadoConexion=conectada;
		instancia_conexion->socket=socket_INST;
		instancia_conexion->entradas_disponibles = configuracion.cant_entradas;

		printf("\nel estado de la instacia es %d y su socket es %d\n\n",instancia_conexion->estadoConexion,instancia_conexion->socket);
		dictionary_put(lista_Instancias, nombre_inst , instancia_conexion);
		list_add(listaSoloInstancias, nombre_inst);
		configurar_instancia(socket_INST);

		//TOMI ROMPISTE TODO
		//nodo_inst_conexion_destroyer(instancia_conexion);

	}
	else{
		cambiarEstadoInstancia(nombre_inst,conectada);
	}
	return NULL;
}

void configurar_instancia(int socket){
	int* dim = malloc(sizeof(int)*2);//RECIBE 3
	memcpy(dim,&configuracion.cant_entradas,sizeof(int));
	memcpy(dim+1,&configuracion.tamanio_entrada,sizeof(int));
	enviarMensaje(socket,config_inst,dim,sizeof(int)*2);
	printf(YELLOW"\nNueva instancia configurada\n"RESET);
}

t_list* instancias_conectadas() {
	int n = list_size(listaSoloInstancias);
	t_list* lista = list_create();
	for(int i = 0; i < n; i++){
		char* nombre_inst = list_get(listaSoloInstancias, i);
		instancia_Estado_Conexion* instancia = dictionary_get(lista_Instancias, nombre_inst);
		if (instancia->estadoConexion == conectada)
			list_add(lista, instancia);
	}
	return lista;
}

void *rutina_ESI(void* argumento) {
	int socket_esi = *(int*)(&argumento);
	int id_esi;
	void* stream;
	printf(BLUE "\nEjecutando ESI (socket %d)..." RESET, socket_esi);

	while (recibirMensaje(socket_esi, &stream) == ejecutar_sentencia_coordinador) {

		t_sentencia sentencia = *(t_sentencia*)stream;
		id_esi = sentencia.id_esi;
		imprimir_sentencia(sentencia);

		enviarMensaje(socket_plan, sentencia_coordinador, &sentencia, sizeof(t_sentencia));

		int sentencia_okey = recibirMensaje(socket_plan, &stream); //el planif me da el OK, entonces ejecuto una sentencia del esi

		usleep(configuracion.retardo * 1000);

		esperar_compactacion();

		if (sentencia_okey == sentencia_coordinador) {

			switch(sentencia.tipo){
				case S_GET:
				{
					if( (dictionary_has_key(instancias_Claves , sentencia.clave) )==false ) {
						//Persistir
						dictionary_put(instancias_Claves, sentencia.clave , "0" );//VERIFICAR SI ES EN VARIABLE
						log_info(log_operaciones, formatear_mensaje_esi(1, S_GET, sentencia.clave, NULL));
					}
					break;
				}
				case S_STORE:
				{
					char* instanciaGuardada = dictionary_get(instancias_Claves , sentencia.clave);
					instancia_Estado_Conexion* conexion_instancia = dictionary_get(lista_Instancias, instanciaGuardada);
					int socketEncontrado = conexion_instancia->socket;
					enviarMensaje(socketEncontrado, ejecutar_sentencia_instancia,  &sentencia,sizeof(t_sentencia));

					void* cantidad_entradas;
					int operacionPedidaInstacia = recibirMensaje(socketEncontrado, &cantidad_entradas);
					actualizar_instancia(instanciaGuardada, *((int*)cantidad_entradas));
					switch(operacionPedidaInstacia){
						case 0:
						{
							int  ESI_bloq = sentencia.id_esi;
							cambiarEstadoInstancia(instanciaGuardada, desconectada);
							printf(RED"Se desconecto la Instancia  %s\n"RESET, instanciaGuardada);
							enviarMensaje(socket_plan, esi_bloqueado, &ESI_bloq, sizeof(int));
							break;
						}
						case ejecucion_ok:
						{
							//printf(GREEN"Ejecucion OK!\n"RESET);
							break;
						}
						default:
						{
							printf(CYAN"IVAN ILUMINANOS\n"RESET);
							break;
						}
					}
					//MANDAR A INSTANCIA
					break;
				}
				case S_SET:
				{
					char* instancia = "0";

					if(!clave_tiene_instancia(sentencia.clave)) //atencion al !
						instancia = aplicar_algoritmo(sentencia.clave, sentencia.valor);
					else {
						printf(RED"\nLa clave ya esta seteada en una instancia\n"RESET);
						instancia = buscar_instancia(sentencia.clave);
					}

					printf("\nBuscando socket de instancia %s\n", instancia);

					char* instanciaGuardada = dictionary_get(instancias_Claves , sentencia.clave);
					/*int largoSentencia = strlen(nombre_instancia) + 1;
					char* instanciaGuardada = malloc(largoSentencia);
					strcpy(instanciaGuardada, nombre_instancia );*/

					int socket = (*(instancia_Estado_Conexion*) dictionary_get(lista_Instancias , instancia)).socket;

					//VALIDAR INSTANCIA CONECTADA
					//MANDAR A INSTANCIA
					enviarMensaje(socket, ejecutar_sentencia_instancia, &sentencia, sizeof(t_sentencia));

					//VER BIEN DONDE VA ESTO, le envio al planif la instancia que esta la clave
					avisar_guardado_planif(instancia, sentencia.clave);

					void* cantidad_entradas;
					int operacionPedidaInstacia = recibirMensaje(socket, &cantidad_entradas);
					//COMPACTAR

					switch(operacionPedidaInstacia){
						case 0:
						{
							int  ESI_bloq= sentencia.id_esi;
							cambiarEstadoInstancia(instanciaGuardada,desconectada);
							printf("SE DESCONECTO LA CHINGADA INSTANCIA\n");
							enviarMensaje(socket_plan, esi_bloqueado, &ESI_bloq, sizeof(int));
							break;
						}

						case ejecucion_ok:
						{
							//printf("GREAT ejecucion_ok\n");
							break;
						}
						case compactar:
						{
							avisar_compactacion();
							compresion = 1;
							void * asd;
							int resultado = recibirMensaje(socket, &asd);
							printf("%d", resultado);
							if(resultado == compactacion_ok) {
								printf("Termino de compactar la instancia del socket %d", socket);
								compresiones_finalizadas++;
							}
							else if (resultado == 0) {
								int  ESI_bloq = sentencia.id_esi;
								printf("SE DESCONECTO LA CHINGADA INSTANCIA\n");
								enviarMensaje(socket_plan, esi_bloqueado, &ESI_bloq, sizeof(int));
							}
							else {
								if (compresiones_finalizadas < list_size(instancias_conectadas())) {
									compresion = 0;
									compresiones_finalizadas = 0;
								}
							}
							break;
						}

						default:
						{
							printf("IVAN ILUMINANOS\n");
							break;
						}
					}
					break;
				}
				default:{
					log_error(log_operaciones,"Error al identificar la operacion en el coordinador");
					break;
				}
			}
			int variable = exitoso;

			enviarMensaje(socket_esi, ejecucion_ok, &variable, sizeof(int));
		}
		else if(sentencia_okey== esi_bloqueado){
			printf(YELLOW "\nESI bloqueado\n" RESET);
			int variable = bloqueado;
			enviarMensaje(socket_esi, ejecucion_bloqueado, &variable, sizeof(int));
		}
		else{
			printf(RED "\nERROR\n" RESET);
			int variable = no_exitoso;
			enviarMensaje(socket_esi, ejecucion_no_ok, &variable, sizeof(int));
			break;
		}
	}
	//sale del while -> no hay mas sentencias
	printf(YELLOW"\nFinalizando ESI %d\n"RESET, id_esi);
	enviarMensaje(socket_plan, terminar_esi, &id_esi, sizeof(id_esi));
	return NULL;
}

void avisar_compactacion() {
	t_list* conectadas = instancias_conectadas();
	int n = list_size(conectadas);
	for (int i = 0; i < n; i++) {
		instancia_Estado_Conexion* instancia = list_get(conectadas, i);
		printf("Compactando instancia en socket %d", instancia->socket);
		avisar(instancia->socket, compactar);
	}
	printf(RED"\nCOMPACTANDO INSTANCIAS\n"RESET);
}

void cambiarEstadoInstancia(char *instanciaGuardada, estado_de_la_instancia accion){
	instancia_Estado_Conexion *aux=malloc(sizeof(instancia_Estado_Conexion));
	dictionary_remove(lista_Instancias, instanciaGuardada);
	aux->estadoConexion=accion;
	aux->socket=0;
	dictionary_put(instancias_Claves, instanciaGuardada, aux);
}

void avisar_guardado_planif(char* instancia, char* clave) {
	t_clave* respuesta = malloc(sizeof(t_clave));
	strcpy(respuesta->clave, clave);
	strcpy(respuesta->instancia, instancia);
	enviarMensaje(socket_plan, clave_guardada_en_instancia, respuesta, sizeof(t_clave));
}

int clave_tiene_instancia(char* clave) {
	return strcmp((char*)dictionary_get(instancias_Claves, clave), "0") == 0 ? 0 : 1; //xd
}

char* aplicar_algoritmo(char* clave, char* valor) { //DEVUELVE EL NOMBRE DE LA INSTANCIA ASIGNADA
	switch(configuracion.algoritmo) {
		case el:	equitative_load(clave);		break;
		case lsu:	least_space_used(clave);	break;
		case ke: 	key_explicit(clave); 		break;
		default: 								break;
	}
	log_info(log_operaciones, formatear_mensaje_esi(1, S_SET, clave, valor)); //por qué 1 xd?
	return (char*) dictionary_get(instancias_Claves , clave);
}

void crear_hilo(int nuevo_socket, int modulo) {
	pthread_attr_t attr;
	pthread_t hilo;
	//Hilos detachables con manejo de errores tienen que ser logs
	int  res = pthread_attr_init(&attr);
	if (res != 0) {
		log_error(log_operaciones, "Error en los atributos del hilo func: crear_hilo");
	}
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (res != 0) {
		log_error(log_operaciones, "Error en el seteado del estado de detached. func: crear_hilo");
	}
	res = (modulo == instancia) ? pthread_create (&hilo ,&attr, rutina_instancia , (void *)nuevo_socket)
			:pthread_create (&hilo ,&attr, rutina_ESI, (void*)nuevo_socket);
	if (res != 0) {
		log_error(log_operaciones, "Error en la creacion del hilo");
	}
	log_info(log_operaciones, "Se ha creado un hilo con la rutina ESI");
	pthread_attr_destroy(&attr);
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

void crear_log_operaciones() {
	log_operaciones = log_create("operaciones_coordinador.log", "coordinador", 0, 1);
}

void esperar_compactacion() {
	int hay_que_compactar = 0;
	int instancias = list_size(instancias_conectadas());
	while(compresion != 1 && compresiones_finalizadas < instancias)
		hay_que_compactar = 1;
	printf("asd");
	/*if (hay_que_compactar == 1)
		printf(GREEN"\nCOMPACTACION FINALIZADA\n"RESET);*/
}

void modificar_clave(char* clave, char* instancia) {
	dictionary_remove(instancias_Claves, clave);
	dictionary_put(instancias_Claves, clave, instancia);
}

void actualizar_instancia(char* instancia, int entradas) {
	instancia_Estado_Conexion *aux = (instancia_Estado_Conexion*) dictionary_get(lista_Instancias, instancia);
	aux->entradas_disponibles = entradas;
	dictionary_remove(lista_Instancias, instancia);
	dictionary_put(lista_Instancias, instancia, aux);
}

void equitative_load(char* claveSentencia){
	log_info(log_operaciones, "Aplicando Equitative Load..");
	int cantidadInstancias = dictionary_size(lista_Instancias)-1;
	char* instancia = list_get(listaSoloInstancias, contadorEquitativeLoad);
	int estadoInstancia = estadoDeInstancia(instancia);
	if( estadoInstancia==conectada ){
		modificar_clave(claveSentencia, instancia);
		contador_EQ(cantidadInstancias);
	}
	else{
		if(estadoInstancia==desconectada){
			equitative_load(claveSentencia);
		}
		else{
			printf(RED"\nERROR\n"RESET);
		}
	}
}

void contador_EQ(int cantidadDeInstancias){
	if(contadorEquitativeLoad+1>cantidadDeInstancias){
		contadorEquitativeLoad = 0;
	}
	else{
		contadorEquitativeLoad=contadorEquitativeLoad+1;
	}
}

int estadoDeInstancia(char * instancia){
	instancia_Estado_Conexion* estado = dictionary_get(lista_Instancias,instancia);
	return estado->estadoConexion;
}

void key_explicit(char* claveSentencia){
	fflush(stdin);
	t_list *listaKE;
	listaKE= list_create();

	int comienzo=(claveSentencia[0])-96;

	int instanciasActivas=0;
	int tamano_lista_instancias=list_size(listaSoloInstancias);

	for(int i=0;i<tamano_lista_instancias;i++){
		if(((*(instancia_Estado_Conexion*)dictionary_get(lista_Instancias,((char*)list_get(listaSoloInstancias,i)))).estadoConexion)==conectada){
			list_add(listaKE, ((char*)list_get(listaSoloInstancias,i)));
			instanciasActivas++;
		}
	}
	float asignacion_por_instancia= (float) 26  /  (float)instanciasActivas;
	float mult=1;
	for(int j=1;j<=26;j++){
		if( (asignacion_por_instancia*mult) < comienzo ){
			mult++;
		}
		if(comienzo==j){
			modificar_clave(claveSentencia, ((char*)list_get(listaKE, (mult-1) )) );
			break;
		}
	}

}


void least_space_used(char* clave) {
	t_list* lista_inst_activa = list_create();
	int instancias_activas = 0;
	for(int i=0 ; i < list_size(listaSoloInstancias) ; i++) {
		if(((*(instancia_Estado_Conexion*)dictionary_get(lista_Instancias,((char*)list_get(listaSoloInstancias,i)))).estadoConexion)==conectada){
			list_add(lista_inst_activa, ((char*)list_get(listaSoloInstancias,i)));
			instancias_activas++;
		}
	}
	//char* instancia_elegida = malloc(80);

	for(int j=1; j<instancias_activas; j++) {
		if((*(instancia_Estado_Conexion*)(dictionary_get(lista_Instancias, (char*)list_get(lista_inst_activa, j)))).entradas_disponibles >
			(*(instancia_Estado_Conexion*)(dictionary_get(lista_Instancias, (char*)list_get(lista_inst_activa, (j-1))))).entradas_disponibles) {
			//list_get(lista_inst_activa, j);
			//strcpy(instancia_elegida, (char*)list_get(lista_inst_activa, j));
			modificar_clave(clave, (char*)list_get(lista_inst_activa, j));
		}
		else {
			//strcpy(instancia_elegida, (char*)list_get(lista_inst_activa, (j+1)));
			modificar_clave(clave, (char*)list_get(lista_inst_activa, (j+1)));
		}
	}
}

char* formatear_mensaje_esi(int id, TipoSentencia t, char* clave, char* valor) {

	char* formato_string = string_new();
	char* str_id = string_itoa(id);
	string_append(&formato_string, "El ESI ");
	string_append(&formato_string, str_id);
	string_append(&formato_string, " hizo un");
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
	dictionary_destroy_and_destroy_elements(instancias_Claves, free);
	dictionary_destroy_and_destroy_elements(lista_Instancias, (void*)nodo_inst_conexion_destroyer);
	list_destroy_and_destroy_elements(listaSoloInstancias, free);
	//list_destroy(lista_instancias_new); POR AHORA DEPRECADO, SOLO POR AHORA
}

void imprimir_sentencia(t_sentencia sentencia) {
	char* tipo;
	switch (sentencia.tipo) {
		case S_GET: tipo = "GET"; break;
		case S_SET: tipo = "SET"; break;
		default: tipo = "STORE"; break;
	}
	printf("\nSentencia "CYAN"%s"RESET" del "GREEN"ESI %d"RESET" con clave "RED"%s"RESET, tipo, sentencia.id_esi, sentencia.clave);
}

char* buscar_instancia(char* clave) {
	return (char*)dictionary_get(instancias_Claves, clave);
}

void nodo_inst_conexion_destroyer(instancia_Estado_Conexion* inst) {
	free(inst);
}
