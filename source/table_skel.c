/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "table.h"
#include <errno.h>
#include "stats.h"
#include "sdmessage.pb-c.h"
#include "stats.h"
#include <pthread.h>
#include "synchronization-private.h"
#include "utils-private.h"

pthread_mutex_t m_table = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_table = PTHREAD_COND_INITIALIZER;

int counter_write_table = 0;
int counter_read_table = 0;

/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna a tabela criada ou NULL em caso de erro.
 */
struct table_t *table_skel_init(int n_lists)
{
    return table_create(n_lists);
}

/* Liberta toda a memória ocupada pela tabela e todos os recursos
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_skel_destroy(struct table_t *table)
{
    return table_destroy(table);
}

/* Executa na tabela table a operação indicada pelo opcode contido em msg
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int invoke(MessageT *msg, struct table_t *table, struct statistics_t *stats, struct rtable_t *next_server)
{
    MessageT__Opcode code = msg->opcode;
    MessageT__CType type = msg->c_type;

    struct timeval time_start;
    struct timeval time_end;

    // escrita
    if (code == MESSAGE_T__OPCODE__OP_PUT && type == MESSAGE_T__C_TYPE__CT_ENTRY)
    {

        if (gettimeofday(&time_start, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        void *dup = malloc(msg->entry->value.len);
        if (dup == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        memcpy(dup, msg->entry->value.data, msg->entry->value.len);
        struct data_t *data = data_create(msg->entry->value.len, dup);
        if (data == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        enter_write(&m_table, &c_table, &counter_write_table, &counter_read_table);

        int ret = table_put(table, msg->entry->key, data);
        if (next_server != NULL)
        {
            struct entry_t *entryAux = entry_create(msg->entry->key, data);
            if (entryAux == NULL)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                exit_write(&m_table, &c_table, &counter_write_table);
                return -1;
            }
            if (rtable_put(next_server, entryAux) == -1)
            {
                entry_destroy(entryAux);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                exit_write(&m_table, &c_table, &counter_write_table);
                return -1;
            }
            if (data_destroy(data) == -1)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
        }
        else
        {
            if (data_destroy(data) == -1)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
        }
        exit_write(&m_table, &c_table, &counter_write_table);

        if (ret == -1) 
        {
            data_destroy(data);
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        msg->opcode = msg->opcode + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

        if (gettimeofday(&time_end, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        struct timeval time_diff = get_time_diff(time_end, time_start);

        enter_write(stats->m, stats->c, &(stats->counter_w), &(stats->counter_r));
        update_time_status(stats, time_diff);
        update_counter_status(stats);
        exit_write(stats->m, stats->c, &(stats->counter_w));

        return 0;
    }
    // leitura
    else if (code == MESSAGE_T__OPCODE__OP_GET && type == MESSAGE_T__C_TYPE__CT_KEY)
    {
        if (gettimeofday(&time_start, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        enter_read(&m_table, &c_table, &counter_write_table, &counter_read_table);
        struct data_t *data = table_get(table, msg->key);
        exit_read(&m_table, &c_table, &counter_read_table);
        if (data == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }

        msg->opcode = msg->opcode + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
        msg->value.len = data->datasize;
        void *data_dup = malloc(data->datasize);
        if (data_dup == NULL)
        {
            data_destroy(data);
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        memcpy(data_dup, data->data, data->datasize);
        msg->value.data = data_dup;
        if (data_destroy(data) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        if (gettimeofday(&time_end, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        struct timeval time_diff = get_time_diff(time_end, time_start);

        enter_write(stats->m, stats->c, &(stats->counter_w), &(stats->counter_r));
        update_time_status(stats, time_diff);
        update_counter_status(stats);
        exit_write(stats->m, stats->c, &(stats->counter_w));

        return 0;
    }
    // escrita
    else if (code == MESSAGE_T__OPCODE__OP_DEL && type == MESSAGE_T__C_TYPE__CT_KEY)
    {

        if (gettimeofday(&time_start, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        enter_write(&m_table, &c_table, &counter_write_table, &counter_read_table);
        int ret = table_remove(table, msg->key);

        if (next_server != NULL)
        {
            if (rtable_del(next_server, msg->key) == -1)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                exit_write(&m_table, &c_table, &counter_write_table);
                return -1;
            }
        }
        exit_write(&m_table, &c_table, &counter_write_table);

        if (ret == -1) // esta bom assim ?
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }

        msg->opcode = msg->opcode + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

        if (gettimeofday(&time_end, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        struct timeval time_diff = get_time_diff(time_end, time_start);

        enter_write(stats->m, stats->c, &(stats->counter_w), &(stats->counter_r));
        update_time_status(stats, time_diff);
        update_counter_status(stats);
        exit_write(stats->m, stats->c, &(stats->counter_w));

        return 0;
    }
    // leitura
    else if (code == MESSAGE_T__OPCODE__OP_SIZE && type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        if (gettimeofday(&time_start, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        int size = table_size(table);
        if (size == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        msg->opcode = msg->opcode + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = size;

        if (gettimeofday(&time_end, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        struct timeval time_diff = get_time_diff(time_end, time_start);

        enter_write(stats->m, stats->c, &(stats->counter_w), &(stats->counter_r));
        update_time_status(stats, time_diff);
        update_counter_status(stats);
        exit_write(stats->m, stats->c, &(stats->counter_w));

        return 0;
    }
    // leitura
    else if (code == MESSAGE_T__OPCODE__OP_GETKEYS && type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        if (gettimeofday(&time_start, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        enter_read(&m_table, &c_table, &counter_write_table, &counter_read_table);
        char **keys = table_get_keys(table);
        exit_read(&m_table, &c_table, &counter_read_table);

        if (keys == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        msg->opcode = msg->opcode + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
        msg->keys = keys;

        int counter = 0;
        for (int i = 0; keys[i] != NULL; i++)
        {
            counter += 1;
        }
        msg->n_keys = counter;

        if (gettimeofday(&time_end, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        struct timeval time_diff = get_time_diff(time_end, time_start);

        enter_write(stats->m, stats->c, &(stats->counter_w), &(stats->counter_r));
        update_time_status(stats, time_diff);
        update_counter_status(stats);
        exit_write(stats->m, stats->c, &(stats->counter_w));

        return 0;
    }
    // leitura
    else if (code == MESSAGE_T__OPCODE__OP_GETTABLE && type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        if (gettimeofday(&time_start, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        enter_read(&m_table, &c_table, &counter_write_table, &counter_read_table);
        char **keys = table_get_keys(table);

        if (keys == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        int size = table_size(table);

        if (size == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        msg->n_entries = size;
        EntryT **entries = malloc(size * sizeof(EntryT));
        if (entries == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        for (int i = 0; keys[i] != NULL; i++)
        {
            struct data_t *data = table_get(table, keys[i]);

            if (data == NULL)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

                for (int j = 0; j < i; j++)
                {
                    entry_t__free_unpacked(entries[j], NULL);
                }
                free(entries);

                return -1;
            }

            EntryT *entry = malloc(sizeof(EntryT));
            if (entry == NULL)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
            entry_t__init(entry);
            entry->key = malloc(strlen(keys[i]) + 1);
            if (entry->key == NULL)
            {

                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

                for (int j = 0; j < i; j++)
                {
                    entry_t__free_unpacked(entries[j], NULL);
                }
                free(entries);

                return -1;
            }

            strcpy(entry->key, keys[i]);
            entry->value.len = data->datasize;
            void *dup_data = malloc(data->datasize);
            if (dup_data == NULL)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

                for (int j = 0; j < i; j++)
                {
                    entry_t__free_unpacked(entries[j], NULL);
                }
                free(entries);

                return -1;
            }
            memcpy(dup_data, data->data, data->datasize);
            entry->value.data = dup_data;
            entries[i] = entry;
            if (data_destroy(data) == -1)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

                for (int j = 0; j < i; j++)
                {
                    entry_t__free_unpacked(entries[j], NULL);
                }
                free(entries);

                return -1;
            }
        }
        exit_read(&m_table, &c_table, &counter_read_table);

        table_free_keys(keys);

        msg->opcode = msg->opcode + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_TABLE;
        msg->entries = entries;

        if (gettimeofday(&time_end, NULL) == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        struct timeval time_diff = get_time_diff(time_end, time_start);

        enter_write(stats->m, stats->c, &(stats->counter_w), &(stats->counter_r));
        update_time_status(stats, time_diff);
        update_counter_status(stats);
        exit_write(stats->m, stats->c, &(stats->counter_w));

        return 0;
    }
    else if (code == MESSAGE_T__OPCODE__OP_STATS && type == MESSAGE_T__C_TYPE__CT_NONE)
    {

        StatisticsT *s = malloc(sizeof(EntryT));
        if (s == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        statistics_t__init(s);

        enter_read(stats->m, stats->c, &(stats->counter_w), &(stats->counter_r));
        s->amount = stats->amount;
        s->clients_connected = stats->clients_connected;
        s->seconds = stats->total_time.tv_sec;
        s->microseconds = stats->total_time.tv_usec;
        exit_read(stats->m, stats->c, &(stats->counter_r));

        msg->stats = s;
        msg->opcode = msg->opcode + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_STATS;
        return 0;
    }

    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
    // se a combinacao de op_code e c_type não corresponder a nenhuma válida
    return -1;
}
