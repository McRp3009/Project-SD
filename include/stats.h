#ifndef _STATS_H
/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#define _STATS_H
#include <sys/time.h>
#include "stats-private.h"

struct statistics_t;

struct statistics_t *stats_create();
int stats_destroy(struct statistics_t *stats);
void update_time_status(struct statistics_t *stats, struct timeval time);
void update_counter_status(struct statistics_t *stats);
void update_clients_status(struct statistics_t *stats, int change);

#endif