#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5050
#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server;
    char buf[BUF_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect"); exit(1);
    }

    printf("Connected to KVSS server (127.0.0.1:%d)\n", PORT);

    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (send(sock, buf, strlen(buf), 0) < 0) { perror("send"); break; }

        int n = recv(sock, buf, sizeof(buf)-1, 0);
        if (n <= 0) { printf("Server closed.\n"); break; }
        buf[n] = '\0';
        printf("%s", buf);

        if (strstr(buf, "bye")) break;
    }

    close(sock);
    return 0;
}

