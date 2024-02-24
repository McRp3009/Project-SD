# Description
The general aim of the project will be to realize a client-server service with a key-value pair storage system.
It has an interface for the user, allowing him to request the execution of operations on a hash table, and on the other hand, the server is programmed to wait for a connection request from the client, thus being ready to receive, execute, and respond to the operations sent. This project was developed in four phases and this is the 4th phase (complete project)

## Execution

Inside the group04 folder, run the following commands to get the job done:  

1. make clean -> cleans the .o, executable, and .d files  
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
    - Make libtable: Compile and make the library . a file containing the data.o entry.o table.o list.o files.
3. Allows multiple clients to be connected to a server.

## Note

This work was carried out and tested using the WSL: Ubuntu system in Visual Studio Code.

The final grade was 20 out of 20.

## Contributors:
João Pereira fc58189
Daniel Nunes fc58257
Martim Pereira fc58223
