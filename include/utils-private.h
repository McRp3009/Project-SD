/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#include "data.h"
#include "entry.h"
#include "client_stub-private.h"
#include "network_client.h"
#include <sys/time.h>
#ifndef _NETWORK_UTILS_PRIVATE_H
#define _NETWORK_UTILS_PRIVATE_H

int write_all(int sock, char *buf, int len);
int read_all(int sock, char *buf, int len);
void sigpipe_handler(int sign);
struct timeval get_time_diff(struct timeval time_end, struct timeval time_start);
#endif
