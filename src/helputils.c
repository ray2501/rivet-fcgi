#include <string.h>

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
