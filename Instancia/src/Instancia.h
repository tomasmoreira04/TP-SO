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
void liberarEntradas(char* clave, int borrar_nodo);
int buscarEspacioLibre(int entradasNecesarias);
void almacenarValor(char* clave, char* valor);
void ejecutarSentencia(t_sentencia* sentencia);
char* devolver_valor(char* clave);
void mostrarValor(char* clave);
int persistirValor(char* clave);
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

bool comparadorMayorTam(Nodo_Reemplazo* nodo1, Nodo_Reemplazo* nodo2);
bool comparadorMayorTiempo(Nodo_Reemplazo* nodo1, Nodo_Reemplazo* nodo2);
int es_atomico(char* valor);
//Lucy funciones
void crear_hilo();
int laListaLoContiene(char * clave);
void *rutina_Dump(void * arg);
void guardarLaWea();
void mostrar_storage();
void terminar_programa(int);
char* string_mostrar_storage();
void limpiarEntradas(int entrada, int numero_entradas);
void limpiarStorage(int desde, int hasta);

//asd
void rutina_principal();

Reg_TablaEntradas* buscar_entrada(char* clave);


//sida
t_list* lista_entradas();
Reg_TablaEntradas* buscar_entrada_en_lista(t_list* lista, int entrada);
void compact();
void imprimir_almacenamiento();
void imprimir_espacio(int bit);
void cargar_clave_montaje(char* archivo, char* clave);

void inicializar_estructuras();
int recuperar_claves(char* ruta);
void cargar_claves_iniciales();
char* algoritmo(AlgoritmoInst alg);
void imprimir_configuracion();

void configurar_entradas();
void procesar_input(char* input);
void mensaje_inicial();

#endif /* INSTANCIA_H_ */
