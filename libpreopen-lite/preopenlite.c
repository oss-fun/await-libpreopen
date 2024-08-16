#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#define MAX_PREOPENED_DIRS 10
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)

static int preopened_fds[MAX_PREOPENED_DIRS] = {-1};
static char *preopened_paths[MAX_PREOPENED_DIRS] = {NULL};
static int preopened_count = 0;

static void __attribute__((constructor)) init_preopenlite(void) {
    DEBUG_PRINT("Initializing preopenlite");
    char *preopen_path = getenv("PREOPEN_PATH");
    if (preopen_path == NULL) {
        DEBUG_PRINT("PREOPEN_PATH is not set");
        return;
    }

    DEBUG_PRINT("PREOPEN_PATH: %s", preopen_path);
    char *path = strdup(preopen_path);
    if (!path) {
        DEBUG_PRINT("Failed to allocate memory for PREOPEN_PATH");
        return;
    }

    char *saveptr;
    char *token = strtok_r(path, ":", &saveptr);

    while (token != NULL && preopened_count < MAX_PREOPENED_DIRS) {
        DEBUG_PRINT("Opening directory: %s", token);
        int fd = open(token, O_DIRECTORY | O_RDONLY);
        if (fd != -1) {
            preopened_fds[preopened_count] = fd;
            preopened_paths[preopened_count] = strdup(token);
            if (!preopened_paths[preopened_count]) {
                DEBUG_PRINT("Failed to allocate memory for path: %s", token);
                close(fd);
            } else {
                DEBUG_PRINT("Opened directory %s with fd %d", token, fd);
                preopened_count++;
            }
        } else {
            DEBUG_PRINT("Failed to open directory %s: %s", token, strerror(errno));
        }
        token = strtok_r(NULL, ":", &saveptr);
    }

    free(path);
    DEBUG_PRINT("Initialization complete. Preopened %d directories", preopened_count);
}

static int find_preopened_fd(const char *path) {
    if (!path) {
        DEBUG_PRINT("find_preopened_fd called with NULL path");
        return -1;
    }
    for (int i = 0; i < preopened_count; i++) {
        if (preopened_paths[i] && strncmp(path, preopened_paths[i], strlen(preopened_paths[i])) == 0) {
            DEBUG_PRINT("Found preopened fd %d for path %s", preopened_fds[i], path);
            return preopened_fds[i];
        }
    }
    DEBUG_PRINT("No preopened fd found for path %s", path);
    return -1;
}

int open(const char *pathname, int flags, ...) {
    DEBUG_PRINT("open called with pathname: %s", pathname ? pathname : "NULL");
    
    static int (*real_open)(const char *pathname, int flags, ...) = NULL;
    mode_t mode = 0;

    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    if (!real_open) {
        DEBUG_PRINT("Loading real open function");
        real_open = dlsym(RTLD_NEXT, "open");
        if (!real_open) {
            DEBUG_PRINT("Error loading real open function: %s", dlerror());
            exit(1);
        }
    }

    int dirfd = find_preopened_fd(pathname);
    if (dirfd != -1) {
        const char *relative_path = pathname;
        for (int i = 0; i < preopened_count; i++) {
            if (preopened_fds[i] == dirfd) {
                size_t prefix_len = strlen(preopened_paths[i]);
                if (strncmp(pathname, preopened_paths[i], prefix_len) == 0) {
                    relative_path = pathname + prefix_len;
                    if (*relative_path == '/') {
                        relative_path++;  // Skip the leading '/' in the relative path
                    }
                    break;
                }
            }
        }
        DEBUG_PRINT("Using openat with dirfd %d and relative path %s", dirfd, relative_path);
        int result = openat(dirfd, relative_path, flags, mode);
        if (result == -1) {
            DEBUG_PRINT("openat failed: %s", strerror(errno));
        } else {
            DEBUG_PRINT("openat succeeded with fd %d", result);
        }
        return result;
    }

    DEBUG_PRINT("Using real open for path %s", pathname);
    return real_open(pathname, flags, mode);
}
