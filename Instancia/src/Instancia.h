#include <commons/config.h>
#include <sys/types.h>

#ifndef INSTANCIA_H_
#define INSTANCIA_H_

#define LARGO_IP 16
#define MAX_ALG 5
#define MAX_RUTA 50
#define MAX_NOMBRE 20

typedef struct{
	char* clave;
	int entrada;
	int tamanio;		//en Bytes
} Reg_TablaEntradas;

typedef struct{
	char* clave;
	int tamanio;
	int ultimaRef;
} Nodo_Reemplazo;

typedef enum {
	hilo_dump,
	hilo_compactar,
	hilo_sentencia
} HiloInstancia;

typedef struct{
	char* clave;
	char* valor;
} t_clave_inicial;

void setValor(char* clave, char* valor, int tamEnBytes);
void almacenarNuevo(char* clave, char* valor, int tamEnBytes);
void recibirDimensiones(int socketServer);
void mostrarArray(char* bitarray);
void limpiarArray(int desde, int hasta);
void liberarEntradas(char* clave);
int buscarEspacioLibre(int entradasNecesarias);
void almacenarValor(char* clave, char* valor);
void ejecutarSentencia(t_sentencia* sentencia);
char* devolverValor(char* clave);
void mostrarValor(char* clave);
void persistirValor(char* clave);
t_list* reemplazoSegunAlgoritmo(int cantNecesita);
void reemplazarValor(char* clave, char* valor, int tamEnEntradas);
void mostrarListaReemplazos(t_list* list);
void destruirlo_todo();
void nodoRempDestroyer(Nodo_Reemplazo* nodo);
bool comparadorMayorTam(Nodo_Reemplazo* nodo1, Nodo_Reemplazo* nodo2);
void eliminarDeListaRemp(t_list* listaEliminar);
void regTablaDestroyer(Reg_TablaEntradas* registro);
void aumentarTiempoRef();
int buscarNodoReemplazo(char* clave);

//Lucy funciones
void crear_hilo();
int laListaLoContiene(char * clave);
void *rutina_Dump(void * arg);
void guardarLaWea();

//asd
void rutina_principal();


//sida
t_list* lista_entradas();
Reg_TablaEntradas* buscar_entrada_en_lista(t_list* lista, int entrada);
void compact();
void imprimir_almacenamiento();
void imprimir_espacio(int bit);
void cargar_clave_montaje(char* archivo, char* clave);
void configurar_entradas();
void inicializar_estructuras();
int recuperar_claves(char* ruta);
void cargar_claves_iniciales();

#endif /* INSTANCIA_H_ */
