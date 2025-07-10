#include <stdio.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sieve.h"

#define usage() do {fprintf(stderr,\
                "Usage: %s -u username -p password [-a authname] -h hostname -P port\n",\
                argv[0]); exit(1);} while(0);

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
    


    /* Get active script */
    sieve_conn_t conn = sieve_connect(host, port);
    
    if (sieve_auth(conn, user, auth, pass)) {
        fprintf(stderr, "Couldn't authenticate to server\n");
        return 1;
    }
    
    char *script = sieve_getscript(conn, "phpscript");
    if (!script) {
        fprintf(stderr, "Couldn't get script\n");
        return 1;
    }

    return 0;
}
