#include "wrappers.h"

void unix_error(int code, char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    _exit(0);
}

int Sem_init(sem_t *sem, int pshared, unsigned int value)
{
    int rc;
    if((rc = sem_init(sem, pshared, value)) != 0)
        unix_error(rc, "Error calling sem_init()");
    return rc;
}

int Sem_destroy(sem_t *sem)
{
    int rc;
    if((rc = sem_destroy(sem)) != 0)
        unix_error(rc, "Error calling sem_destroy()");
    return rc;
}

int W(sem_t *sem)
{
    int rc;
    if((rc = sem_wait(sem)) != 0)
        unix_error(rc, "Error calling sem_wait()");
    return rc;
}

int V(sem_t *sem)
{
    int rc;
    if((rc = sem_post(sem)) != 0)
        unix_error(rc, "Error calling sem_post()");
    return rc;
}

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, void * (*routine)(void *), void *argp)
{
    int rc;
    if((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
        unix_error(rc, "Error calling pthread_create()");
}

void Pthread_join(pthread_t tidp, void **value_ptr)
{
    int rc;
    if((rc = pthread_join(tidp, value_ptr)) != 0)
        unix_error(rc, "Error calling pthread_join()");
}
