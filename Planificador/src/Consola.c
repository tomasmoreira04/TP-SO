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

void iniciar_terminal() {
    if(pipe(file_descriptors) == -1)
        abort();

    int pid = fork();

    if (pid == -1)
    	abort();
    if (pid == 0) {
    	close(file_descriptors[1]);
    	char* buffer = malloc(LARGO_RUTA);
    	sprintf(buffer, "%d", file_descriptors[0]);
    	execlp("xterm","xterm", "-e", "cat", buffer, NULL);
    	abort();
    }

    close(file_descriptors[0]);
    outPlanif = fdopen(file_descriptors[1], "w");
}

void* iniciar_consola() {
	//iniciar_terminal();

	size_t tamanio_buffer = 50;
	Operacion operacion;
	char *buffer = malloc(tamanio_buffer);

	while(true) {
		printf("\nIngrese un comando: ");
		getline(&buffer, &tamanio_buffer, stdin);
		operacion = crear_operacion(buffer);
		validar_operacion(operacion);
		//y aca hacer la operacion xd
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
		printf("Operacion no valida.\n");
	else {
		int params = parametros_necesarios(operacion.id);
		if (params == 1) {
			if (operacion.param1 == NULL)
				printf("La operacion %s requiere un parametro.\n", operacion.nombre);
			else
				ejecutar_comando(operacion);
		}
		else if (params == 2) {
			if (operacion.param1 == NULL || operacion.param2 == NULL)
				printf("La operacion %s requiere dos parametros.\n", operacion.nombre);
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
	planificar = 0;
	printf(RED"\n\nSE HA PAUSADO LA PLANIFICACION\n\n"RESET);
}

void continuar_planificacion() {
	planificar = 1;
	printf(GREEN"\n\nSE HA REANUDADO LA PLANIFICACION\n\n"RESET);
}

t_list* buscar_deadlocks() {
	t_list* lista = cola_de_bloqueados;
	int cant = list_size(lista);
	t_list* deadlocks = list_create();

	for (int i = 0; i < cant; i++) {
		ESI* esi = list_get(lista, i);
		for (int j = 0; j < cant; i++) {
			ESI* otro_esi = list_get(lista, j);
			t_deadlock* deadlock = clave_que_necesita(esi, otro_esi);
			if (deadlock != NULL)
				list_add(deadlocks, deadlock);
		}
	}
	return deadlocks;
}

void mostrar_deadlocks() {
	t_list* deadlocks = buscar_deadlocks();
	int num_d = list_size(deadlocks);
	if (num_d == 0)
		printf(GREEN "\nNO SE HA ENCONTRADO DEADLOCK ENTRE LOS ESIS BLOQUEADOS" RESET);
	else {
		printf(RED "\nSE HAN ENCONTRADO DEADLOCKS" RESET);
		for (int i = 0; i < num_d; i++)
			imprimir_deadlock(list_get(deadlocks, i));
	}
}

void imprimir_deadlock(t_deadlock* deadlock) {
	printf(GREEN"\nESI %d" RESET " esperando clave" MAGENTA " %s " RESET " bloqueada por "GREEN "ESI %d"RESET,
			deadlock->esi_bloqueado, deadlock->clave_necesaria, deadlock->esi_bloqueante);
}

t_deadlock* clave_que_necesita(ESI* a, ESI* b) {
	for (int i = 0; i < list_size(a->claves); i++) {
		t_clave* clave = buscar_clave_bloqueada(list_get(a->claves, i));
		for (int j = 0; j < list_size(clave->esis_esperando); j++){
			ESI* esi_esperando = list_get(clave->esis_esperando, j);
			if (esi_esperando->id == b->id) {
				t_deadlock* deadlock = malloc(sizeof(t_deadlock));
				deadlock->esi_bloqueado = b->id;
				deadlock->esi_bloqueante = a->id;
				strcpy(deadlock->clave_necesaria, clave->clave);
				return deadlock;
			}
		}
	}
	return NULL;
}

void desbloquear_clave(char* clave) {
	int ok = liberar_clave(clave);
	if (!ok)
		printf(RED"\nNo se ha podido liberar la clave"CYAN" %s"RESET, clave);
}

void listar_esis_recurso(char* clave) {
	t_clave* c = buscar_clave_bloqueada(clave);
	if (c != NULL) {
		t_list* esis = c->esis_esperando;
		printf("\nLista de procesos esperando la clave %s:\n", clave);
		for (int i = 0; i < list_size(esis); i++)
			printf("ESI %d, ", ((ESI*)list_get(esis, i))->id);
	}
	else printf("\nLa clave %s no esta bloqueada\n", clave);
}

void matar_esi(char* id) {
	int id_esi = atoi(id);
	ESI* esi = obtener_esi(id_esi);
	if (esi != NULL) {
		t_list *claves_liberadas = esi->claves;
		//s_wait(&ejecucion);
		finalizar_esi(esi->id);
		printf(GREEN"\nSe ha finalizado el ESI %d, liberando las claves "RESET, esi->id);
		_imprimir_claves_esi(claves_liberadas);
		//s_signal(&ejecucion);
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
	if (clave_bloqueada != NULL) {
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

ESI* _buscar_esi(t_list* lista, int id) {
	for (int i = 0; i < list_size(lista); i++) {
		ESI* otro = (ESI*)list_get(lista, i);
		if (otro->id == id)
			return otro;
	}
	return NULL;
}

char* consultar_simulacion(char* clave) {
	void* mensaje;
	int coordinador = conexion_con_servidor(config.ip_coordinador, config.puerto_coordinador);
	handShake(coordinador, consola);
	enviarMensaje(coordinador, consulta_simulacion, clave, sizeof(clave));
	recibirMensaje(coordinador, &mensaje);
	return (char*)mensaje;
}
