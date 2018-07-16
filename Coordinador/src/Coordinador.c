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

int compresiones_finalizadas = 0;
ConfigCoordinador configuracion;
int socket_plan; //esto cambiar tal vez
t_dictionary *instancias_Claves;
t_dictionary *lista_Instancias;
t_list* lista_instancias_new;
t_list *listaSoloInstancias;
int contadorEquitativeLoad;

pthread_mutex_t semaforo_compactacion = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL); //a veces el printf no andaba, se puede hacer esto o un fflush
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

void* espera_desconexion(int socket_inst) {
	while(true) {
		void* b;
		int a = recibirMensaje(socket_inst, &b);

	}
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
	printf(BLUE "\nNueva rutina ESI en socket"CYAN" %d."RESET, socket_esi);
	while (recibirMensaje(socket_esi, &stream) == ejecutar_sentencia_coordinador) {
		t_sentencia sentencia = *(t_sentencia*)stream;
		id_esi = sentencia.id_esi;
		enviarMensaje(socket_plan, sentencia_coordinador, &sentencia, sizeof(t_sentencia));
		int	accion = recibirMensaje(socket_plan, &stream);
		usleep(configuracion.retardo * 1000);
		procesar_permiso_planificador(accion, sentencia, socket_esi);
	}
	//sale del while -> no hay mas sentencias
	printf(YELLOW"\nEl ESI %d ha terminado su rutina. Finalizando ESI...\n"RESET, id_esi);
	enviarMensaje(socket_plan, terminar_esi, &id_esi, sizeof(id_esi));
	return NULL;
}

void procesar_permiso_planificador(Accion mensaje, t_sentencia sentencia, int socket_esi) { //el planif me da el ok para ejecutar
	int resultado; //lo mando luego de ejecutar
	switch (mensaje) {
		case sentencia_coordinador:
			realizar_sentencia(sentencia);
			resultado = exitoso;
			break;
		case esi_bloqueado:
			printf(YELLOW "\nESI bloqueado por el planificador" RESET);
			resultado = bloqueado; //antes era exitoso /?
			break;
		case error:
		default:
			printf(RED"\nERROR" RESET);
			resultado = no_exitoso;
			break;
	}
	enviarMensaje(socket_esi, resultado_ejecucion, &resultado, sizeof(int));
}

void GET(t_sentencia sentencia) {
	if(!existe_clave(sentencia.clave)) {
		dictionary_put(instancias_Claves, sentencia.clave, "0" );
		log_info(log_operaciones, formatear_mensaje_esi(sentencia.id_esi, S_GET, sentencia.clave, NULL));
	}
}

void SET(t_sentencia sentencia) {
	char* instancia = buscar_instancia(sentencia.clave);
	if(!clave_tiene_instancia(sentencia.clave))
		instancia = aplicar_algoritmo(sentencia);

	printf("\nBuscando socket de instancia %s", instancia);
	instancia_Estado_Conexion* conexion = dictionary_get(lista_Instancias , instancia);
	int socket = conexion->socket;

	//VALIDAR INSTANCIA CONECTADA
	enviarMensaje(socket, ejecutar_sentencia_instancia, &sentencia, sizeof(t_sentencia));
	avisar_guardado_planif(instancia, sentencia.clave); //aviso de clave guardada en tal instancia

	void* cantidad_entradas;
	int operacion = recibirMensaje(socket, &cantidad_entradas);
	procesar_pedido_instancia(operacion, instancia, sentencia.id_esi);
}

void STORE(t_sentencia sentencia) {
	char* instancia = buscar_instancia(sentencia.clave);
	instancia_Estado_Conexion* conexion = dictionary_get(lista_Instancias, instancia);
	int socket = conexion->socket;
	enviarMensaje(socket, ejecutar_sentencia_instancia, &sentencia, sizeof(t_sentencia));

	void* cantidad_entradas;
	int operacion = recibirMensaje(socket, &cantidad_entradas);
	actualizar_instancia(instancia, *((int*)cantidad_entradas));
	procesar_pedido_instancia(operacion, instancia, sentencia.id_esi);
}

void realizar_sentencia(t_sentencia sentencia) {

	s_wait(&semaforo_compactacion);

	imprimir_sentencia(sentencia);
	switch(sentencia.tipo) {
		case S_GET:		GET(sentencia); 	break;
		case S_SET: 	SET(sentencia); 	break;
		case S_STORE: 	STORE(sentencia); 	break;
		default: 		log_error(log_operaciones,"Error al identificar la operacion en el coordinador"); break;
	}

	s_signal(&semaforo_compactacion);
}

void procesar_pedido_instancia(Accion operacion, char* instancia, int esi) {
	switch(operacion){
		case ejecucion_ok:
			printf(GREEN"\nLa instancia ejecuto la operacion de forma exitosa."RESET);
			break;
		case compactar:
			hilo_avisar_compactacion(); //creo un hilo que espera las compactaciones
			break;
		case error:
		default:
			printf(RED"\nLa instancia no pudo ejecutar la operacion. Finalizando ESI."RESET);
			enviarMensaje(socket_plan, terminar_esi, &esi, sizeof(int));
			printf(RED"\nDesconectando "CYAN"%s."RESET, instancia);
			cambiarEstadoInstancia(instancia, desconectada);
			break;
	}
}

void* avisar_compactacion() {
	s_wait(&semaforo_compactacion);
	t_list* conectadas = instancias_conectadas();
	int n = list_size(conectadas);
	for (int i = 0; i < n; i++) {
		instancia_Estado_Conexion* instancia = list_get(conectadas, i);
		hilo_compactacion(instancia->socket); //creo un hilo para cada instancia
	}
	printf(RED"\nCOMPACTANDO INSTANCIAS\n"RESET);
	esperar_compactacion(); //espera activa
	printf(GREEN"\nCOMPACTACION FINALIZADA\n"RESET);
	compresiones_finalizadas = 0;
	s_signal(&semaforo_compactacion);
	return NULL;
}

void hilo_avisar_compactacion() {
	pthread_attr_t attr;
	pthread_t hilo;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (&hilo ,&attr, avisar_compactacion, NULL);
	pthread_attr_destroy(&attr);
}

void hilo_compactacion(int socket_instancia) {
	pthread_attr_t attr;
	pthread_t hilo;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (&hilo ,&attr, rutina_compactacion, (void *)socket_instancia);
	pthread_attr_destroy(&attr);
}

void* rutina_compactacion(void* sock) {
	int socket_instancia = (int)sock;
	void* stream;
	printf(GREEN"\nCompactando instancia en socket %d"RESET, socket_instancia);
	avisar(socket_instancia, compactar); //aviso a la instancia que debe compactar

	int resultado = recibirMensaje(socket_instancia, &stream); //espero que compacte
	switch(resultado) {
	case compactacion_ok:
		printf(GREEN"\nTermino de compactar la instancia del socket %d"RESET, socket_instancia);
		compresiones_finalizadas++; //una vez que el contador es igual al total de instancias, todas terminaron
		break;
	case error:
	default:
		if (compresiones_finalizadas < list_size(instancias_conectadas()))
			compresiones_finalizadas = 0;
		break;
	}
	return NULL;
}

void cambiarEstadoInstancia(char *instanciaGuardada, estado_de_la_instancia accion){
	instancia_Estado_Conexion *estado = dictionary_get(lista_Instancias, instanciaGuardada);
	/*instancia_Estado_Conexion *aux=malloc(sizeof(instancia_Estado_Conexion));
	dictionary_remove(lista_Instancias, instanciaGuardada);
	aux->estadoConexion=accion;
	aux->socket=0; //Y ESTO POR QUE?
	dictionary_put(instancias_Claves, instanciaGuardada, aux);*/
	estado->estadoConexion = accion;
}

void avisar_guardado_planif(char* instancia, char* clave) {
	t_clave* respuesta = malloc(sizeof(t_clave));
	strcpy(respuesta->clave, clave);
	strcpy(respuesta->instancia, instancia);
	enviarMensaje(socket_plan, clave_guardada_en_instancia, respuesta, sizeof(t_clave));
}


int clave_tiene_instancia(char* clave) {
	return strcmp((char*)dictionary_get(instancias_Claves, clave), "0") == 0 ? 0 : 1;
}

char* aplicar_algoritmo(t_sentencia sentencia) { //DEVUELVE EL NOMBRE DE LA INSTANCIA ASIGNADA
	switch(configuracion.algoritmo) {
		case el:	equitative_load(sentencia.clave);		break;
		case lsu:	least_space_used(sentencia.clave);	break;
		case ke: 	key_explicit(sentencia.clave); 		break;
		default: 								break;
	}
	log_info(log_operaciones, formatear_mensaje_esi(sentencia.id_esi, S_SET, sentencia.clave, sentencia.valor));
	return (char*) dictionary_get(instancias_Claves , sentencia.clave);
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
	switch (modulo) {
	case instancia:
		res = pthread_create (&hilo ,&attr, rutina_instancia , (void *)nuevo_socket);
		break;
	case esi:
		res = pthread_create (&hilo ,&attr, rutina_ESI , (void *)nuevo_socket);
		break;
	case consola:
		res = pthread_create (&hilo ,&attr, rutina_consulta , (void *)nuevo_socket);
		break;
	default:
		break;
	}
	if (res != 0) {
		log_error(log_operaciones, "Error en la creacion del hilo");
	}
	//log_info(log_operaciones, "Se ha creado un hilo con la rutina ESI");
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
	int instancias = list_size(instancias_conectadas());
	while(compresiones_finalizadas != instancias);
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
	if( estadoInstancia == conectada ){
		modificar_clave(claveSentencia, instancia);
		contador_EQ(cantidadInstancias);
	}
	else if(estadoInstancia == desconectada)
		equitative_load(claveSentencia);
	else
		printf(RED"\nERROR\n"RESET); //esto nunca pasa nigga
}

void contador_EQ(int cantidadDeInstancias){
	int c = contadorEquitativeLoad, i = cantidadDeInstancias;
	contadorEquitativeLoad = c + 1 > i ? 0 : c + 1;
}

int estadoDeInstancia(char* instancia){
	instancia_Estado_Conexion* estado = dictionary_get(lista_Instancias,instancia);
	estado->estadoConexion = enviar_check_conexion_instancia(estado->socket);
	return estado->estadoConexion;
}

int enviar_check_conexion_instancia(int socket) {
	header h = {.accion = verificar_conexion, .tamano = sizeof(int)};
	void* stream = malloc(sizeof(header) + sizeof(int) );
	int mensaje = 1;
	memcpy(stream, &h, sizeof(header));
	memcpy(stream + sizeof(header), &mensaje, sizeof(int));
	int enviado = send(socket, stream,sizeof(header) + sizeof(int), 0);
	free(stream);
	return enviado == -1 ? desconectada : conectada;
}

void key_explicit(char* claveSentencia){
	t_list* instancias_activas = lista_instancias_activas();
	int cantidad_instancias = list_size(instancias_activas);
	int comienzo = claveSentencia[0] - ('a' - 1);

	float asignacion_por_instancia = (float) 26  /  (float)cantidad_instancias;
	float multiplicador = 1;
	for(int j = 1; j <= 26; j++){
		if(asignacion_por_instancia * multiplicador < comienzo)
			multiplicador++;
		if(comienzo == j) {
			modificar_clave(claveSentencia, (char*)list_get(instancias_activas, multiplicador - 1 ));
			break;
		}
	}
}

void least_space_used(char* clave) {
	char* instancia = instancia_con_mas_espacio();
	modificar_clave(clave, instancia);
}

char* instancia_con_mas_espacio() {
	t_list* instancias_activas = lista_instancias_activas();
	int cantidad_instancias = list_size(instancias_activas);
	char* instancia = list_get(instancias_activas, 0); //la primera
	instancia_Estado_Conexion* conexion = dictionary_get(lista_Instancias, instancia); //conex de la primera

	for(int j = 1; j < cantidad_instancias; j++) {
		char* otra_instancia = list_get(instancias_activas, j);
		instancia_Estado_Conexion* otra_conexion = dictionary_get(lista_Instancias, otra_instancia);
		if(otra_conexion->entradas_disponibles > conexion->entradas_disponibles) { //si es mejor que la anterior, pasa a ser la seleccionada
			instancia = otra_instancia;
			conexion = otra_conexion;
		}
	}
	return instancia;
}

t_list* lista_instancias_activas() {
	t_list* lista = list_create();
	for(int i = 0 ; i < list_size(listaSoloInstancias) ; i++) {
		char* nombre = list_get(listaSoloInstancias, i);
		if(estadoDeInstancia(nombre) == conectada)
			list_add(lista, nombre);
	}
	return lista;
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



////////////////////////////////////////////////////////////
//_______________________MAGIA____________________________//
//_________________________________________________________
//_________________________________________________________

void* rutina_consulta(void* argumento) {
	int socket_consola = (int)argumento;
	void* stream;
	Accion accion = recibirMensaje(socket_consola, &stream);
	char* instancia = "ERROR";
	if (accion == consulta_simulacion)
		instancia = simular_algoritmo((char*)stream);
	enviarMensaje(socket_consola, consulta_simulacion, instancia, sizeof(instancia));
	return NULL;
}

char* simular_algoritmo(char* clave) {
	switch(configuracion.algoritmo) {
		case el:	return equitative_load_simulado(clave);
		case lsu:	return least_space_used_simulado(clave);
		case ke: 	return key_explicit_simulado(clave);
		default: 	return "ERROR";
	}
}

char* equitative_load_simulado(char* clave) {
	char* instancia = list_get(listaSoloInstancias, contadorEquitativeLoad);
	int estadoInstancia = estadoDeInstancia(instancia);
	return estadoInstancia == conectada ? instancia : equitative_load_simulado(clave);
}

char* key_explicit_simulado(char* clave) {
	t_list* instancias_activas = lista_instancias_activas();
	int cantidad_instancias = list_size(instancias_activas);
	int comienzo = clave[0] - ('a' - 1);
	float asignacion_por_instancia = (float) 26  /  (float)cantidad_instancias;
	float multiplicador = 1;
	for (int j = 1; j <= 26; j++) {
		if(asignacion_por_instancia * multiplicador < comienzo)
			multiplicador++;
		if(comienzo == j)
			return (char*)list_get(instancias_activas, multiplicador - 1);
	}
	return NULL;
}

char* least_space_used_simulado(char* clave) {
	return instancia_con_mas_espacio();
}

int existe_clave(char* clave) {
	return dictionary_has_key(instancias_Claves, clave);
}


