#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "sieve.h"
#include "base64.h"

#define CONN_BUF_SZ 4096

#define fatal(x) do{fprintf(stderr,(x));exit(1);}while(0);
#define pfatal(x) do{perror((x));exit(errno);}while(0);
#define min(x, y) ((x) > (y) ? (y) : (x))

struct sieve_conn_t {
    int sfd;
    char *buf;
    size_t bufidx;
    size_t bufc;
};

/* Obtain server descriptor */
sieve_conn_t sieve_connect(char *server, char *port) {
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

    sieve_conn_t conn = calloc(1, sizeof(*conn));
    conn->sfd = sfd;
    conn->bufidx = 0;
    conn->buf = calloc(1, CONN_BUF_SZ + 1);
    ++conn->buf;

    /* Read lines till OK */
    char *line;
    int did_get_ok = 0;
    while ((line = sieve_readline(conn))) {
        if (!strncmp(line, "OK", 2)) {
            did_get_ok = 1;
            break;
        }
        free(line);
    }
    if (line) free(line);

    if (!did_get_ok)
        fatal("Didn't get OK on initial connect\n");

    return conn;
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
    int to_print = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (to_print < 0) return -1;
    /* Mind that vsnprintf wont return bytes written but the bytes that it wanted to write */
    return safe_write(fd, buffer, (to_print > 4095 ? 4095 : to_print) * sizeof(char));
}

int vsafe_writef(int fd, char *format, va_list args) {
    char buffer[4096];
    int to_print = vsnprintf(buffer, sizeof(buffer), format, args);
    if (to_print < 0) return -1;
    /* Mind that vsnprintf wont return bytes written but the bytes that it wanted to write */
    return safe_write(fd, buffer, (to_print > 4095 ? 4095 : to_print) * sizeof(char));
}

void sieve_send(sieve_conn_t conn, char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsafe_writef(conn->sfd, format, args);
    va_end(args);
    if (ret == -1 || safe_write(conn->sfd, "\r\n", 2) == -1)
        pfatal("Error while sending command\n");
}

char *mystrcat(char *a, char *b) {
    if (!a) return strdup(b);
    char *r = malloc(strlen(a) + strlen(b) + 1);
    strcpy(r, a);
    strcat(r, b);
    return r;
}

char *find_crlf(char *s) {
    while ((s = strchr(s, '\n'))) {
        if (s[-1] == '\r')
            return s - 1;
    }
    return NULL;
}

char *sieve_readline(sieve_conn_t conn) {
    /* Check data in buffer */
    if (conn->bufc) {
        char *crlf = find_crlf(conn->buf + conn->bufidx);
        if (crlf) {
            /* We already have a new line :) */
            *crlf = '\0';
            char *res = strdup(conn->buf + conn->bufidx);
            size_t newidx = crlf - conn->buf + 2;
            conn->bufc -= newidx - conn->bufidx;
            conn->bufidx = newidx;
            return res;
        }

        /* Reset buffer */
        if (conn->bufc && conn->bufidx > 0)
            memmove(conn->buf, conn->buf + conn->bufidx, conn->bufc + 1);

    }

    /* Read till CRLF */
    char *res = NULL;
    ssize_t did_read = 0;
    readline_retry:;
    while ((did_read = read(conn->sfd, conn->buf + conn->bufc, CONN_BUF_SZ - conn->bufc - 1)) > 0) {
        conn->buf[conn->bufc + did_read] = '\0';
        char *crlf = find_crlf(conn->buf + conn->bufc);
        conn->bufc += did_read;
        if (crlf) {
            *crlf = '\0';
            res = mystrcat(res, conn->buf);
            conn->bufidx = crlf - conn->buf + 2;
            conn->bufc -= conn->bufidx;
            return res;
        }
        if (conn->bufc + 1 == CONN_BUF_SZ) {
            res = mystrcat(res, conn->buf);
            *conn->buf = '\0';
            conn->bufc = 0;
        }
    }

    /* Probably bad :( */
    if (did_read < 0) { 
        if (errno == EINTR) goto readline_retry;
        pfatal("Error while trying to read line");
    }
    if (res) free(res);
    return NULL;
}

int sieve_auth(sieve_conn_t conn, char *user, char *auth, char *pass) {
    size_t plain_sz = strlen(user) + strlen(auth) + strlen(pass) + 2;
    char *plain = malloc(plain_sz + 1), *tmp = plain;
    strcpy(tmp, user);
    tmp += strlen(user) + 1;
    strcpy(tmp, auth);
    tmp += strlen(auth) + 1;
    strcpy(tmp, pass);
    size_t cipher_sz;
    char *cipher = base64_encode(plain, plain_sz, &cipher_sz);
    sieve_send(conn, "AUTHENTICATE \"PLAIN\" \"%s\"", cipher);
    char *res = sieve_readline(conn);
    int r = strncmp(res, "OK", 2);
    if (r) fprintf(stderr, "%s\n", res);
    free(res);
    return r;
}

char *sieve_getscript(sieve_conn_t conn, char *name) {
    sieve_send(conn, "GETSCRIPT \"%s\"", name);
    char *res = sieve_readline(conn);
    if (!res || !strncmp(res, "NO", 2)) {
        fprintf(stderr, "%s\n", res);
        free(res);
        return NULL;
    }
    size_t to_read;
    if (sscanf(res, "{%zu}", &to_read) != 1)
        fatal("Failed to read script size\n");
    free(res);

    char *script = malloc(to_read + 1);
    
    /* Check for buffer */
    size_t had_before = min(conn->bufc, to_read);
    if (had_before) {
        memcpy(script, conn->buf + conn->bufidx, had_before);
        conn->bufc -= had_before;
        conn->bufidx += had_before;
    }
    script[had_before] = '\0';

    /* Read rest */
    if (to_read > had_before)
        if (safe_read(conn->sfd, script + had_before, to_read - had_before))
            pfatal("Error while reading script");

    script[to_read] = '\0';
    printf("%s\n", script);
    res = sieve_readline(conn);
    res = sieve_readline(conn);
    if (strncmp(res, "OK", 2)) fatal("Impossible value after GETSCRIPT\n");
    free(res);
    return script;
}

void sieve_dumplines(sieve_conn_t conn) {
    char *line;
    while ((line = sieve_readline(conn))) {
        printf("%s\n", line);
        free(line);
    }
    if (line) free(line);
}