/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#include "table_skel.h"
#include "network_server.h"
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utils-private.h"
#include "stats-private.h"
#include "stats.h"
#include "zookeeper/zookeeper.h"
#include "utils-private.h"
#include "zookeeper_utils-private.h"

// parte anterior
struct table_t *table = NULL;
int sockfd;

// parte 4
static char *root_path = "/chain";
static zhandle_t *zh;

char *node_Path;
char *next_node_metadados;
struct rtable_t *next_server = NULL;

void sigint_handler();

//-------------------------ZOOKEEPERFUNCS-------------------------//
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state != ZOO_CONNECTED_STATE)
        {
            sigint_handler();
        }
    }
}

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
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

            if (next_node_metadados != NULL)
            {
                free(next_node_metadados);
            }
            next_node_metadados = find_next_server(zh, node_Path, root_path, children_list);
            if (next_server != NULL)
            {
                rtable_disconnect(next_server);
            }
            next_server = rtable_connect(next_node_metadados);
            update_next_server(next_server);
        }
    }
    free(children_list);
}
//-------------------------END--------------------------------//
int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sigint_handler);

    if (argc != 4)
    {
        perror("Argumentos Inválidos");
        return -1;
    }
    int port = atoi(argv[1]);
    // Int pode ter um valor maior do que os permitidos de um short
    if (port >= SHRT_MIN && port <= SHRT_MAX)
    {
        short shortPort = (short)port;
        // int sockfd;
        if ((sockfd = network_server_init(shortPort)) == -1)
        {
            return -1;
        }

        table = table_skel_init(atoi(argv[2]));
        if (table == NULL)
        {
            perror("Erro ao inicializar tabela");
            return -1;
        }

        // Connect to Zookeeper
        zh = zookeeper_init(argv[3], connection_watcher, 20000000, 0, NULL, 0);
        if (zh == NULL)
        {
            fprintf(stderr, "Erro ao conectar ao Zookeeper\n");
            return -1;
        }

        zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);

        // Tentar criar nó raiz
        while (ZNONODE == zoo_exists(zh, root_path, 0, NULL))
        {
            if (ZOK != zoo_create(zh, root_path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0))
            {
                fprintf(stderr, "Error Creating %s!\n", root_path);
                zookeeper_close(zh);
                network_server_close(sockfd);
                return -1;
            }
        }

        // Criar filho efemero
        node_Path = znode_create(zh, root_path, sockfd);
        if (node_Path == NULL)
        {
            zookeeper_close(zh);
            return -1;
        }

        // dar watch dos filhos
        zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
        if (ZOK != zoo_wget_children(zh, root_path, child_watcher, NULL, children_list))
        {
            fprintf(stderr, "Error setting watch at %s!\n", root_path);
        }
        next_node_metadados = find_next_server(zh, node_Path, root_path, children_list);
        next_server = rtable_connect(next_node_metadados);

        char *previous_node_metadados = find_previous_server(zh, node_Path, root_path, children_list);
        free(children_list);

        struct rtable_t *temp = rtable_connect(previous_node_metadados);
        if (temp != NULL)
        {
            struct entry_t **gettable = rtable_get_table(temp);

            if (gettable != NULL)
            {
                for (int i = 0; gettable[i] != NULL; i++)
                {

                    if (table_put(table, gettable[i]->key, gettable[i]->value) == -1)
                    {
                        fprintf(stderr, "Error while replicating the tables!\n");
                    }
                }

                rtable_free_entries(gettable);
            }

            if (rtable_disconnect(temp) == -1)
            {
                fprintf(stderr, "Error while disconnecting form previous server!\n");
            }
        }
        free(previous_node_metadados); // maybe gera double frees

        network_main_loop(sockfd, table, next_server);
        network_server_close(sockfd);
        table_skel_destroy(table);
        free(node_Path);
        zookeeper_close(zh);
        return 0;
    }
    else
    {
        printf("Valor demasiado grande para um short.\n");
        return -1;
    }
    return 0;
}

void sigint_handler()
{
    printf("\nServidor fechado\n");
    free(node_Path);
    if (next_node_metadados != NULL)
    {
        free(next_node_metadados);
    }
    if (next_server != NULL)
    {
        rtable_disconnect(next_server);
    }
    table_skel_destroy(table);
    zookeeper_close(zh);
    network_server_close(sockfd);
    exit(0);
}
