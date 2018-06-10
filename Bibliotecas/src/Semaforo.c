#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void s_wait(pthread_mutex_t* mutex) {
	pthread_mutex_lock(mutex);
}

void s_signal(pthread_mutex_t* mutex) {
	pthread_mutex_unlock(mutex);
}
