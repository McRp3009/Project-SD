/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#ifndef _STATS_PRIVATE_H
#define _STATS_PRIVATE_H

#include "stats-private.h"
#include <sys/time.h>
#include <pthread.h>

struct statistics_t
{
    int amount;
    struct timeval total_time;
    int clients_connected;
    pthread_mutex_t *m;
    pthread_cond_t *c;
    int counter_w;
    int counter_r;
};

#endif