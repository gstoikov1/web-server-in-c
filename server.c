#include "http.h"
#include "vendor/jsmn/jsmn.h"
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

typedef struct {
    HttpMethod method;
    const char *path;
    void (*handler)(void);
} HttpApiMapping;

typedef struct {
    HttpMethod method;
    const char *urlPath;
    const char *filePath;
    const char *type;
} HttpPageMapping;

int Api(HttpApiMapping *mapping, HttpRequestLine *req);
void makeMappings();
void handleGetUsers();
void handleGetRoot();
void mapJson(HttpMethod method, const char *path, void (*handler)(void));
void mapHtml(HttpMethod method, const char *path, const char *pathToFile);
void printRequest(HttpRequest *req);
int sendFile(SOCKET client_fd, const char *filePath, const char *contentType);
int findHttpPageMapping(HttpPageMapping *mapping, HttpRequestLine *req);

const char *response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-Length: 13\r\n"
                       "\r\n"
                       "Hello, world!";

// TODO: switch this to a hashmap
HttpApiMapping apiMappings[1024] = {0};
int apiMappingsCount = 0;

HttpPageMapping pageMappings[1024] = {0};
int pageMappingsCount = 0;

// int main(void) {
//     const char *json = "{\"name\":\"Gabriel\",\"age\":23}";

//     jsmn_parser parser;
//     jsmntok_t tokens[128];

//     jsmn_init(&parser);

//     int tokenCount = jsmn_parse(&parser, json, strlen(json), tokens, 128);

//     if (tokenCount < 0) {
//         printf("Failed to parse JSON: %d\n", tokenCount);
//         return 1;
//     }

//     printf("Parsed %d tokens\n", tokenCount);

//     for (int i = 0; i < tokenCount; i++) {
//         printf("Token %d: type=%d, start=%d, end=%d, size=%d, value=%.*s\n", i, tokens[i].type,
//                tokens[i].start, tokens[i].end, tokens[i].size, tokens[i].end - tokens[i].start,
//                json + tokens[i].start);
//     }

//     return 0;
// }

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
        HttpPageMapping mapping = {0};
        findHttpPageMapping(&mapping, &request.requestLine);
        if (mapping.filePath) {
            sendFile(client_fd, mapping.filePath, mapping.type);
        }
        closesocket(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();

    return 0;
}

void mapJson(HttpMethod method, const char *path, void (*handler)(void)) {
    if (apiMappingsCount >= 1024) {
        printf("Too many mappings\n");
        return;
    }

    apiMappings[apiMappingsCount].method = method;
    apiMappings[apiMappingsCount].path = path;
    apiMappings[apiMappingsCount].handler = handler;

    apiMappingsCount++;
}

void mapPage(HttpMethod method, const char *urlPath, const char *filePath, const char *type) {
    if (pageMappingsCount >= 1024) {
        printf("Too many mappings\n");
        return;
    }

    pageMappings[pageMappingsCount].method = method;
    pageMappings[pageMappingsCount].urlPath = urlPath;
    pageMappings[pageMappingsCount].filePath = filePath;
    pageMappings[pageMappingsCount].type = type;

    pageMappingsCount++;
}

int findHttpApiMapping(HttpApiMapping *mapping, HttpRequestLine *req) {
    for (int i = 0; i < apiMappingsCount; i++) {
        HttpApiMapping *curr = &apiMappings[i];

        if (curr->method == req->method && strcmp(curr->path, req->path) == 0) {
            // printf("found a mapping for %s %s\n", httpMethodToString(req->method), req->path);
            *mapping = *curr;
            return 0;
        }
    }

    printf("did not find an api mapping for %s %s\n", httpMethodToString(req->method), req->path);
    return -1;
}

int findHttpPageMapping(HttpPageMapping *mapping, HttpRequestLine *req) {
    for (int i = 0; i < pageMappingsCount; i++) {
        HttpPageMapping *curr = &pageMappings[i];
        if (curr->method == req->method && strcmp(curr->urlPath, req->path) == 0) {
            // printf("found a mapping for %s %s\n", httpMethodToString(req->method), req->path);
            *mapping = *curr;
            return 0;
        }
    }

    printf("did not find a page mapping for %s %s\n", httpMethodToString(req->method), req->path);
    return -1;
}

void makeMappings() {
    mapJson(HTTP_GET, "/", handleGetRoot);
    mapJson(HTTP_GET, "/users", handleGetUsers);
    mapPage(HTTP_GET, "/hello", "public/hello.html", "text/html");
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

int sendFile(SOCKET client_fd, const char *filePath, const char *contentType) {
    FILE *file = fopen(filePath, "rb");

    if (file == NULL) {
        const char *notFoundBody = "404 Not Found";

        char header[512];
        int headerLength = snprintf(header, sizeof(header),
                                    "HTTP/1.1 404 Not Found\r\n"
                                    "Content-Type: text/plain\r\n"
                                    "Content-Length: %d\r\n"
                                    "\r\n",
                                    (int)strlen(notFoundBody));

        send(client_fd, header, headerLength, 0);
        send(client_fd, notFoundBody, (int)strlen(notFoundBody), 0);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    if (fileSize < 0) {
        fclose(file);
        return -1;
    }

    char header[512];

    int headerLength = snprintf(header, sizeof(header),
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: %s\r\n"
                                "Content-Length: %ld\r\n"
                                "\r\n",
                                contentType, fileSize);

    send(client_fd, header, headerLength, 0);

    char buffer[4096];

    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_fd, buffer, (int)bytesRead, 0);
    }

    fclose(file);
    return 0;
}