#include <commons/config.h>
#include <sys/types.h>

#ifndef INSTANCIA_H_
#define INSTANCIA_H_

#define LARGO_IP 16
#define MAX_ALG 5
#define MAX_RUTA 50
#define MAX_NOMBRE 20

typedef struct{
	int entrada;
	int tamanio;
}Reg_TablaEntradas;

void setValor(char* clave, char* valor, int tamEnBytes);
void almacenarNuevo(char* clave, char* valor, int tamEnBytes);
void recibirDimensiones(int socketServer);
void mostrarArray(char* bitarray);
void limpiarArray(int desde, int hasta);
void eliminarEntradas(char* clave);
int buscarEspacioLibre(int entradasNecesarias);
void almacenarValor(char* clave, char* valor);
void ejecutarSentencia(t_sentencia* sentencia);

#endif /* INSTANCIA_H_ */
