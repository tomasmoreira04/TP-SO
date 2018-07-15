#ifndef SOCKET_H
#define SOCKET_H

#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXDATASIZE 100
#define BACKLOG 10

typedef enum { planificador, coordinador, esi, instancia, consola } Modulo;

fd_set master;
fd_set read_fds;
struct sockaddr_in remoteaddr;
socklen_t addrlen;
int fdmax;

typedef struct{
	int accion;
	int tamano;
} header;

int recibirMensaje(int socket,void** stream);
void enviarMensaje(int socket,int accion,void* contenido,int tamano);
int crear_socket_de_escucha(int puerto);
int aceptar_nueva_conexion(int listener);
void *get_in_addr(struct sockaddr *sa);
void *getSin_Addr(struct sockaddr *sa);
int conexion_con_servidor(char* ip, int port);
void handShake(int socket,int modulo);


#endif
