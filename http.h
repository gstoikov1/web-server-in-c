#ifndef HTTP_H
#define HTTP_H

#define MAX_HEADERS 64
#define MAX_HEADER_NAME_LENGTH 128
#define MAX_HEADER_VALUE_LENGTH 1024
#define MAX_BODY_LENGTH 4096

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
    char data[MAX_BODY_LENGTH];
    int size;
} HttpBody;

typedef struct {
    HttpRequestLine requestLine;
    HttpHeader headers[MAX_HEADERS];
    int headersCount;
    HttpBody body;
} HttpRequest;

int extractHeadersFromRequest(HttpHeader *headers, int *headersCount, const char *request);
int extractHttpRequestLine(HttpRequestLine *request, const char *incomingData);
int extractHttpBody(HttpBody *body, const char *request, int requestSize, int contentLength);
HttpMethod mapHttpMethodToEnum(const char *method);
const char *httpMethodToString(HttpMethod method);
const char *getHeaderValue(HttpHeader *headers, int headersCount, const char *name);
int extractHttpRequest(HttpRequest *request, const char *data, const int dataSize);

#endif