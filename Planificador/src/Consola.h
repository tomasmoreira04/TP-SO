typedef struct Operacion Operacion;

void* iniciar_consola();
Operacion crear_operacion(char* linea);
int id_operacion(char* nombre_op);
int parametros_necesarios(int id);
void validar_operacion(Operacion operacion);
char* crear_buffer_input(size_t tamanio);
/*void pausar();
void continuar();
void listar_deadlocks();
int bloquear(char* clave, int id);
int desbloquear(char* clave);
int matar(int id);
struct InfoInstancia status(char* id);*/
