#include <stdio.h>
#include <stdlib.h>

char base64_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* base64_encode(const char* plain, size_t len, size_t *outlen) {

    size_t counts = 0;
    unsigned char buf[3];
    char* cipher = malloc(len * 4 / 3 + 4);
    size_t i = 0, c = 0;

    for (i = 0 ; i < len ; i++) {
        buf[counts++] = plain[i];
        if (counts == 3) {
            cipher[c++] = base64_map[buf[0] >> 2];
            cipher[c++] = base64_map[((buf[0] & 0x03) << 4) | (buf[1] >> 4)];
            cipher[c++] = base64_map[((buf[1] & 0x0f) << 2) | (buf[2] >> 6)];
            cipher[c++] = base64_map[buf[2] & 0x3f];
            counts = 0;
        }
    }

    if (counts > 0) {
        cipher[c++] = base64_map[buf[0] >> 2];
        if (counts == 1) {
            cipher[c++] = base64_map[(buf[0] & 0x03) << 4];
            cipher[c++] = '=';
        } else {
            cipher[c++] = base64_map[((buf[0] & 0x03) << 4) | (buf[1] >> 4)];
            cipher[c++] = base64_map[(buf[1] & 0x0f) << 2];
        }
        cipher[c++] = '=';
    }

    cipher[c] = '\0';

    *outlen = c;
    return cipher;
}