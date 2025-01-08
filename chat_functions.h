#ifndef CHAT_FUNCTIONS_H
#define CHAT_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MESSAGE_LEN 10000

// server
void broadcast_message(char *message, int sender_socket);
void *handle_client(void *arg);
void server_init();
void *handle_input(void *arg);
void *handle_accept(void *arg);
void catch_ctrl_c(int sig);
void output(FILE *file, char *message);

// client
void *receive_handler(void *arg);
void handle_sigpipe(int sig);

#endif