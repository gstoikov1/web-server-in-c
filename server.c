#include "http.h"
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

int findHttpMapping(HttpMapping *mapping, HttpRequestLine *req);
void makeMappings();
void handleGetUsers();
void handleGetRoot();
void map(HttpMethod method, const char *path, void (*handler)(void));
void printRequest(HttpRequest *req);

const char *response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-Length: 13\r\n"
                       "\r\n"
                       "Hello, world!";

// TODO: switch this to a hashmap
HttpMapping mappings[1024] = {0};
int mappingsCount = 0;

int main(void) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // test function
    makeMappings();

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

        if (bytesReceived == sizeof(buffer) - 1) {
            printf("Request too large or possibly truncated\n");
            closesocket(client_fd);
            continue;
        }

        buffer[bytesReceived] = '\0';

        HttpRequest request = {0};
        extractHttpRequest(&request, buffer, bytesReceived);
        printRequest(&request);
        send(client_fd, response, (int)strlen(response), 0); // HttpRequestLine requestLine;
        closesocket(client_fd);                              // HttpHeader headers[MAX_HEADERS];
                                                             // int headersCount;
                                                             // HttpBody body;
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

int findHttpMapping(HttpMapping *mapping, HttpRequestLine *req) {
    for (int i = 0; i < mappingsCount; i++) {
        HttpMapping *curr = &mappings[i];

        if (curr->method == req->method && strcmp(curr->path, req->path) == 0) {
            // printf("found a mapping for %s %s\n", httpMethodToString(req->method), req->path);
            *mapping = *curr;
            return 0;
        }
    }

    printf("did not find a mapping for %s %s\n", httpMethodToString(req->method), req->path);
    return -1;
}

void makeMappings() {
    map(HTTP_GET, "/", handleGetRoot);
    map(HTTP_GET, "/users", handleGetUsers);
}

void printRequest(HttpRequest *req) {
    printf("*******REQUEST_LINE*******\n");
    printf("METHOD: %s | PATH: %s\n", httpMethodToString(req->requestLine.method),
           req->requestLine.path);

    printf("*******HTTP_HEADERS*******\n");
    for (int i = 0; i < req->headersCount; i++) {
        printf("%d. NAME:%s VALUE:%s\n", i + 1, req->headers[i].name, req->headers[i].value);
    }

    printf("*******HTTP_BODY*******\n");
    printf("%s\n", req->body);
}

void handleGetUsers() {
    // printf("Handling GET /users call\n");
}

void handleGetRoot() {
    //  printf("Handling GET / call\n");
}