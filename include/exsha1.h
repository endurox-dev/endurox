#ifndef EXSHA1_H
#define EXSHA1_H

/*
   SHA-1 in C
   By Steve Reid <steve@edmweb.com>
   100% Public Domain
 */

#include "stdint.h"

typedef struct
{
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} EXSHA1_CTX;

void EXSHA1Transform(
    uint32_t state[5],
    const unsigned char buffer[64]
    );

void EXSHA1Init(
    EXSHA1_CTX * context
    );

void EXSHA1Update(
    EXSHA1_CTX * context,
    const unsigned char *data,
    uint32_t len
    );

void EXSHA1Final(
    unsigned char digest[20],
    EXSHA1_CTX * context
    );

void EXSHA1(
    char *hash_out,
    const char *str,
    int len);

#endif /* EXSHA1_H */
