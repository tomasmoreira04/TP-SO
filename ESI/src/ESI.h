#ifndef ESI_H_
#define ESI_H_

#include "commons/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <parsi/parser.h>
#include "../../Bibliotecas/src/Estructuras.h"

void leer_sentencias(int planificador, int coordinador, char* ruta);
char* ruta_script(char* argumento);
void GET_CLAVE(t_esi_operacion operacion);
void SET_CLAVE_VALOR(t_esi_operacion operacion);
void STORE_CLAVE(t_esi_operacion operacion);
void ejecutar_operacion(t_esi_operacion operacion);
FILE* cargar_script(char* ruta);
int cantidad_de_sentencias(FILE* script);
void informar_nuevo_esi(int socket, int rafagas, char* nombre);
void morir(t_esi_operacion operacion, char* linea);

#endif /* ESI_H_ */
