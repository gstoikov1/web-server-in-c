#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PATCH,
    HTTP_DELETE,
    HTTP_UNKNOWN
} HttpMethod;

typedef struct {
    HttpMethod method;
    const char *path;
    void (*handler)(void);
} HttpMapping;

// TODO: switch this to a hashmap
HttpMapping mappings[1024] = {0};
int mappingsCount = 0;

int main(void) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);

    printf("Server running on http://localhost:8080\n");

    while (1) {
        SOCKET client_fd = accept(server_fd, NULL, NULL);

        char buffer[4096] = {0};
        int bytesReceived = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            closesocket(client_fd);
            continue;
        }

        buffer[bytesReceived] = '\0';

        char *lineEnd = strstr(buffer, "\r\n");

        if (lineEnd == NULL) {
            printf("Invalid HTTP request\n");
            closesocket(client_fd);
            continue;
        }

        size_t lineLength = lineEnd - buffer;

        char firstLine[1024];
        char method[10];
        char path[1024];

        if (lineLength >= sizeof(firstLine)) {
            printf("Request line too long\n");
            closesocket(client_fd);
            continue;
        }

        memcpy(firstLine, buffer, lineLength);
        firstLine[lineLength] = '\0';

        char *methodEnd = strchr(firstLine, ' ');

        if (methodEnd == NULL) {
            printf("Invalid request line: missing method separator\n");
            closesocket(client_fd);
            continue;
        }

        size_t methodLength = methodEnd - firstLine;

        if (methodLength >= sizeof(method)) {
            printf("HTTP method too long\n");
            closesocket(client_fd);
            continue;
        }

        memcpy(method, firstLine, methodLength);
        method[methodLength] = '\0';

        char *pathStart = methodEnd + 1;
        char *pathEnd = strchr(pathStart, ' ');

        if (pathEnd == NULL) {
            printf("Invalid request line: missing path separator\n");
            closesocket(client_fd);
            continue;
        }

        size_t pathLength = pathEnd - pathStart;

        if (pathLength >= sizeof(path)) {
            printf("Path too long\n");
            closesocket(client_fd);
            continue;
        }

        memcpy(path, pathStart, pathLength);
        path[pathLength] = '\0';

        printf("First line: %s\n", firstLine);
        printf("Http Method: %s\n", method);
        printf("Path: %s\n", path);

        const char *response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 13\r\n"
                               "\r\n"
                               "Hello, world!";

        send(client_fd, response, (int)strlen(response), 0);
        closesocket(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();

    return 0;
}

void map(HttpMethod method, const char *path, void (*handler)(void)) {
    if (mappingsCount >= 1024) {
        printf("Too many mappings\n");
        return;
    }

    mappings[mappingsCount].method = method;
    mappings[mappingsCount].path = path;
    mappings[mappingsCount].handler = handler;

    mappingsCount++;
}

HttpMethod mapHttpMethodToEnum(const char *method) {
    if (method == NULL) {
        return HTTP_UNKNOWN;
    }

    if (strcmp(method, "GET") == 0) {
        return HTTP_GET;
    }

    if (strcmp(method, "POST") == 0) {
        return HTTP_POST;
    }

    if (strcmp(method, "PATCH") == 0) {
        return HTTP_PATCH;
    }

    if (strcmp(method, "DELETE") == 0) {
        return HTTP_DELETE;
    }

    return HTTP_UNKNOWN;
}