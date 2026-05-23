#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define MAX_HEADERS 64
#define MAX_HEADER_NAME_LENGTH 128
#define MAX_HEADER_VALUE_LENGTH 1024

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

typedef struct {
    HttpMethod method;
    const char *path;
} HttpRequestLine;

typedef struct {
    char name[MAX_HEADER_NAME_LENGTH];
    char value[MAX_HEADER_VALUE_LENGTH];
} HttpHeader;

typedef struct {
    HttpRequestLine requestLine;
    HttpHeader headers[MAX_HEADERS];
    int headersCount;
    // HttpBody body
} HttpRequest;

int extractHttpRequestLine(HttpRequestLine *request, const char *incomingData, int size);
HttpMethod mapHttpMethodToEnum(const char *method);
int findHttpMapping(HttpMapping *mapping, HttpRequestLine *req);
void makeMappings();
void handleGetUsers();
void handleGetRoot();
const char *httpMethodToString(HttpMethod method);
int extractHeadersFromRequest(HttpHeader *headers, int *headersCount, const char *request);

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
        buffer[bytesReceived] = '\0';
        // printf("Request:\n%s", buffer);
        HttpHeader headers[1024] = {0};
        int headersCount = 0;
        extractHeadersFromRequest(headers, &headersCount, buffer);
        HttpRequestLine request;
        printf("HeadersCount:%d\n", headersCount);
        if (extractHttpRequestLine(&request, buffer, bytesReceived) != 0) {
            closesocket(client_fd);
            continue;
        }
        HttpMapping mapping;

        if (findHttpMapping(&mapping, &request) != 0) {
            closesocket(client_fd);
            continue;
        }

        mapping.handler();
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

int extractHttpRequestLine(HttpRequestLine *request, const char *incomingData, int size) {
    char *lineEnd = strstr(incomingData, "\r\n");

    if (lineEnd == NULL) {
        printf("Invalid HTTP request\n");
        return -1;
    }

    size_t lineLength = lineEnd - incomingData;

    char firstLine[1024];
    char method[10];
    char path[1024];

    if (lineLength >= sizeof(firstLine)) {
        printf("Request line too long\n");
        return -1;
    }

    memcpy(firstLine, incomingData, lineLength);
    firstLine[lineLength] = '\0';

    char *methodEnd = strchr(firstLine, ' ');

    if (methodEnd == NULL) {
        printf("Invalid request line: missing method separator\n");
        return -1;
    }

    size_t methodLength = methodEnd - firstLine;

    if (methodLength >= sizeof(method)) {
        printf("HTTP method too long\n");
        return -1;
    }

    memcpy(method, firstLine, methodLength);
    method[methodLength] = '\0';

    char *pathStart = methodEnd + 1;
    char *pathEnd = strchr(pathStart, ' ');

    if (pathEnd == NULL) {
        printf("Invalid request line: missing path separator\n");
        return -1;
    }

    size_t pathLength = pathEnd - pathStart;

    if (pathLength >= sizeof(path)) {
        printf("Path too long\n");
        return -1;
    }

    memcpy(path, pathStart, pathLength);
    path[pathLength] = '\0';

    // printf("First line: %s\n", firstLine);
    // printf("Http Method: %s\n", method);
    // printf("Path: %s\n", path);

    HttpMethod HttpMethod = mapHttpMethodToEnum(method);

    request->method = HttpMethod;
    request->path = path;
    return 0;
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

const char *httpMethodToString(HttpMethod method) {
    switch (method) {
    case HTTP_GET:
        return "GET";
    case HTTP_POST:
        return "POST";
    case HTTP_PATCH:
        return "PATCH";
    case HTTP_DELETE:
        return "DELETE";
    case HTTP_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

int extractHeadersFromRequest(HttpHeader *headers, int *headersCount, const char *request) {
    const char *requestLineEnd = strstr(request, "\r\n");

    if (requestLineEnd == NULL) {
        return -1;
    }

    const char *headersStart = requestLineEnd + 2;
    const char *headersEnd = strstr(headersStart, "\r\n\r\n");

    if (headersEnd == NULL) {
        return -1;
    }

    const char *currHeader = headersStart;
    int count = 0;

    while (count < MAX_HEADERS && currHeader < headersEnd) {
        const char *currHeaderEnd = strstr(currHeader, "\r\n");

        if (currHeaderEnd == NULL || currHeaderEnd > headersEnd) {
            return -1;
        }

        size_t lineLength = currHeaderEnd - currHeader;

        if (lineLength == 0) {
            break;
        }

        const char *colon = memchr(currHeader, ':', lineLength);

        if (colon == NULL) {
            printf("Invalid header line\n");
            return -1;
        }

        size_t nameLength = colon - currHeader;

        const char *valueStart = colon + 1;

        while (valueStart < currHeaderEnd && *valueStart == ' ') {
            valueStart++;
        }

        size_t valueLength = currHeaderEnd - valueStart;

        if (nameLength >= MAX_HEADER_NAME_LENGTH) {
            printf("Header name too long\n");
            return -1;
        }

        if (valueLength >= MAX_HEADER_VALUE_LENGTH) {
            printf("Header value too long\n");
            return -1;
        }

        memcpy(headers[count].name, currHeader, nameLength);
        headers[count].name[nameLength] = '\0';

        memcpy(headers[count].value, valueStart, valueLength);
        headers[count].value[valueLength] = '\0';

        printf("Header: %s = %s\n", headers[count].name, headers[count].value);

        count++;

        currHeader = currHeaderEnd + 2;
    }

    *headersCount = count;

    return 0;
}

void handleGetUsers() {
    // printf("Handling GET /users call\n");
}
void handleGetRoot() {
    //  printf("Handling GET / call\n");
}