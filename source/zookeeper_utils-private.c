/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */

#include "data.h"
#include "entry.h"
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "client_stub.h"
#include "utils-private.h"
#include "network_client.h"
#include "sdmessage.pb-c.h"
#include "client_stub-private.h"
#include <zookeeper/zookeeper.h>
#include "zookeeper_utils-private.h"

/*
 * Funcao que cria os filhos efemeros de root_path
 */
char *znode_create(zhandle_t *zh, char *root_path, int socket)
{
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(struct sockaddr);
    if (getsockname(socket, (struct sockaddr *)&addr, &addrLen))
    {
        perror("getsockname");
        return NULL;
    }

    // criar ip_port
    short port = ntohs(addr.sin_port);

    // Espaço necessário para um short mais o Null terminator
    char *str_port = malloc(6);
    memset(str_port, -1, 6); // Settar tudo a -1
    sprintf(str_port, "%hu", port);
    for (int i = 0; i < 6; i++)
    {
        if (str_port[i] == -1)
        {
            str_port[i] = '\0'; // Settamos o primeiro -1 a \0 para simbolizar o fim da string para depois podermos usar o strlen
        }
    }
    char *aux = getIpAddr();
    int len = strlen(aux);
    char ip_port[len + strlen(str_port) + 2]; // Length do ip + length do port + 1 para o \0
    snprintf(ip_port, sizeof(ip_port), "%s:%s", aux, str_port);
    ip_port[len + strlen(str_port) + 1] = '\0';

    // criar nodes
    char node_path[120] = "";
    strcat(node_path, root_path);
    strcat(node_path, "/node");
    int new_path_len = 1024;
    char *new_path = malloc(new_path_len);

    if (ZOK != zoo_create(zh, node_path, ip_port, sizeof(ip_port), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, new_path, new_path_len))
    {
        fprintf(stderr, "Error creating znode from path %s!\n", node_path);
        return NULL;
    }
    fprintf(stderr, "Ephemeral Sequencial ZNode created! ZNode path: %s\n", new_path);
    free(aux);
    free(str_port);
    return new_path;
}

char *find_next_server(zhandle_t *zh, char *node, char *root_path, zoo_string *children_list)
{

    char *next = NULL;
    for (int i = 0; i < children_list->count; i++)
    {
        int full_path_len = strlen(root_path) + strlen(children_list->data[i]) + 2;
        char *full_path = malloc(full_path_len);
        if (full_path == NULL)
        {
            return NULL;
        }
        snprintf(full_path, full_path_len, "%s/%s", root_path, children_list->data[i]);
        if (strcmp(full_path, node) <= 0)
        {
            free(full_path);
            continue;
        }

        if (next == NULL)
        {
            next = full_path;
            continue;
        }

        if (strcmp(full_path, next) <= 0)
        {
            free(next);
            next = full_path;
            continue;
        }

        free(full_path);
    }

    // verificar se há next
    if (next == NULL) // caso em que não há next
    {
        return NULL;
    }

    // encontrámos um next - obter metadados
    int metadados_len = ZDATALEN * sizeof(char);
    char *metadados = malloc(metadados_len);

    int ret = zoo_wget(zh, next, NULL, NULL, metadados, &metadados_len, NULL);
    if (ret == ZOK)
    {
        free(next);
        return metadados;
    }

    free(metadados);
    free(next);
    return NULL;
}

char *find_previous_server(zhandle_t *zh, char *node, char *root_path, zoo_string *children_list)
{
    char *previous = NULL;
    for (int i = 0; i < children_list->count; i++)
    {
        int full_path_len = strlen(root_path) + strlen(children_list->data[i]) + 2;
        char *full_path = malloc(full_path_len);
        if (full_path == NULL)
        {
            return NULL;
        }
        snprintf(full_path, full_path_len, "%s/%s", root_path, children_list->data[i]);

        if (strcmp(full_path, node) >= 0)
        {
            free(full_path);
            continue;
        }

        if (previous == NULL)
        {
            previous = full_path;
            continue;
        }

        if (strcmp(full_path, previous) >= 0)
        {
            // Substitui o guardado
            free(previous);
            previous = full_path;
            continue;
        }

        free(full_path);
    }

    // verificar se há previous
    if (previous == NULL) // caso em que não há prev
    {
        free(previous);
        return NULL;
    }

    // encontrámos um prev - obter metadados
    int metadados_len = ZDATALEN * sizeof(char);
    char *metadados = malloc(metadados_len);

    int ret = zoo_wget(zh, previous, NULL, NULL, metadados, &metadados_len, NULL);

    if (ret == ZOK)
    {
        free(previous);
        return metadados;
    }

    free(metadados);
    free(previous);
    return NULL;
}

char *find_head_server(zhandle_t *zh, char *root_path, zoo_string *children_list)
{
    char *head = NULL;
    for (int i = 0; i < children_list->count; i++)
    {
        int full_path_len = strlen(root_path) + strlen(children_list->data[i]) + 2;
        char *full_path = malloc(full_path_len);
        if (full_path == NULL)
        {
            return NULL;
        }
        snprintf(full_path, full_path_len, "%s/%s", root_path, children_list->data[i]);

        if (head == NULL)
        {
            head = full_path;
            continue;
        }

        if (strcmp(full_path, head) < 0)
        {
            free(head);
            head = full_path;
            continue;
        }
        free(full_path);
    }

    // verificar se há head
    if (head == NULL) // caso em que não há head
    {
        return NULL;
    }

    // encontrámos um head - obter metadados
    int metadados_len = ZDATALEN * sizeof(char);
    char *metadados = malloc(metadados_len);

    int ret = zoo_wget(zh, head, NULL, NULL, metadados, &metadados_len, NULL);
    if (ret == ZOK)
    {
        free(head);
        return metadados;
    }

    free(metadados);
    free(head);
    return NULL;
}

char *find_tail_server(zhandle_t *zh, char *root_path, zoo_string *children_list)
{
    char *tail = NULL;
    for (int i = 0; i < children_list->count; i++)
    {
        int full_path_len = strlen(root_path) + strlen(children_list->data[i]) + 2;
        char *full_path = malloc(full_path_len);
        if (full_path == NULL)
        {
            return NULL;
        }
        snprintf(full_path, full_path_len, "%s/%s", root_path, children_list->data[i]);

        if (tail == NULL)
        {
            tail = full_path;
            continue;
        }

        if (strcmp(full_path, tail) > 0) // encontrei o next
        {
            free(tail);
            tail = full_path;
            continue;
        }
        free(full_path);
    }

    if (tail == NULL) // caso em que não há outro mais pequeno
    {
        return NULL;
    }

    // encontrámos uma tail - obter metadados
    int metadados_len = ZDATALEN * sizeof(char);
    char *metadados = malloc(metadados_len);

    int ret = zoo_wget(zh, tail, NULL, NULL, metadados, &metadados_len, NULL);
    if (ret == ZOK)
    {
        free(tail);
        return metadados;
    }

    free(metadados);
    free(tail);
    return NULL;
}

char *getIpAddr()
{
    struct ifaddrs *ifaddr;
    int family, s;
    char host[1025];

    if (getifaddrs(&ifaddr) == -1)
        return NULL;

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        // Se a familia for de IPv4
        if (family == AF_INET)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, 1025, NULL, 0, 1);
            if (s != 0)
            {
                freeifaddrs(ifaddr);
                return NULL;
            }

            // Se for igual ao endereco do loopback
            if (strcmp(host, "127.0.0.1") == 0)
                continue;

            freeifaddrs(ifaddr);
            return strdup(host);
        }
    }

    freeifaddrs(ifaddr);
    return NULL;
}
