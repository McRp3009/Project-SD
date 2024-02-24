## Distributed Systems

Martim Pereira fc58223  
João Pereira fc58189  
Daniel Nunes fc58257  

Group 04  

## Description

The general aim of the project will be to realise a client-server service with a key-value pair storage system.
It has an interface for the user, allowing him to request the execution of operations on a hash table, and on the other hand the server is programmed to wait for a connection request from the client, thus being ready to receive, execute and respond to the operations sent. This phase introduces the possibility of having several clients connected to one server.

This phase specifically introduced support for fault tolerance through replication of the server's state, following the Chain Replication model, using the Apache Zookeeper coordination service.

### To do this, the following modules were developed in the previous phases:

- Creation of modules (network_client and network_server), in which the client will perform the functions of initiating or terminating a connection with the server by sending a request message and then receiving a reply, while the server will be able to initialise itself, responsible for interacting with the network.
- Creation of modules (table_client and table_server), in which the client allows commands to be sent via the network to invoke operations implemented by the server and the server receives the commands received, executes them on the table and returns the response to the client.
- Creation of a module (client_stub), which allows messages and operations sent by the server to be adapted for the client side.
- Creation of a module (table_skel), which allows a client message to be transformed into one or more calls.
- Creation of a module (synchronisation-private), which makes it possible to deal with synchronisation problems between threads.
- Creation of the modules (stats, stats-private), which deal with operations related to the stats_t structure (structure that stores relevant information about the execution).
- Updating the (utils-private) module, adding a function that deals with time differences.
- Modification of the sdmessage.proto file to include a new message type.

### Modules modified/developed in part 4:
- Creation of the module (zookeeper_utils-private), which contains the auxiliary functions responsible for creating ephemeral and sequential znode children of chains and the functions for finding specific znodes in the chain, specifically:
   - find_head_server: locates the server at the head of the chain
   - find_tail_server: locates the server in the tail of the chain
   - find_next_server: locate the successor server
   - find_previous_server: locate the predecessor server
- Modification of the table_server, table_skel and network_server modules so that they implement the following functionalities:
   - Search ZooKeeper for the next (successor) server in the replication chain.
   - After executing a write operation, send it to the next (successor) server in order to propagate the replication
   - Watch ZooKeeper to be notified of changes in the replication chain and connect to its new successor if it has changed.
- Modify the table_client module so that it implements the following functionalities:
  - Search ZooKeeper for the servers at the head and tail of the chain.
  - Send write operations to the server at the head of the chain
  - Send read operations to the server at the tail of the chain 
  - Watch ZooKeeper to be notified of changes in the replication chain and connect to new servers (at the head or tail of the chain),
      if they have changed.

## Execution

Inside the group04 folder, run the following commands to get the job done:  

1. make clean -> cleans the .o, executable and .d files  
2. make all - > create the .o and executable files for the client and server \
3. Switch on zookeeper:\
    - Switch on the zookeeper server using the zkServer.sh start command
    - Switch on the zookeeper client using the zkCli.sh command
4. ./binary/table_server \<port> <number_of_table_rows> \<ip:port> of the zookeeper (if it's already in the binary folder you don't need to include ./binary)  
5. ./binary/table_client \<ip:port> from zookeeper (if it's already in the binary folder you don't need to include the ./binary)

__Note__:

1. you must run the server first
2. There are other compilation rules:
    - Make table_server: Compiles and makes the executable for the server
    - Make table_client: Compile and make the executable for the client
    - Make libtable: Compile and make the library .a file containing the data.o entry.o table.o list.o files.
3. Allows multiple clients to be connected to a server.

## Note

This work was carried out and tested using the WSL: Ubuntu system in Visual Studio Code.

The final grade was 20 out of 20.
