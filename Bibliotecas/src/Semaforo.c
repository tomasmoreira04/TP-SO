#ifndef H_SEMAFORO
#define H_SEMAFORO

#include <stdio.h>
#include <stdlib.h>

void s_wait(int* semaforo) {
	while (*semaforo != 1);
	*semaforo = 0;
}

void s_signal(int* semaforo) {
	*semaforo = 1;
}

#endif
