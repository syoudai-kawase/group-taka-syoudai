#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void *receive_messages(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        int len = read(sock, buffer, sizeof(buffer)-1);
        if (len <= 0) {
            printf("サーバーとの接続が切れました。\n");
            exit(0);
        }
        buffer[len] = '\0';
        printf("\n%s", buffer);
        printf("あなた: "); fflush(stdout);
    }
    return NULL;
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char name[50];

    printf("あなたの名前を入力してください: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';  // 改行削除

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); exit(1);
    }

    // 名前送信
    write(sock, name, strlen(name));

    // 受信スレッド
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, &sock);

    // メインスレッドで送信
    while (1) {
        printf("あなた: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        write(sock, buffer, strlen(buffer));
    }

    close(sock);
    return 0;
}

