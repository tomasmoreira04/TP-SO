struct InfoInstancia {
	int id_inst; //etc
};

typedef struct Operacion {
	int id; //del 1 al 8
	char* param1;
	char* param2;
} operacion;

/*operaciones
 * del 1 al 3: 0 parametros
 * del 4 al 7: 1 parametro
 * la 8: 2 parametros clave id
 */

void iniciar_consola();
operacion crear_operacion(char* linea);
void pausar();
void continuar();
void listar_deadlocks();
int bloquear(char* clave, int id);
int desbloquear(char* clave);
int matar(int id);
struct InfoInstancia status(char* id);
int id_operacion(char* nombre_op);
int parametros_necesarios(int id);
