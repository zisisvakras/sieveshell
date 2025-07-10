#ifndef WRAPS_H_
#define WRAPS_H_

#include <unistd.h>

/* Obtain server descriptor */
int connect_tcp(char *server, char *port);

/**
 *  Safely reads from fd and stores it in buffer.
 *  Returns 0 on success.
 *  Returns -1 on error and sets errno.
 *  Returns -2 on mid-read EOF.
 */
ssize_t safe_read(int fd, void *buffer, size_t size);

/**
 *  Safely writes to fd from buffer.
 *  Returns size on success.
 *  Returns -1 on error and sets errno.
 */
ssize_t safe_write(int fd, const void *buffer, size_t size);

/**
 *  Safely writes to fd a formatted string.
 *  Returns 0 on success.
 *  Returns -1 on error and sets errno.
 */
ssize_t safe_writef(int fd, char *format, ...);

#endif // WRAPS_H_