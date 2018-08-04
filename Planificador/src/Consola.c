#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commons/string.h"
#include "Consola.h"
#include "Planificador.h"
#include "../../Bibliotecas/src/Estructuras.h"
#include "../../Bibliotecas/src/Macros.h"
#include "../../Bibliotecas/src/Color.h"
#include <stddef.h>
#include <limits.h>
#include <unistd.h>

int file_descriptors[2];
FILE* outPlanif;
char* buffer;
int unlock_hecho = 0;
int pausado = 0;

void* iniciar_consola() {
	size_t tamanio_buffer = 50;
	Operacion operacion;

	while(true) {
		buffer = malloc(tamanio_buffer);
		printf(GREEN"\nIngrese un comando "RESET"\n>> ");
		getline(&buffer, &tamanio_buffer, stdin);
		operacion = crear_operacion(buffer);
		free(buffer);
		validar_operacion(operacion);
	}
}

Operacion crear_operacion(char* linea) { //separo en tokens lo ingresado
	Operacion op;
	char *token = strtok(linea, " \n\0\t");
	op.id = id_operacion(token);
	op.nombre = token;
	op.param1 = NULL;
	op.param2 = NULL;
	if (parametros_necesarios(op.id) > 0)
		op.param1 = strtok(NULL, " \n\0\t");
	if (parametros_necesarios(op.id) > 1)
		op.param2 = strtok(NULL, " \n\0\t");
	return op;
}

Comando id_operacion(char* nombre) {
	if (strcmp(nombre, "pause") == 0) return _pause;
	if (strcmp(nombre, "continue") == 0) return _continue;
	if (strcmp(nombre, "deadlock") == 0) return deadlock;
	if (strcmp(nombre, "unlock") == 0) return unlock;
	if (strcmp(nombre, "list") == 0) return list;
	if (strcmp(nombre, "kill") == 0) return _kill;
	if (strcmp(nombre, "status") == 0) return status;
	if (strcmp(nombre, "block") == 0) return block;
	return -1;
}

int parametros_necesarios(Comando comando) {
	switch(comando){
	case _pause: case _continue: case deadlock:
		return 0;
	case unlock: case list:	case _kill:	case status:
		return 1;
	case block:
		return 2;
	default:
		return -1;
	}
}

void validar_operacion(Operacion operacion) {
	if (operacion.id == -1)
		printf(RED"Operacion no valida.\n"RESET);
	else {
		int params = parametros_necesarios(operacion.id);
		if (params == 1) {
			if (operacion.param1 == NULL)
				printf(YELLOW"La operacion"RED" %s "YELLOW"requiere un parametro.\n"RESET, operacion.nombre);
			else
				ejecutar_comando(operacion);
		}
		else if (params == 2) {
			if (operacion.param1 == NULL || operacion.param2 == NULL)
				printf(YELLOW"La operacion"RED" %s "YELLOW"requiere dos parametros.\n"RESET, operacion.nombre);
			else
				ejecutar_comando(operacion);
		}
		else
			ejecutar_comando(operacion);
	}
}

void ejecutar_comando(Operacion comando) {
	switch(comando.id) {
	case _pause:
		pausar_planificacion();
		break;
	case _continue:
		continuar_planificacion();
		break;
	case deadlock:
		mostrar_deadlocks();
		break;
	case unlock:
		desbloquear_clave(comando.param1);
		break;
	case list:
		listar_esis_recurso(comando.param1);
		break;
	case _kill:
		matar_esi(comando.param1);
		break;
	case status:
		estado_clave(comando.param1);
		break;
	case block:
		bloquear_esi_en_clave(comando.param1, comando.param2);
		break;
	}
}

void pausar_planificacion() {
	printf(RED"\n\nSE HA PAUSADO LA PLANIFICACION\n\n"RESET);
	pthread_mutex_lock(&mutex_planificar);
	pausado = 1;
}

void continuar_planificacion() {
	printf(GREEN"\n\nSE HA REANUDADO LA PLANIFICACION\n\n"RESET);
	pthread_mutex_unlock(&mutex_planificar);
	pausado = 0;
	/*if (unlock_hecho) {
		unlock_hecho = 0;
		replanificar();
	}*/
}

t_clave* buscar_clave_que_necesita(ESI* esi) {
	t_list* lista = lista_claves_bloqueadas;
	int n = list_size(lista_claves_bloqueadas);
	for (int i = 0; i < n; i++) {
		t_clave* clave = list_get(lista, i);
		for (int j = 0; j < list_size(clave->esis_esperando); j++) {
			ESI* esperando = list_get(clave->esis_esperando, j);
			if (esperando->id == esi->id)
				return clave;
		}
	}
	return NULL;
}

int alguno_bloquea_clave(t_clave* clave) { //y ademas bloqueado
	if (clave->bloqueada && clave->esi_duenio != NULL)
		return clave->esi_duenio->cola_actual == cola_de_bloqueados;
	return 0;
}

char* nombre_esi(int indice) {
	ESI* esi = obtener_esi(indice);
	return esi->nombre;
}

void imprimir_deadlock(t_deadlock* d) {
	char* n1 = nombre_esi(d->bloqueo1->esi_bloqueado);
	char* n2 = nombre_esi(d->bloqueo2->esi_bloqueado);
	printf(RED"\n%s"YELLOW" bloquea a "GREEN"%s"YELLOW" por la clave "CYAN"%s"RESET, n1, n2, d->bloqueo1->clave);
	printf(RED"\n%s"YELLOW" bloquea a "GREEN"%s"YELLOW" por la clave "CYAN"%s\n"RESET, n2, n1, d->bloqueo2->clave);
}

void imprimir_deadlocks(t_list* deadlocks) {
	int cantidad = list_size(deadlocks);
	if (cantidad == 0)
		printf(GREEN"\nNo hay deadlocks"RESET);
	else {
		printf(RED"\nDeadlocks detectados:"RESET);
		for (int i = 0; i < cantidad; i ++)
			imprimir_deadlock(list_get(deadlocks, i));
	}
}

int tiene_clave(ESI* esi, char* clave) {
	t_list* claves = claves_de_esi(esi);
	for (int i = 0; i < list_size(claves); i++) {
		t_clave* clave2  = list_get(claves, i);
		if (strcmp(clave, clave2->clave) == 0) {
			list_clean_and_destroy_elements(claves, free);
			free(claves);
			return 1;
		}
	}
	list_clean_and_destroy_elements(claves, free);
	free(claves);
	return 0;
}

void nuevo_bloqueo(t_list* bloqueos, ESI* esi_bloqueado, ESI* esi_bloqueante, char* clave) {
	t_bloqueo* nuevo_bloqueo = malloc(sizeof(t_bloqueo));
	nuevo_bloqueo->esi_bloqueado = esi_bloqueado->id;
	nuevo_bloqueo->esi_bloqueante = esi_bloqueante->id;
	strcpy(nuevo_bloqueo->clave, clave);
	list_add(bloqueos, nuevo_bloqueo);
}

t_list* armar_bloqueos() {
	t_list* bloqueados = cola_de_bloqueados;
	int cantidad_bloqueados = list_size(bloqueados);
	t_list* bloqueos = list_create();
	for (int i = 0; i < cantidad_bloqueados; i++) {
		ESI* esi1 = list_get(bloqueados, i);
		t_clave* clave1 = buscar_clave_que_necesita(esi1);
		if (clave1 != NULL && alguno_bloquea_clave(clave1)) {
			ESI* esi2 = clave1->esi_duenio;
			t_clave* clave2 = buscar_clave_que_necesita(esi2);
			if (clave2 != NULL && alguno_bloquea_clave(clave2) && esi1->id == clave2->esi_duenio->id)
				nuevo_bloqueo(bloqueos, esi1, esi2, clave1->clave);
		}
	}
	return bloqueos;
}

t_list* armar_deadlocks(t_list* bloqueos) {
	t_list* deadlocks = list_create();
	int n = list_size(bloqueos);
	for (int i = 0; i < n; i++) {
		t_bloqueo* bloqueo1 = list_get(bloqueos, i);
		for (int j = i + 1; j < n; j++) {
			t_bloqueo* bloqueo2 = list_get(bloqueos, j);
			if (bloqueo1->esi_bloqueado == bloqueo2->esi_bloqueante
					&& bloqueo2->esi_bloqueado == bloqueo1->esi_bloqueante) {
				t_deadlock* d = malloc(sizeof(deadlock));
				d->bloqueo1 = bloqueo1;
				d->bloqueo2 = bloqueo2;
				list_add(deadlocks, d);
			}
		}
	}
	return deadlocks;
}

void mostrar_deadlocks() {
	t_list* bloqueos = armar_bloqueos();
	t_list* deadlocks = armar_deadlocks(bloqueos);
	imprimir_deadlocks(deadlocks);
	destruir_deadlocks(bloqueos, deadlocks);
}

void destruir_deadlocks(t_list* bloqueos, t_list* deadlocks) {

	void destructor(void* elemento) {
		free(elemento);
	}
	list_destroy_and_destroy_elements(bloqueos, (void*)destructor);
	list_destroy_and_destroy_elements(deadlocks, (void*)destructor);
}

void desbloquear_clave(char* clave) {
	int esi = liberar_clave_consola(clave);
	if (esi == -1)
		printf(RED"\nNo se ha podido liberar la clave"CYAN" %s"RESET, clave);
	unlock_hecho = 1;
	aviso_get_clave(clave, esi);
}

void listar_esis_recurso(char* clave) {
	t_clave* c = buscar_clave_bloqueada(clave);
	if (c != NULL) {
		t_list* esis = c->esis_esperando;
		printf(CYAN"\nLista de procesos esperando la clave"RED" %s:\n"RESET, clave);
		for (int i = 0; i < list_size(esis); i++)
			printf("ESI "GREEN"%d"RESET",", ((ESI*)list_get(esis, i))->id);
	}
	else printf(RED"\nLa clave "GREEN"%s"RED" no esta bloqueada\n"RESET, clave);
}

void matar_esi(char* id) {
	int id_esi = atoi(id);
	ESI* esi = obtener_esi(id_esi);
	if (esi != NULL) {
		ESI* esi = obtener_esi(id_esi);
		finalizar_esi_ref(esi);
		avisar(esi->socket_planif, terminar_esi);
		printf(RED"\n\nSe ha finalizado el ESI %d"RESET, id_esi);
		sem_post(&contador_esis_disponibles);
	}
	else {
		printf(RED"\nNo existe el ESI %d en ninguna cola ni ejecutando\n"GREEN, id_esi);
	}
}



void estado_clave(char* clave){
	//listar estado y valor
	//instancia en la que se encuentra
	//instancia en la que se guardaria
	//esis esperando liberacion
	t_clave* clave_bloqueada = buscar_clave_bloqueada(clave);
	if (clave_bloqueada != NULL && clave_bloqueada->bloqueada == 1) {
		printf(YELLOW"\nLa clave"RED" %s "YELLOW"se encuentra bloqueada", clave);
		//---------------VERIFICAR VALOR-----------------
		if (clave_bloqueada->valor[0] != '\0')
			printf(", con valor"GREEN" %s."RESET, clave_bloqueada->valor);
		else
			printf(", pero sin valor asignado."RESET);
		//---------------VERIFICAR ESI DUEÑO-----------------
		if (clave_bloqueada->esi_duenio != NULL)
			printf("\nLa clave pertenece al ESI"GREEN" %d.", clave_bloqueada->esi_duenio->id);
		else
			printf("\nLa clave no pertenece a ningun ESI, es una clave bloqueada por configuracion.");
		//---------------VERIFICAR ESIS ESPERANDO-----------------
		int esis_esperando = list_size(clave_bloqueada->esis_esperando);
		if (esis_esperando == 0)
			printf("\nNingun ESI esta esperando la liberacion de esta clave."RESET);
		else {
			printf("\nLos siguientes ESIS estan bloqueados por esta clave:"RESET);
			for (int i = 0; i < esis_esperando; i++) {
				ESI* esi = list_get(clave_bloqueada->esis_esperando, i);
				printf(CYAN"\nESI %d"RESET, esi->id);
			}
		}
		if (strcmp(clave_bloqueada->instancia, "No asignada") != 0)
			printf(CYAN"\nEsta clave se encuentra guardada en "RED"%s."RESET, clave_bloqueada->instancia);
		else {
			char* instancia_posible = consultar_simulacion(clave);
			printf(GREEN"\nLa clave se guardaria en la instancia"RED" %s."RESET, instancia_posible);
		}
	}
	else {
		printf(GREEN"\nLa clave"RED" %s "GREEN"no se encuentra bloqueada y no tiene valor asignado.", clave);
		char* instancia_posible = consultar_simulacion(clave);
		printf(GREEN"\nLa clave se guardaria en la instancia"RED" %s."RESET, instancia_posible);
	}
}

void bloquear_esi_en_clave(char* clave, char* id_esi) {
	int id = atoi(id_esi);
	ESI* esi = obtener_esi(id);
	if (esi != NULL) {
		nueva_solicitud_clave(clave, esi);
		bloquear_esi(esi);
		printf(YELLOW"\nESI %d bloqueado por clave"RED" %s"RESET, id, clave);
	}
	else printf(RED"\nNo existe el ESI"CYAN" %d"RESET, id);

}

void _imprimir_claves_esi(t_list* claves) {
	for (int i = 0; i < list_size(claves); i++)
		printf(RED"%s, "RESET, (char*)list_get(claves, i));
}

char* consultar_simulacion(char* clave) {
	void* mensaje;
	int coordinador = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador);
	handShake(coordinador, consola);
	enviarMensaje(coordinador, consulta_simulacion, clave, strlen(clave) + 1);
	recibirMensaje(coordinador, &mensaje);
	return (char*)mensaje;
}

void aviso_get_clave(char* clave, int esi) {

	char* otro = strdup(clave);

	int coordinador = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador);
	int largo = strlen(otro) + 1;
	int tam = sizeof(int) * 2 + largo;

	handShake(coordinador, consola);

	void* asd = malloc(tam);

	memcpy(asd, &esi, sizeof(int));
	memcpy(asd + sizeof(int), &largo, sizeof(int));
	memcpy(asd + sizeof(int) * 2, otro, largo);
	free(otro);
	enviarMensaje(coordinador, aviso_bloqueo_clave, asd, tam);

}
