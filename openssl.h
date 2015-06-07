#ifndef _OPENSSL_H_
#define _OPENSSL_H_

#include <string.h>
/* Header files */
#include <openssl/des.h>
#include <openssl/sha.h>

/* Plain text of des key */
#define KEY "freesvr!@#"

int decrypt( const char *, char *, int, int* );
int encrypt( const char *, char *, int, int* );
char * g2u(char *inbuf);

#endif
