#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commons/string.h"
#include "Consola.h"

void iniciar_consola() {
	size_t size = 50;
	operacion op;
	char *buffer = malloc(size * sizeof(char));
	if (buffer == NULL) {
		perror("no te dieron memoria salu2\n");
		exit(0);
	}
	else printf("Consola iniciada, ingrese una operacion debajo:\n");
	while(1) {
		getline(&buffer, &size, stdin);
		op = crear_operacion(buffer);
		if (!op.id)
			printf("Operacion no valida.\n");
		else
		printf("Operacion %d, con parametros %s %s\n", op.id, op.param1, op.param2);
		//y aca hacer la operacion xd
	}
}

operacion crear_operacion(char* linea) { //separo en tokens lo ingresado
	operacion op;
	char *token = strtok(linea, " \n\0\t");
	op.id = id_operacion(token);
	op.param1 = NULL;
	op.param2 = NULL;
	if (parametros_necesarios(op.id) > 0)
		op.param1 = strtok(NULL, " \n\0\t");
	if (parametros_necesarios(op.id) > 1)
		op.param2 = strtok(NULL, " \n\0\t");
	return op;

}

void pausar() {
	// Implementar
}

void continuar() {
	// Implementar
}

int bloquear(char* clave, int id) {
	// Implementar
	return 0;
}

int desbloquear(char* clave) {
	// Implementar
	return 0;
}

int matar(int id) { //no me deja kill
	// Implementar
	return 0;
}

struct InfoInstancia status(char* id){
	struct InfoInstancia ret;
	ret.id_inst = 1;
	return ret;
}


void listar_deadlocks() {
	//
}

int id_operacion(char* nombre_op) {
	if (strcmp(nombre_op, "pause") == 0)
		return 1;
	if (strcmp(nombre_op, "continue") == 0)
		return 2;
	if (strcmp(nombre_op, "deadlock") == 0)
		return 3;
	if (strcmp(nombre_op, "status") == 0)
		return 4;
	if (strcmp(nombre_op, "kill") == 0)
		return 5;
	if (strcmp(nombre_op, "list") == 0)
		return 6;
	if (strcmp(nombre_op, "unlock") == 0)
		return 7;
	if (strcmp(nombre_op, "block") == 0)
		return 8;
	return 0; //error no es operacion
}

int parametros_necesarios(int id) {
	if (id >= 1 && id <= 3)
		return 0;
	if (id >= 4 && id <= 7)
		return 1;
	return 2;
}
