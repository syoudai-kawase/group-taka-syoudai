#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int sock;
    char name[50];
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// 他のクライアントにメッセージを送信
void broadcast_message(char *message, int sender_sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].sock != sender_sock) {
            write(clients[i].sock, message, strlen(message));
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    client_t *cli = (client_t *)arg;
    char buffer[BUFFER_SIZE];

    // 名前登録
    int len = read(cli->sock, buffer, sizeof(buffer)-1);
    if (len <= 0) {
        close(cli->sock);
        return NULL;
    }
    buffer[len] = '\0';
    strncpy(cli->name, buffer, sizeof(cli->name));

    char welcome[BUFFER_SIZE];
    snprintf(welcome, sizeof(welcome), "[サーバー] %s が入室しました\n", cli->name);
    printf("%s", welcome);
    broadcast_message(welcome, cli->sock);

    // メッセージ受信ループ
    while (1) {
        len = read(cli->sock, buffer, sizeof(buffer)-1);
        if (len <= 0) {
            snprintf(welcome, sizeof(welcome), "[サーバー] %s が退室しました\n", cli->name);
            printf("%s", welcome);
            broadcast_message(welcome, cli->sock);
            close(cli->sock);
            
            pthread_mutex_lock(&clients_mutex);
            // 配列から削除
            for (int i = 0; i < client_count; i++) {
                if (clients[i].sock == cli->sock) {
                    for (int j = i; j < client_count - 1; j++)
                        clients[j] = clients[j + 1];
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            break;
        }
        buffer[len] = '\0';

        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message), "[%s] %s\n", cli->name, buffer);
        broadcast_message(message, cli->sock);
    }
    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // ソケット作成
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); exit(1);
    }

    if (listen(server_fd, 5) < 0) { perror("listen"); exit(1); }
    printf("サーバー起動、接続待機中...\n");

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) { perror("accept"); continue; }

        pthread_mutex_lock(&clients_mutex);
        if (client_count >= MAX_CLIENTS) {
            printf("接続拒否: 最大クライアント数です\n");
            close(client_fd);
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }

        client_t *cli = &clients[client_count++];
        cli->sock = client_fd;
        pthread_mutex_unlock(&clients_mutex);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, cli);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
