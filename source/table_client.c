/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#include "client_stub.h"
#include "network_client.h"
#include "table_skel.h"
#include "utils-private.h"
#include "stats.h"
#include "zookeeper/zookeeper.h"
#include "zookeeper_utils-private.h"
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

struct rtable_t *head = NULL;
struct rtable_t *tail = NULL;
static zhandle_t *zh;
static char *root_path = "/chain";

void sigint_handler();
void process_put(char *token);
void process_get(char *token);
void process_del(char *token);
void process_size();
void process_get_keys();
void process_get_table();
void process_quit();
void process_help();
void process_stats();

// --------------------------ZOOKEEPERFUNCS----------------------------//
void connection_watcher(zhandle_t *zh, int type, int state, const char *path, void *context)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state != ZOO_CONNECTED_STATE)
        {
            sigint_handler();
        }
    }
}

void child_watcher(zhandle_t *zh, int type, int state, const char *zpath, void *watcher_ctx)
{
    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {
            if (ZOK != zoo_wget_children(zh, root_path, child_watcher, watcher_ctx, children_list))
            {
                fprintf(stderr, "Error setting watch at %s!\n", root_path);
            }

            char *head_metadados = find_head_server(zh, root_path, children_list);
            char *tail_metadados = find_tail_server(zh, root_path, children_list);

            if (head != NULL)
            {
                rtable_disconnect(head);
            }
            if (tail != NULL)
            {
                rtable_disconnect(tail);
            }

            head = rtable_connect(head_metadados);
            tail = rtable_connect(tail_metadados);

            free(head_metadados);
            free(tail_metadados);

            if (head == NULL && tail == NULL)
            {
                perror("Nó raiz não tem filhos");
            }
        }
    }
    free(children_list);
}

// --------------------------END----------------------------------------//

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sigint_handler);

    if (argc != 2)
    {
        perror("Argumentos Inválidos");
        return -1;
    }

    // Connect to Zookeeper
    zh = zookeeper_init(argv[1], connection_watcher, 20000000, 0, NULL, 0);
    if (zh == NULL)
    {
        fprintf(stderr, "Erro ao conectar ao Zookeeper\n");
        return -1;
    }

    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);

    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    if (ZOK != zoo_wget_children(zh, root_path, child_watcher, NULL, children_list))
    {
        fprintf(stderr, "Error setting watch at %s!\n", root_path);
        return -1;
    }

    char *head_metadados = find_head_server(zh, root_path, children_list);
    char *tail_metadados = find_tail_server(zh, root_path, children_list);
    free(children_list);
    
    if (head != NULL)
    {
        rtable_disconnect(head);
    }
    if (tail != NULL)
    {
        rtable_disconnect(tail);
    }

    head = rtable_connect(head_metadados);
    tail = rtable_connect(tail_metadados);

    free(head_metadados);
    free(tail_metadados);

    if (head == NULL && tail == NULL)
    {
        perror("Nó raiz não tem filhos");
        return -1;
    }

    while (1)
    {
        char input[100];

        do
        {
            printf("Comando: ");
            fgets(input, sizeof(input), stdin);

        } while (strlen(input) == 1);

        int len = strlen(input);

        // Remove the newline character if it exists
        if (len > 0 && input[len - 1] == '\n')
        {
            input[len - 1] = '\0';
        }

        // Use strtok to split the string by the delimiter
        char *token = strtok(input, " ");

        // The first token is the hostname
        if (token)
        {
            char *op = token;

            if (strcmp(op, "put") == 0 || strcmp(op, "p") == 0)
            {
                process_put(token);
            }
            else if (strcmp(op, "get") == 0 || strcmp(op, "g") == 0)
            {
                process_get(token);
            }
            else if (strcmp(op, "del") == 0 || strcmp(op, "d") == 0)
            {
                process_del(token);
            }
            else if (strcmp(op, "size") == 0 || strcmp(op, "s") == 0)
            {
                process_size();
            }
            else if (strcmp(op, "getkeys") == 0 || strcmp(op, "k") == 0)
            {
                process_get_keys();
            }
            else if (strcmp(op, "gettable") == 0 || strcmp(op, "t") == 0)
            {
                process_get_table();
            }
            else if (strcmp(op, "stats") == 0)
            {
                process_stats();
            }
            else if (strcmp(op, "quit") == 0 || strcmp(op, "q") == 0)
            {
                process_quit();
            }
            else
            {
                process_help();
            }
        }
        else
        {
            return -1;
        }
    }
}

// ###########################################################################################################

void sigint_handler()
{
    printf("\nBye, Bye!\n");
    if (head != NULL)
    {
        rtable_disconnect(head);
    }
    if (tail != NULL)
    {
        rtable_disconnect(tail);
    }
    zookeeper_close(zh);
    exit(0);
}

void process_put(char *token)
{
    char *key = strtok(NULL, " \n");
    char *data = strtok(NULL, "\n");

    if (key == NULL || data == NULL)
    {
        printf("Argumentos Inválidos. Uso: [p]ut <key> <value>\n");
        return;
    }

    char *data_dup = malloc(strlen(data));
    memcpy(data_dup, data, strlen(data));
    struct data_t *value = data_create(strlen(data), data_dup);
    if (data == NULL)
    {
        printf("Erro no rtable_put\n");
        return;
    }
    struct entry_t *entry = entry_create(strdup(key), value);

    if (rtable_put(head, entry) == -1)
    {
        printf("Erro no rtable_put\n");
        return;
    }
    entry_destroy(entry);
}

void process_get(char *token)
{
    char *key = strtok(NULL, "\n");
    if (key == NULL)
    {
        printf("Argumentos Inválidos. Uso: [g]et <key>\n");
        return;
    }

    struct data_t *value = rtable_get(tail, key);
    if (value == NULL)
    {
        printf("Erro em rtable_get ou key não encontrada\n");
        return;
    }

    printf("Key %s está associada a valor %.*s\n", key, value->datasize, (char *)(value->data));
    data_destroy(value);
}

void process_del(char *token)
{
    char *key = strtok(NULL, "\n");
    if (key == NULL)
    {
        printf("Argumentos Inválidos. Uso: [d]el <key>\n");
        return;
    }

    if (rtable_del(head, key) == -1)
    {
        printf("Erro em rtable_del ou key não encontrada\n");
    }
}

void process_size()
{
    int size = rtable_size(tail);
    if (size == -1)
    {
        return;
    }
    printf("%d\n", size);
}

void process_get_keys()
{
    char **keys = rtable_get_keys(tail);

    if (keys == NULL)
    {
        return;
    }
    for (int i = 0; keys[i] != NULL; i++)
    {
        printf("%s\n", keys[i]);
    }
    rtable_free_keys(keys);
}

void process_get_table()
{
    struct entry_t **entries = rtable_get_table(tail);
    if (entries == NULL)
    {
        return;
    }
    for (int i = 0; entries[i] != NULL; i++)
    {
        printf("%s :: %.*s\n", entries[i]->key, entries[i]->value->datasize, (char *)entries[i]->value->data);
    }
    rtable_free_entries(entries);
}

void process_quit()
{
    if (head != NULL)
    {
        rtable_disconnect(head);
    }
    if (tail != NULL)
    {
        rtable_disconnect(tail);
    }
    printf("Bye, Bye!\n");
    zookeeper_close(zh);
    exit(0);
}

void process_help()
{

    printf("Comandos válidos:\n");
    printf(" > p[ut] <key> <value>\n");
    printf(" > g[et] <key>\n");
    printf(" > d[el] <key>\n");
    printf(" > s[ize]\n");
    printf(" > [get]k[eys]\n");
    printf(" > [get]t[able]\n");
    printf(" > stats\n");
    printf(" > q[uit]\n");
    printf(" > h[elp]\n");
}

void process_stats()
{
    struct statistics_t *stats = rtable_stats(tail);
    if (stats == NULL)
    {
        return;
    }
    printf("Clientes conectados: %d\n", stats->clients_connected);
    printf("Operações feitas na tabela: %d\n", stats->amount);
    printf("Microsegundos: %ld\n", stats->total_time.tv_sec * 1000000 + stats->total_time.tv_usec);
    free(stats);
}