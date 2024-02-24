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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils-private.h"

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) com base na
 *   informação guardada na estrutura rtable;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtable;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtable_t *rtable)
{
    struct sockaddr_in server;
    if (rtable == NULL)
    {
        perror("Tabela inválida");
        return -1;
    }

    if ((rtable->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {

        perror("Erro ao criar socket TCP");

        return -1;
    }

    // Preenche estrutura server para estabelecer conexao

    server.sin_family = AF_INET;
    server.sin_port = htons(rtable->server_port);
    if (inet_pton(AF_INET, rtable->server_address, &server.sin_addr) < 1)
    {
        printf("Erro ao converter IP\n");
        close(rtable->sockfd);
        return -1;
    }
    if (connect(rtable->sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Erro ao conectar-se ao servidor");
        close(rtable->sockfd);
        return -1;
    }
    return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtable_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Tratar de forma apropriada erros de comunicação;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
MessageT *network_send_receive(struct rtable_t *rtable, MessageT *msg)
{
    if (rtable == NULL || msg == NULL)
    {
        perror("Bad parameters");
        return NULL;
    }

    int sockfd = rtable->sockfd;
    size_t len = message_t__get_packed_size(msg);
    uint8_t *buf = malloc(len);

    if (buf == NULL)
    {
        perror("Malloc error: network_client.network_send_receive");
        return NULL;
    }

    message_t__pack(msg, buf);
    int nbytes;
    unsigned short buf_size = htons(len);

    // enviar tamanho da string
    if ((nbytes = write_all(rtable->sockfd, (char *)&buf_size, sizeof(buf_size))) != sizeof(buf_size))
    {
        free(buf);
        perror("Erro ao enviar dados ao servidor");
        close(sockfd);
        return NULL;
    }
    // enviar buffer
    if ((nbytes = write_all(sockfd, (char *)buf, len)) != len)
    {
        free(buf);
        perror("Erro ao enviar dados ao servidor");
        close(sockfd);
        return NULL;
    }
    free(buf);

    uint16_t size_read = 0;
    // receber short q é tamanho do buffeer a recebr
    if ((nbytes = read_all(sockfd, (char *)&size_read, sizeof(size_read))) != sizeof(size_read))
    {
        perror("Erro ao receber dados do servidor");
        close(sockfd);
        return NULL;
    }

    unsigned short size_read2 = ntohs(size_read);
    uint8_t *buf_receive = malloc(size_read2);
    if (buf_receive == NULL)
    {
        return NULL;
    }
    // recebe buffer com o tamanho recebido anteriormente
    if ((nbytes = read_all(sockfd, (char *)buf_receive, size_read2)) != size_read2)
    {
        free(buf_receive);
        perror("Erro ao receber dados do servidor");
        close(sockfd);
        return NULL;
    };

    MessageT *received = message_t__unpack(NULL, size_read2, buf_receive);
    free(buf_receive);
    return received;
}

/* Fecha a ligação estabelecida por network_connect().
 * Retorna 0 (OK) ou -1 (erro).
 */
int network_close(struct rtable_t *rtable)
{
    if (rtable == NULL)
    {
        return -1;
    }
    return close(rtable->sockfd);
}