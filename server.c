#include "chat_functions.h"

typedef struct
{
    int socket;
    char name[32];
    struct sockaddr_in address;
    pthread_t tid;
} client_t;

client_t *clients[MAX_CLIENTS]; // 紀錄 client 的 address 和 socket
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
char message_records[MESSAGE_LEN];
int client_count = 0;

volatile sig_atomic_t server_running = 1;

FILE *system_record_file;
FILE *message_record_file;

int server_socket;
struct sockaddr_in server_addr;

int main()
{
    pthread_t tid_for_handleAccept;

    server_init();

    signal(SIGINT, catch_ctrl_c);

    // create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        output(system_record_file, "無法創建 socket\n");
        return -1;
    }

    // create address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    // bind socket & address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        output(system_record_file, "綁定失敗\n");
        return -1;
    }

    // listen to client
    if (listen(server_socket, 10) < 0)
    {
        output(system_record_file, "監聽失敗\n");
        return -1;
    }

    output(system_record_file, "聊天室伺服器啟動\n");

    pthread_create(&tid_for_handleAccept, NULL, handle_accept, NULL);

    while (server_running)
    {
    }

    pthread_cancel(tid_for_handleAccept);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            close(clients[i]->socket);
            pthread_cancel(clients[i]->tid);
            free(clients[i]);
        }
    }

    fclose(system_record_file);
    fclose(message_record_file);

    close(server_socket);

    return 0;
}

void output(FILE *file, char *message)
{
    printf("%s", message);

    pthread_mutex_lock(&clients_mutex);
    fprintf(file, "%s", message);
    fflush(file);
    pthread_mutex_unlock(&clients_mutex);
}

void append_message(char *message)
{
    pthread_mutex_lock(&clients_mutex);

    size_t current_len = strlen(message_records);
    size_t space_left = MESSAGE_LEN - current_len - 1;
    if (space_left > 0)
    {
        strncat(message_records, message, space_left);
    }

    pthread_mutex_unlock(&clients_mutex);
}

void server_init()
{
    memset(message_records, 0, sizeof(message_records));
    memset(clients, 0, sizeof(clients));

    system_record_file = fopen("system_record_file.txt", "w+");
    message_record_file = fopen("message_record_file.txt", "w+");
}

void broadcast_message(char *message, int sender_socket)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] && sender_socket != clients[i]->socket)
        {
            // handle exception
            if (send(clients[i]->socket, message, strlen(message), 0) < 0)
            {
                output(system_record_file, "廣播訊息失敗\n");
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *handle_accept(void *arg)
{
    while (1)
    {
        // create client address
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        // get client socket
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_socket < 0)
        {
            output(system_record_file, "接收連線失敗\n");
            continue;
        }

        if (client_count + 1 == MAX_CLIENTS)
        {
            output(system_record_file, "達到最大連線數\n");
            close(client_socket);
            continue;
        }

        // send messages record
        send(client_socket, message_records, sizeof(message_records), 0);

        // create client
        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->address = client_addr;
        client->socket = client_socket;

        // put into client list
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (!clients[i])
            {
                clients[i] = client;
                break;
            }
        }
        client_count++;

        pthread_mutex_unlock(&clients_mutex);

        // create thread to handle client
        pthread_create(&(client->tid), NULL, handle_client, (void *)client);
        pthread_detach(client->tid);
    }
}

void catch_ctrl_c(int sig)
{
    printf("\n伺服器即將關閉...\n");
    output(system_record_file, "伺服器關閉\n");
    server_running = 0;
}

void *handle_client(void *arg)
{
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE * 2];
    char name[32];

    // receive name
    if (recv(client->socket, name, 32, 0) <= 0)
    {
        output(system_record_file, "無法獲取客戶端名稱\n");
        goto cleanup;
    }

    strcpy(client->name, name);
    snprintf(buffer, sizeof(buffer), "%s 加入了聊天室\n", client->name);

    broadcast_message(buffer, client->socket);
    append_message(buffer);

    output(message_record_file, buffer);

    while (1)
    {
        int receive = recv(client->socket, buffer, BUFFER_SIZE, 0);

        if (receive <= 0)
        {
            snprintf(buffer, sizeof(buffer), "%s 離開了聊天室\n", client->name);

            broadcast_message(buffer, client->socket);
            append_message(buffer);

            output(message_record_file, buffer);
            break;
        }

        buffer[receive] = '\0';
        snprintf(message, sizeof(message), "%s: %s", client->name, buffer);

        broadcast_message(message, client->socket);
        append_message(message);

        output(message_record_file, message);
    }

cleanup:
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] && clients[i]->socket == client->socket)
        {
            clients[i] = NULL;

            pthread_mutex_lock(&clients_mutex);
            client_count--;
            pthread_mutex_unlock(&clients_mutex);
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    close(client->socket);
    free(client);

    return NULL;
}