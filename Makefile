# João Pereira fc58189
# Martim Pereira fc58223
# Daniel Nunes fc58257
# Grupo 04

CC = gcc
LIBS = -L $(LIB_DIR) -ltable -lprotobuf-c -lzookeeper_mt
INC_DIR = include
SRC_DIR = source
OBJ_DIR = object
BIN_DIR = binary
LIB_DIR = lib
DEP_DIR = dependencies
CFLAGS = -D THREADED -Wall -I$(INC_DIR) -MMD -MP -MF $(DEP_DIR)/$*.d 

# Library source files
LIB_SRCS = $(SRC_DIR)/data.c $(SRC_DIR)/entry.c $(SRC_DIR)/list.c $(SRC_DIR)/table.c
#Vai mostrar quais os ficheiros objetos são necessários a partir dos source files
#patsubst vai substituir os o nome dos ficheiros em source para os respetivos .o
LIB_OBJS = $(OBJ_DIR)/data.o $(OBJ_DIR)/entry.o $(OBJ_DIR)/list.o $(OBJ_DIR)/table.o

# Table-client source files
CLIENT_SRCS = $(SRC_DIR)/sdmessage.pb-c.c $(SRC_DIR)/table_client.c $(SRC_DIR)/client_stub.c $(SRC_DIR)/network_client.c $(SRC_DIR)/utils-private.c $(SRC_DIR)/table_skel.c $(SRC_DIR)/stats.c $(SRC_DIR)/synchronization-private.c $(SRC_DIR)/zookeeper_utils-private.c
CLIENT_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CLIENT_SRCS))

SERVER_SRCS = $(SRC_DIR)/sdmessage.pb-c.c $(SRC_DIR)/table_server.c $(SRC_DIR)/client_stub.c $(SRC_DIR)/network_server.c $(SRC_DIR)/table_skel.c $(SRC_DIR)/utils-private.c $(SRC_DIR)/network_client.c $(SRC_DIR)/stats.c $(SRC_DIR)/synchronization-private.c $(SRC_DIR)/zookeeper_utils-private.c
SERVER_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SERVER_SRCS))

# Executaveis
CLIENT_EXEC = $(BIN_DIR)/table_client
SERVER_EXEC = $(BIN_DIR)/table_server


.PHONY: all

all: $(LIB_DIR)/libtable.a $(CLIENT_EXEC) $(SERVER_EXEC)

$(LIB_DIR)/libtable.a: $(LIB_OBJS)
	ar -rcs $(LIB_DIR)/libtable.a $(LIB_OBJS)

$(CLIENT_EXEC): $(CLIENT_OBJS) $(LIB_DIR)/libtable.a
	$(CC) $(CFLAGS) -o $(CLIENT_EXEC) $(CLIENT_OBJS) $(LIBS)

$(SERVER_EXEC): $(SERVER_OBJS) $(LIB_DIR)/libtable.a
	$(CC) $(CFLAGS) -o $(SERVER_EXEC) $(SERVER_OBJS) $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/sdmessage.pb-c.c: sdmessage.proto
	protoc-c --c_out=. sdmessage.proto
	mv sdmessage.pb-c.c $(SRC_DIR)
	mv sdmessage.pb-c.h $(INC_DIR)

include $(wildcard $(DEP_DIR)/*.d)
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(BIN_DIR)/* $(LIB_DIR)/* $(DEP_DIR)/*