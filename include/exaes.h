#ifndef _EXAES_H_
#define _EXAES_H_

#include <stdint.h>


// #define the macros below to 1/0 to enable/disable the mode of operation.
//
// CBC enables EXAES encryption in CBC-mode of operation.
// ECB enables the basic ECB 16-byte block algorithm. Both can be enabled simultaneously.

// The #ifndef-guard allows it to be configured before #include'ing or at compile time.
#ifndef CBC
  #define CBC 1
#endif

#ifndef ECB
  #define ECB 1
#endif

#define EXAES128 1
//#define EXAES192 1
//#define EXAES256 1

#if defined(ECB) && (ECB == 1)

void EXAES_ECB_encrypt(const uint8_t* input, const uint8_t* key, uint8_t *output, const uint32_t length);
void EXAES_ECB_decrypt(const uint8_t* input, const uint8_t* key, uint8_t *output, const uint32_t length);

#endif // #if defined(ECB) && (ECB == !)


#if defined(CBC) && (CBC == 1)

void EXAES_CBC_encrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
void EXAES_CBC_decrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);

#endif // #if defined(CBC) && (CBC == 1)


#endif //_EXAES_H_
