/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */

#include "client_stub.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"
#include "network_client.h"
#include "network_server.h"
#include "entry.h"
#include "stats.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna a estrutura rtable preenchida, ou NULL em caso de erro.
 */
struct rtable_t *rtable_connect(char *address_port)
{
    if (address_port == NULL)
    {
        return NULL;
    }

    char dup_address_port[strlen(address_port) + 1];
    strcpy(dup_address_port, address_port);

    char *ip = strtok(dup_address_port, ":");
    char *port = strtok(NULL, ":");
    if (ip == NULL || port == NULL)
    {
        return NULL;
    }

    struct rtable_t *table = malloc(sizeof(struct rtable_t));
    if (table == NULL)
    {
        return NULL;
    }

    char *ip2 = malloc((strlen(ip) + 1) * sizeof(char));
    if (ip2 == NULL)
    {
        free(table);
        return NULL;
    }
    strcpy(ip2, ip);

    table->server_address = ip2;
    table->server_port = atoi(port);

    if (network_connect(table) == -1)
    {
        free(table);
        return NULL;
    }

    return table;
}

/* Termina a associação entre o cliente e o servidor, fechando a
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem, ou -1 em caso de erro.
 */
int rtable_disconnect(struct rtable_t *rtable)
{
    if (rtable == NULL)
    {
        return -1;
    }

    if (network_close(rtable) == -1)
    {
        return -1;
    }
    free(rtable);
    return 0;
}

/* Função para adicionar um elemento na tabela.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Retorna 0 (OK, em adição/substituição), ou -1 (erro).
 */
int rtable_put(struct rtable_t *rtable, struct entry_t *entry)
{
    if (rtable == NULL || entry == NULL)
    {
        return -1;
    }

    MessageT send_msg;
    message_t__init(&send_msg);
    send_msg.opcode = MESSAGE_T__OPCODE__OP_PUT;
    send_msg.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    EntryT e;
    entry_t__init(&e);
    e.key = entry->key;
    e.value.data = entry->value->data;
    e.value.len = entry->value->datasize;
    send_msg.entry = &e;

    MessageT *response = network_send_receive(rtable, &send_msg);
    if (response == NULL)
    {
        return -1;
    }
    if (response->opcode != MESSAGE_T__OPCODE__OP_PUT + 1 ||
        response->c_type != MESSAGE_T__C_TYPE__CT_NONE)
    {
        entry_destroy(entry);
        message_t__free_unpacked(response, NULL);
        return -1;
    }
    message_t__free_unpacked(response, NULL);
    return 0;
}

/* Retorna o elemento da tabela com chave key, ou NULL caso não exista
 * ou se ocorrer algum erro.
 */
struct data_t *rtable_get(struct rtable_t *rtable, char *key)
{
    if (rtable == NULL || key == NULL)
    {
        return NULL;
    }

    MessageT send_msg;
    message_t__init(&send_msg);
    send_msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    send_msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    send_msg.key = key;

    MessageT *response = network_send_receive(rtable, &send_msg);
    if (response == NULL)
    {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    if (response->opcode != MESSAGE_T__OPCODE__OP_GET + 1 ||
        response->c_type != MESSAGE_T__C_TYPE__CT_VALUE)
    {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    void *dup = malloc(response->value.len);
    if (dup == NULL)
    {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    memcpy(dup, response->value.data, response->value.len);
    struct data_t *ret = data_create(response->value.len, dup);
    message_t__free_unpacked(response, NULL);

    if (ret == NULL)
    {
        free(dup);
        return NULL;
    }
    return ret;
}

/* Função para remover um elemento da tabela. Vai libertar
 * toda a memoria alocada na respetiva operação rtable_put().
 * Retorna 0 (OK), ou -1 (chave não encontrada ou erro).
 */
int rtable_del(struct rtable_t *rtable, char *key)
{
    if (rtable == NULL || key == NULL)
    {
        return -1;
    }

    MessageT send_msg;
    message_t__init(&send_msg);
    send_msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    send_msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    send_msg.key = key;

    MessageT *response = network_send_receive(rtable, &send_msg);
    if (response == NULL)
    {
        return -1;
    }

    if (response->opcode != MESSAGE_T__OPCODE__OP_DEL + 1 ||
        response->c_type != MESSAGE_T__C_TYPE__CT_NONE)
    {
        message_t__free_unpacked(response, NULL);
        return -1;
    }
    message_t__free_unpacked(response, NULL);
    return 0;
}

/* Retorna o número de elementos contidos na tabela ou -1 em caso de erro.
 */
int rtable_size(struct rtable_t *rtable)
{
    if (rtable == NULL)
    {
        return -1;
    }

    MessageT send_msg;
    message_t__init(&send_msg);
    send_msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    send_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response = network_send_receive(rtable, &send_msg);

    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR ||
        response->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        message_t__free_unpacked(response, NULL);
        return -1;
    }
    int size = response->result;
    message_t__free_unpacked(response, NULL);
    return size;
}

/* Retorna um array de char* com a cópia de todas as keys da tabela,
 * colocando um último elemento do array a NULL.
 * Retorna NULL em caso de erro.
 */
char **rtable_get_keys(struct rtable_t *rtable)
{
    if (rtable == NULL)
    {
        return NULL;
    }

    MessageT send_msg;
    message_t__init(&send_msg);
    send_msg.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    send_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    MessageT *response = network_send_receive(rtable, &send_msg);

    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR ||
        response->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        return NULL;
    }

    int n_keys = response->n_keys;
    char **keys = malloc(sizeof(char *) * (n_keys + 1));

    if (keys == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < n_keys; i++)
    {
        keys[i] = malloc(strlen(response->keys[i]) + 1);
        if (keys[i] == NULL)
        {

            for (int j = 0; j < i; j++)
            {
                free(keys[j]);
            }
            free(keys);
            message_t__free_unpacked(response, NULL);
            return NULL;
        }
        memcpy(keys[i], response->keys[i], strlen(response->keys[i]) + 1);
    }
    message_t__free_unpacked(response, NULL);
    keys[n_keys] = NULL;
    return keys;
}

/* Liberta a memória alocada por rtable_get_keys().
 */
void rtable_free_keys(char **keys)
{
    table_free_keys(keys);
}

/* Retorna um array de entry_t* com todo o conteúdo da tabela, colocando
 * um último elemento do array a NULL. Retorna NULL em caso de erro.
 */
struct entry_t **rtable_get_table(struct rtable_t *rtable)
{
    if (rtable == NULL)
    {
        return NULL;
    }

    MessageT send_msg;
    message_t__init(&send_msg);
    send_msg.opcode = MESSAGE_T__OPCODE__OP_GETTABLE;
    send_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response = network_send_receive(rtable, &send_msg);

    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR ||
        response->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        return NULL;
    }

    int n_entries = response->n_entries;
    struct entry_t **entries = malloc(sizeof(struct entry_t) * (n_entries + 1));

    if (entries == NULL)
    {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    for (int i = 0; i < n_entries; i++)
    {
        void *data_dup = malloc(response->entries[i]->value.len);
        if (data_dup == NULL)
        {
            for (int j = 0; j < i; j++)
            {
                entry_destroy(entries[j]);
            }
            free(entries);
            message_t__free_unpacked(response, NULL);
            return NULL;
        }

        memcpy(data_dup, response->entries[i]->value.data, response->entries[i]->value.len);
        struct data_t *value = data_create(response->entries[i]->value.len, data_dup);
        entries[i] = entry_create(strdup(response->entries[i]->key), value);

        if (entries[i] == NULL)
        {
            for (int j = 0; j < i; j++)
            {
                entry_destroy(entries[j]);
            }
            free(entries);
            message_t__free_unpacked(response, NULL);
            return NULL;
        }
    }
    entries[n_entries] = NULL;
    message_t__free_unpacked(response, NULL);
    return entries;
}

/* Liberta a memória alocada por rtable_get_table().
 */
void rtable_free_entries(struct entry_t **entries)
{
    for (int i = 0; entries[i] != NULL; i++)
    {
        entry_destroy(entries[i]);
    }
    free(entries);
}

/* Obtém as estatísticas do servidor. */
struct statistics_t *rtable_stats(struct rtable_t *rtable)
{
    if (rtable == NULL)
    {
        return NULL;
    }

    MessageT send_msg;
    message_t__init(&send_msg);
    send_msg.opcode = MESSAGE_T__OPCODE__OP_STATS;
    send_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response = network_send_receive(rtable, &send_msg);

    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR ||
        response->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        return NULL;
    }

    struct statistics_t *ret = malloc(sizeof(struct statistics_t));
    ret->amount = response->stats->amount;
    ret->clients_connected = response->stats->clients_connected;
    ret->total_time.tv_sec = response->stats->seconds;
    ret->total_time.tv_usec = response->stats->microseconds;

    message_t__free_unpacked(response, NULL);

    return ret;
}