/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#include "network_server.h"
#include "sdmessage.pb-c.h"
#include "utils-private.h"
#include "table_skel.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "stats.h"
#include "synchronization-private.h"
#include "zookeeper/zookeeper.h"
struct table_t *t = NULL;
struct rtable_t *next_s = NULL;
struct statistics_t *stats = NULL;

/* Função para preparar um socket de receção de pedidos de ligação
 * num determinado porto.
 * Retorna o descritor do socket ou -1 em caso de erro.
 */
int network_server_init(short port) // OK
{
    int sockfd;
    // Cria um socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Erro ao criar o socket");
        return -1;
    }

    int option_value = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value)) == -1)
    {
        perror("Erro no setsockopt");
    }
    // Define as informações do endereço de escuta
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Vincula o socket ao endereço e à porta
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Erro ao vincular o socket");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 0) < 0)
    {
        perror("Erro ao executar listen");
        close(sockfd);
        return -1;
    }

    stats = stats_create();
    if (stats == NULL)
    {
        return -1;
    }

    return sockfd;
}

/* A função network_main_loop() deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada
     na tabela table;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 * A função não deve retornar, a menos que ocorra algum erro. Nesse
 * caso retorna -1.
 */
int network_main_loop(int listening_socket, struct table_t *table, struct rtable_t *next)
{
    t = table;
    int connsockfd;
    struct sockaddr_in client;
    socklen_t size_client = sizeof(client);
    next_s = next;
    printf("Servidor ligado\n");
    while ((connsockfd = accept(listening_socket, (struct sockaddr *)&client, &size_client)) != -1)
    {
        printf("Cliente conectado\n");
        pthread_t thr;
        int *i = malloc(sizeof(int));
        *i = connsockfd;
        if (pthread_create(&thr, NULL, &network_main_loop_thread, i) != 0)
        {
            continue;
        }
        pthread_detach(thr);
    }
    free(next_s);
    return 0;
}

void *network_main_loop_thread(void *arg)
{
    enter_write(stats->m, stats->c, &(stats->counter_w), &(stats->counter_r));
    update_clients_status(stats, 1);
    exit_write(stats->m, stats->c, &(stats->counter_w));

    // unlock
    int connsockfd = *(int *)arg;
    MessageT *msg;
    while ((msg = network_receive(connsockfd)) != NULL)
    {

        if (invoke(msg, t, stats, next_s) == -1)
        {
            message_t__free_unpacked(msg, NULL);
            break;
        }

        int ret = network_send(connsockfd, msg);
        message_t__free_unpacked(msg, NULL);

        if (ret == -1)
        {
            continue;
        }
    }
    close(connsockfd);
    free(arg);
    enter_write(stats->m, stats->c, &(stats->counter_w), &(stats->counter_r));
    update_clients_status(stats, -1);
    exit_write(stats->m, stats->c, &(stats->counter_w));
    return NULL;
}

/* A função network_receive() deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura MessageT.
 * Retorna a mensagem com o pedido ou NULL em caso de erro.
 */
MessageT *network_receive(int client_socket)
{
    int nbytes;
    uint16_t size_read = 0;
    // receber short q é tamanho do buffer a receber
    if ((nbytes = read_all(client_socket, (char *)&size_read, sizeof(size_read))) != sizeof(size_read))
    {
        printf("Cliente desconectado\n");
        return NULL;
    }

    int16_t size = ntohs(size_read);
    uint8_t *buf_receive = malloc(size);

    if (buf_receive == NULL)
    {
        return NULL;
    }
    // recebe buffer com o tamanho recebido anteriormente
    if ((nbytes = read_all(client_socket, (char *)buf_receive, size)) != size)
    {
        free(buf_receive);
        perror("Erro ao receber dados do cliente");
        return NULL;
    };

    MessageT *received = message_t__unpack(NULL, size, buf_receive);
    free(buf_receive);
    return received;
}

/* A função network_send() deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Enviar a mensagem serializada, através do client_socket.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int network_send(int client_socket, MessageT *msg)
{
    size_t len = message_t__get_packed_size(msg);
    uint8_t *buf = malloc(len);
    if (buf == NULL)
    {
        perror("Malloc error: network_client.network_send_receive");
        return -1;
    }
    message_t__pack(msg, buf);
    int nbytes;
    unsigned short buf_size = htons(len);
    // enviar tamanho da string
    if ((nbytes = write_all(client_socket, (char *)&buf_size, sizeof(buf_size))) != sizeof(buf_size))
    {
        free(buf);
        perror("Erro ao enviar dados ao servidor");
        return -1;
    }
    // enviar buffer
    if ((nbytes = write_all(client_socket, (char *)buf, len)) != len)
    {
        free(buf);
        perror("Erro ao enviar dados ao servidor");
        return -1;
    }
    free(buf);
    return 0;
}

/* Liberta os recursos alocados por network_server_init(), nomeadamente
 * fechando o socket passado como argumento.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int network_server_close(int socket)
{
    stats_destroy(stats);
    return close(socket);
}

/**
 * Atualiza a variável global relativamente a qual o próximo
 * servidor na chain é (sucessor)
 */
void update_next_server(struct rtable_t *next)
{
    next_s = next;
}
