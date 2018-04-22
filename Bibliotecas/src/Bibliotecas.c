/*
 ============================================================================
 Name        : Bibliotecas.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


enum tipos{
	entero = 1,
	cadena = 2,
	boolean = 3
};

void * serializar(void* puntero, int cantidadDeVariables, ...){
	va_list valist;
	void * stream = malloc(0);
	int i, tipo, avanceStream = 0, avancePuntero = 0, size = 0;
	va_start(valist, cantidadDeVariables);

	for (i = 0; i < cantidadDeVariables; i++) {
		tipo = va_arg(valist, int);

		switch(tipo){
			case entero:{
				size+= sizeof(int);
				stream = realloc(stream, size);
				memcpy(stream+avanceStream, puntero + avancePuntero, sizeof(int));
				avanceStream+= sizeof(int);
				avancePuntero+= sizeof(int);
			}break;

			case cadena:{
				int tamanoCadena = strlen(puntero + avancePuntero) + 1;
				size+= sizeof(int) + tamanoCadena;
				stream = realloc(stream, size);

				memcpy(stream+avanceStream, &tamanoCadena, sizeof(int));
				avanceStream+= sizeof(int);

				int direccionDeMemoria;
				memcpy(&direccionDeMemoria, puntero + avancePuntero, sizeof(int));

				char * cadena = direccionDeMemoria;

				memcpy(stream+avanceStream, cadena, tamanoCadena);
				avanceStream+= tamanoCadena;
				avancePuntero+= sizeof(int);
			}break;
		}

	}
	va_end(valist);
	return stream;
}


void * deserializar(void* stream, int cantidadDeVariables, ...){
	va_list valist;
	void * original = malloc(0);
	int i, tipo, avanceStream = 0, avanceOriginal = 0, size = 0;
	va_start(valist, cantidadDeVariables);

	for (i = 0; i < cantidadDeVariables; i++) {
		tipo = va_arg(valist, int);

		switch(tipo){
			case entero:{

				size+= sizeof(int);
				original = realloc(original, size);
				memcpy(original+avanceOriginal, stream + avanceStream, sizeof(int));
				avanceOriginal+= sizeof(int);
				avanceStream+= sizeof(int);
			}break;

			case cadena:{



				int tamanoCadena;
				memcpy(&tamanoCadena, stream + avanceStream, sizeof(int));
				avanceStream+= sizeof(int);

				char* cadena = malloc(tamanoCadena);
				memcpy(cadena, stream + avanceStream, tamanoCadena);
				avanceStream += tamanoCadena;


				size+=sizeof(int);
				original = realloc(original,size);

				int direccion = cadena;
				memcpy(original + avanceOriginal, &direccion, sizeof(int));
				avanceOriginal+= sizeof(int);

			}break;

		}
	}
	va_end(valist);
	return original;
}

int main(void) {
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}

