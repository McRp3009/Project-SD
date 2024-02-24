/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#ifndef _SYNCHRONIZATION_PRIVATE_H
#define _SYNCHRONIZATION_PRIVATE_H

#include <pthread.h>

void enter_write(pthread_mutex_t *m, pthread_cond_t *c, int *counter_w, int *counter_r);

void enter_read(pthread_mutex_t *m, pthread_cond_t *c, int *counter_w, int *counter_r);

void exit_write(pthread_mutex_t *m, pthread_cond_t *c, int *counter_w);

void exit_read(pthread_mutex_t *m, pthread_cond_t *c, int *counter_r);

#endif