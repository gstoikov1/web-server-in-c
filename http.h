#ifndef HTTP_H
#define HTTP_H

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

int extractHeadersFromRequest(HttpHeader *headers, int *headersCount, const char *request);
int extractHttpRequestLine(HttpRequestLine *request, const char *incomingData, int size);
HttpMethod mapHttpMethodToEnum(const char *method);
const char *httpMethodToString(HttpMethod method);

#endif