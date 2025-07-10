#ifndef WRAPS_H_
#define WRAPS_H_

#include <unistd.h>

typedef struct sieve_conn_t *sieve_conn_t;

/* Obtain server descriptor */
sieve_conn_t sieve_connect(char *server, char *port);

void sieve_send(sieve_conn_t conn, char *format, ...);

int sieve_auth(sieve_conn_t conn, char *user, char *auth, char *pass);

char *sieve_readline(sieve_conn_t conn);

void sieve_dumplines(sieve_conn_t conn);

char *sieve_getscript(sieve_conn_t conn, char *name);

#endif // WRAPS_H_