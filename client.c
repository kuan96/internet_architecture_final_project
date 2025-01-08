#include "chat_functions.h"

int sockfd = 0;
int exit_flag = 0;

int main(int argc, char **argv)
{
    char buffer[BUFFER_SIZE] = {};
    char name[32];
    pthread_t tid;

    if (argc != 2)
    {
        printf("Usage: %s <name>\n", argv[0]);
        return EXIT_FAILURE;
    }
    strcpy(name, argv[1]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("無法創建 socket\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8888);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        printf("無法連接到伺服器\n");
        return EXIT_FAILURE;
    }

    char record[MESSAGE_LEN] = {};
    recv(sockfd, record, sizeof(record), 0);

    send(sockfd, name, 32, 0);
    printf("------歡迎來到聊天室!------\n\n");
    printf("===以下是先前的對話紀錄===\n");
    printf("%s", record);
    printf("========================\n\n");

    signal(SIGINT, catch_ctrl_c);

    if (pthread_create(&tid, NULL, receive_handler, NULL) != 0)
    {
        printf("無法創建接收執行緒\n");
        return EXIT_FAILURE;
    }

    while (1)
    {
        if (exit_flag)
        {
            printf("bye bye!\n");
            break;
        }

        fgets(buffer, BUFFER_SIZE, stdin);

        if (send(sockfd, buffer, sizeof(buffer), 0) == -1)
        {
            printf("伺服器已關閉\n");
            break;
        }
    }

    pthread_detach(tid);
    close(sockfd);
    return 0;
}

void catch_ctrl_c(int sig)
{
    exit_flag = 1;
}

void *receive_handler(void *arg)
{
    char message[BUFFER_SIZE] = {};

    while (1)
    {
        int receive = recv(sockfd, message, BUFFER_SIZE, 0);
        if (receive > 0)
        {
            message[receive] = '\0';
            printf("%s", message);
        }

        memset(message, 0, sizeof(message));
    }
}