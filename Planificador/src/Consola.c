#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commons/string.h"
#include "Consola.h"

typedef struct Operacion {
	int id; //del 1 al 8
	char* nombre;
	char* param1;
	char* param2;
} Operacion;

/*operaciones
 * del 1 al 3: 0 parametros
 * del 4 al 7: 1 parametro
 * la 8: 2 parametros clave id
 */

void* iniciar_consola() {
	size_t tamanio_buffer = 50;
	Operacion operacion;
	char *buffer = crear_buffer_input(tamanio_buffer);

	while(1) {
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

char* crear_buffer_input(size_t tamanio) {
	char* buffer = malloc(tamanio * sizeof(char));
		if (buffer == NULL) {
			perror("No hay memoria suficiente para correr la consola.\n");
			exit(0);
		}
	printf("Consola iniciada, ingrese una operacion debajo:\n");
	return buffer;
}

void validar_operacion(Operacion operacion) {
	if (operacion.id == 0)
		printf("Operacion no valida.\n");
	else {
		int params = parametros_necesarios(operacion.id);
		if (params == 1) {
			if (operacion.param1 == NULL)
				printf("La operacion %s requiere un parametro.\n", operacion.nombre);
			else
				printf("Operacion %d (%s) con parametro %s", operacion.id, operacion.nombre, operacion.param1);
		}
		else if (params == 2) {
			if (operacion.param1 == NULL || operacion.param2 == NULL)
				printf("La operacion %s requiere dos parametros.\n", operacion.nombre);
			else
				printf("Operacion %d (%s) con parametros %s, %s", operacion.id, operacion.nombre, operacion.param1, operacion.param2);
		}
		else
			printf("Operacion %d (%s)", operacion.id, operacion.nombre);
	}
}
