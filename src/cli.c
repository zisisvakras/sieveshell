#include <stdio.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sieve.h"
#include "base64.h"

#define usage() do {fprintf(stderr,\
                "Usage: %s -u username -p password [-a authname] -h hostname -P port\n",\
                argv[0]); exit(1);} while(0);

/* Read until OK */
char *read_till_ok(int sfd) {
    static char buffer[8 << 20];
    size_t did_read = 0, have_read = 0;
    while ((did_read = read(sfd, buffer + have_read, sizeof(buffer))) > 0) {
        have_read += did_read;
        buffer[have_read + 1] = 0;
        if (!strncmp(buffer + have_read - 4, "OK\r\n", 4)) break;
    }
    if (did_read < 1) {
        fprintf(stderr, "failed to get OK\n");
        exit(1);
    }
    return buffer;
}

int main(int argc, char **argv) {
    char *user = NULL; /* User to log as */
    char *auth = NULL; /* Authenticating user */
    char *pass = NULL; /* Password for auth (if exists) or user */
    char *host = NULL; /* Hostname of sieve server */
    char *port = NULL; /* Port to connect */

    opterr = 0;

    int c;
    while ((c = getopt(argc, argv, "u:a:p:h:P:")) != -1) {
        switch (c) {
            case 'u':
                user = optarg;
                break;
            case 'a':
                auth = optarg;
                break;
            case 'p':
                pass = optarg;
                break;
            case 'h':
                host = optarg;
                break;
            case 'P':
                port = optarg;
                break;
            case '?':
                usage();
            default:
                abort();
        }
    }
    if (optind != argc) usage();
    if (!auth || !pass || !host || !port) usage();
    if (!user) user = "";
    
    size_t plain_sz = strlen(user) + strlen(auth) + strlen(pass) + 2;
    char *plain = malloc(plain_sz + 1), *tmp = plain;
    strcpy(tmp, user);
    tmp += strlen(user) + 1;
    strcpy(tmp, auth);
    tmp += strlen(auth) + 1;
    strcpy(tmp, pass);
    size_t cipher_sz;
    char *cipher = base64_encode(plain, plain_sz, &cipher_sz);

    /* Get active script */
    int sfd = connect_tcp(host, port);
    read_till_ok(sfd);
    safe_writef(sfd, "AUTHENTICATE \"PLAIN\" \"%s\"\r\n", cipher);
    read_till_ok(sfd);
    safe_writef(sfd, "GETSCRIPT \"phpscript\"\r\n");
    char *script = read_till_ok(sfd);
    script = strchr(script, '\n') + 1; /* Skip size */
    script[strlen(script) - 4] = '\0'; /* Skip OK */
    printf("%s", script);
    return 0;
}
