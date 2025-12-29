#include <fcgi_stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/*
 * From https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
 * This function is to get filename extension.
 */
const char *FileSuffix(const char path[]) {
    const char *result;
    int i, n;

    if (path == NULL) {
        return NULL;
    }

    n = strlen(path);
    i = n - 1;
    while ((i > 0) && (path[i] != '.') && (path[i] != '/') &&
           (path[i] != '\\')) {
        i--;
    }

    if ((i > 0) && (i < n - 1) && (path[i] == '.') && (path[i - 1] != '/') &&
        (path[i - 1] != '\\')) {
        result = path + i;
    } else {
        result = path + n;
    }

    return result;
}

void get_timestamp(char *buffer, size_t buffer_size) {
    time_t timer;
    struct tm *tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void LogMessage(const char *format, ...) {
    char timestamp[26];
    get_timestamp(timestamp, sizeof(timestamp));

    fprintf(stderr, "[%s] ", timestamp);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}
