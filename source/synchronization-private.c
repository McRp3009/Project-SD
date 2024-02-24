/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#include "synchronization-private.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void enter_write(pthread_mutex_t *m, pthread_cond_t *c, int *counter_w, int *counter_r)
{
    pthread_mutex_lock(m);
    while ((*counter_w) > 0 || (*counter_r) > 0)
    {
        pthread_cond_wait(c, m);
    }

    (*counter_w)++;
    pthread_mutex_unlock(m);
}

void exit_write(pthread_mutex_t *m, pthread_cond_t *c, int *counter_w)
{

    pthread_mutex_lock(m);
    (*counter_w)--;
    pthread_cond_broadcast(c);
    pthread_mutex_unlock(m);
}

// ########################################################################################## //

void enter_read(pthread_mutex_t *m, pthread_cond_t *c, int *counter_w, int *counter_r)
{
    pthread_mutex_lock(m);
    while ((*counter_w) > 0)
    {
        pthread_cond_wait(c, m);
    }

    (*counter_r)++;
    pthread_mutex_unlock(m);
}

void exit_read(pthread_mutex_t *m, pthread_cond_t *c, int *counter_r)
{
    pthread_mutex_lock(m);
    (*counter_r)--;
    pthread_cond_broadcast(c);
    pthread_mutex_unlock(m);
}
