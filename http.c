#include "http.h"
#include <stdio.h>
#include <string.h>

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
