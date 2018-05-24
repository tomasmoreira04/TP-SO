#include <commons/config.h>
#include <sys/types.h>

#ifndef INSTANCIA_H_
#define INSTANCIA_H_

#define LARGO_IP 16
#define MAX_ALG 5
#define MAX_RUTA 50
#define MAX_NOMBRE 20

typedef struct{
	int32_t cant_entradas;
	int32_t tam_entradas;
} Dimensiones_Inst;

typedef struct{
	char* entrada;
	int tamanio;
}Reg_TablaEntradas;

void setValor(char* clave, char* valor, int tamEnBytes);
void almacenarNuevo(char* clave, char* valor, int tamEnBytes);

#endif /* INSTANCIA_H_ */
