#ifndef BASE64_H_
#define BASE64_H_

#include <stddef.h>

char* base64_encode(const char* plain, size_t len, size_t *outlen);

#endif // BASE64_H_