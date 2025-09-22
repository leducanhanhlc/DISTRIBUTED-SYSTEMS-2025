#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 5050
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define VERSION "KV/1.0"

// Struct lưu key-value đơn giản (map)
typedef struct KV {
    char key[128];
    char value[128];
    struct KV *next;
} KV;

KV *store = NULL;
time_t start_time = 0;
long served_requests = 0;

// Hàm log request/response
void log_msg(const char *prefix, const char *msg) {
    time_t now = time(NULL);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("[%s] %s: %s\n", buf, prefix, msg);
    fflush(stdout);
}

// Hàm tìm key trong store
KV* find_key(const char *key) {
    KV *cur = store;
    while (cur) {
        if (strcmp(cur->key, key) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

// Thêm/ghi đè key
int put_key(const char *key, const char *value, int *is_new) {
    KV *cur = find_key(key);
    if (cur) {
        strcpy(cur->value, value);
        *is_new = 0;
        return 0;
    }
    KV *n = (KV*)malloc(sizeof(KV));
    strcpy(n->key, key);
    strcpy(n->value, value);
    n->next = store;
    store = n;
    *is_new = 1;
    return 0;
}

// Xóa key
int del_key(const char *key) {
    KV *cur = store, *prev = NULL;
    while (cur) {
        if (strcmp(cur->key, key) == 0) {
            if (prev) prev->next = cur->next;
            else store = cur->next;
            free(cur);
            return 1; // deleted
        }
        prev = cur;
        cur = cur->next;
    }
    return 0; // not found
}

// Đếm số key
int count_keys() {
    int c = 0;
    KV *cur = store;
    while (cur) { c++; cur = cur->next; }
    return c;
}

// Xử lý 1 request
void handle_request(int client_fd, char *buf) {
    served_requests++;
    log_msg("REQ", buf);

    char version[16], cmd[16], key[128], value[128];
    memset(version, 0, sizeof(version));
    memset(cmd, 0, sizeof(cmd));
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));

    int n = sscanf(buf, "%15s %15s %127s %127[^\n]", version, cmd, key, value);

    // Kiểm tra version
    if (strcmp(version, VERSION) != 0) {
        char *resp = "426 UPGRADE_REQUIRED\n";
        send(client_fd, resp, strlen(resp), 0);
        log_msg("RES", resp);
        return;
    }

    if (strcmp(cmd, "PUT") == 0) {
        if (n < 4) {
            char *resp = "400 BAD_REQUEST\n";
            send(client_fd, resp, strlen(resp), 0);
            log_msg("RES", resp);
            return;
        }
        int is_new;
        put_key(key, value, &is_new);
        if (is_new) {
            char *resp = "201 CREATED\n";
            send(client_fd, resp, strlen(resp), 0);
            log_msg("RES", resp);
        } else {
            char msg[BUF_SIZE];
            snprintf(msg, sizeof(msg), "200 OK %s\n", value);
            send(client_fd, msg, strlen(msg), 0);
            log_msg("RES", msg);
        }

    } else if (strcmp(cmd, "GET") == 0) {
        if (n < 3) {
            char *resp = "400 BAD_REQUEST\n";
            send(client_fd, resp, strlen(resp), 0);
            log_msg("RES", resp);
            return;
        }
        KV *cur = find_key(key);
        if (cur) {
            char msg[BUF_SIZE];
            snprintf(msg, sizeof(msg), "200 OK %s\n", cur->value);
            send(client_fd, msg, strlen(msg), 0);
            log_msg("RES", msg);
        } else {
            char *resp = "404 NOT_FOUND\n";
            send(client_fd, resp, strlen(resp), 0);
            log_msg("RES", resp);
        }

    } else if (strcmp(cmd, "DEL") == 0) {
        if (n < 3) {
            char *resp = "400 BAD_REQUEST\n";
            send(client_fd, resp, strlen(resp), 0);
            log_msg("RES", resp);
            return;
        }
        if (del_key(key)) {
            char *resp = "204 NO_CONTENT\n";
            send(client_fd, resp, strlen(resp), 0);
            log_msg("RES", resp);
        } else {
            char *resp = "404 NOT_FOUND\n";
            send(client_fd, resp, strlen(resp), 0);
            log_msg("RES", resp);
        }

    } else if (strcmp(cmd, "STATS") == 0) {
        int uptime = (int)(time(NULL) - start_time);
        char msg[BUF_SIZE];
        snprintf(msg, sizeof(msg), "200 OK keys=%d uptime=%ds served=%ld\n",
                 count_keys(), uptime, served_requests);
        send(client_fd, msg, strlen(msg), 0);
        log_msg("RES", msg);

    } else if (strcmp(cmd, "QUIT") == 0) {
        char *resp = "200 OK bye\n";
        send(client_fd, resp, strlen(resp), 0);
        log_msg("RES", resp);
        close(client_fd);

    } else {
        char *resp = "400 BAD_REQUEST\n";
        send(client_fd, resp, strlen(resp), 0);
        log_msg("RES", resp);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    fd_set readfds;
    int clients[MAX_CLIENTS];
    int i, max_sd, sd;

    for (i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;
    start_time = time(NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(server_fd, 3) < 0) { perror("listen"); exit(1); }

    printf("KVSS server listening on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select"); exit(1);
        }

        if (FD_ISSET(server_fd, &readfds)) {
            client_fd = accept(server_fd, NULL, NULL);
            if (client_fd < 0) { perror("accept"); continue; }
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] < 0) { clients[i] = client_fd; break; }
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buf[BUF_SIZE];
                int n = recv(sd, buf, sizeof(buf)-1, 0);
                if (n <= 0) {
                    close(sd); clients[i] = -1;
                } else {
                    buf[n] = '\0';
                    handle_request(sd, buf);
                }
            }
        }
    }
    return 0;
}

