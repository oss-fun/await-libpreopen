// test_preopenlite.c

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
        exit(1);
    }

    printf("Successfully opened file: %s\n", filename);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    if (bytes_read == -1) {
        fprintf(stderr, "Error reading file: %s\n", strerror(errno));
        close(fd);
        exit(1);
    }

    close(fd);
    return 0;
}
