#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

int Sem_init(sem_t *sem, int pshared, unsigned int value);
int Sem_destroy(sem_t *sem);
int W(sem_t *sem);
int V(sem_t *sem);

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, void *(*routine) (void *), void *argp);
void Pthread_join(pthread_t tidp, void **value_ptr);
