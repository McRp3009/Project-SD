/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#include "stats-private.h"
#include "stats.h"
#include <sys/time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

struct statistics_t *stats_create()
{
    struct statistics_t *stats = malloc(sizeof(struct statistics_t));
    if (stats == NULL)
    {
        return NULL;
    }

    stats->amount = 0;
    stats->total_time.tv_sec = 0;
    stats->total_time.tv_usec = 0;
    stats->clients_connected = 0;

    stats->m = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(stats->m, NULL);

    stats->c = malloc(sizeof(pthread_cond_t));
    pthread_cond_init(stats->c, NULL);

    stats->counter_w = 0;
    stats->counter_r = 0;

    return stats;
}

int stats_destroy(struct statistics_t *stats)
{
    if (stats == NULL)
    {
        return -1;
    }
    free(stats->m);
    free(stats->c);
    free(stats);
    return 0;
}

void update_time_status(struct statistics_t *stats, struct timeval time)
{

    // mutex_lock
    stats->total_time.tv_sec += time.tv_sec;
    stats->total_time.tv_usec += time.tv_usec;

    if (stats->total_time.tv_usec >= 1000000)
    {
        stats->total_time.tv_usec += 1;
        stats->total_time.tv_usec -= 1000000;
    }
    // unlock
}

void update_counter_status(struct statistics_t *stats)
{
    stats->amount += 1;
}

void update_clients_status(struct statistics_t *stats, int change)
{
    stats->clients_connected += change;
}