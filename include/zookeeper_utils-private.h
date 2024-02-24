/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */

#include <zookeeper/zookeeper.h>

#ifndef _ZOOKEEPER_UTILS_PRIVATE_H_
#define _ZOOKEEPER_UTILS_PRIVATE_H_

#define ZDATALEN 1024 * 1024

typedef struct String_vector zoo_string;

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);
char *znode_create(zhandle_t *zh, char *root_path, int socket);
char *find_next_server(zhandle_t *zh, char *node, char *root_path, zoo_string *children_list);
char *find_previous_server(zhandle_t *zh, char *node, char *root_path, zoo_string *children_list);
char *find_head_server(zhandle_t *zh, char *root_path, zoo_string *children_list);
char *find_tail_server(zhandle_t *zh, char *root_path, zoo_string *children_list);
char *getIpAddr();
#endif