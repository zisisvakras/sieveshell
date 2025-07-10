#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

/* Obtain server descriptor */
int connect_tcp(char *server, char *port) {
    struct addrinfo* infop = NULL;
    struct addrinfo hint = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };
    /* Resolve server name */
    int err_ret = getaddrinfo(server, port, &hint, &infop);
    if (err_ret != 0) {
        const char *err_str = gai_strerror(err_ret);
        fprintf(stderr, "Error connecting to server: %s\n", err_str);
        exit(err_ret);
    }
    /* Try to connect to results from getaddrinfo */
    int sfd;
    for (struct addrinfo * i = infop ; i != NULL ; i = i->ai_next) {
        sfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (sfd == -1 || connect(sfd, i->ai_addr, i->ai_addrlen)) {
            if (i->ai_next == NULL) { /* Last address so exit */
                perror("Error connecting to server");
                exit(errno);
            }
            close(sfd);
            continue;
        }
        break;
    }
    freeaddrinfo(infop);
    return sfd;
}


/**
 *  Safely reads from fd and stores it in buffer.
 *  Returns 0 on success.
 *  Returns -1 on error and sets errno.
 *  Returns -2 on mid-read EOF.
 */
ssize_t safe_read(int fd, void *buffer, size_t size) {
    char *ptr = buffer;
    size_t been_read = 0;
    while (size > been_read) {
        ssize_t curr = read(fd, ptr + been_read, size - been_read);
        if (curr == -1) return -1;
        if (curr == 0) {
            return been_read ? -2 : 0;
        }
        been_read += curr;
    }
    return 0;
}

/**
 *  Safely writes to fd from buffer.
 *  Returns size on success.
 *  Returns -1 on error and sets errno.
 */
ssize_t safe_write(int fd, const void *buffer, size_t size) {
    const char *ptr = buffer;
    size_t been_send = 0;
    while (size > been_send) { /* While not at buffer end */
        int curr = write(fd, ptr + been_send, size - been_send);
        if (curr == -1) return -1;
        been_send += curr;
    }
    return 0;
}

/**
 *  Safely writes to fd a formatted string.
 *  Returns 0 on success.
 *  Returns -1 on error and sets errno.
 */
int safe_writef(int fd, char *format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    /* Format string using libc (no syscalls used here) */
    int to_print = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (to_print < 0) return -1;
    /* Mind that vsnprintf wont return bytes written but the bytes that it wanted to write */
    return safe_write(fd, buffer, (to_print > 4095 ? 4095 : to_print) * sizeof(char));
}