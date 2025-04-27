#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// #define the macros below to 1/0 to enable/disable the mode of operation.
//
// CBC enables AES encryption in CBC-mode of operation.
// CTR enables encryption in counter-mode.
// ECB enables the basic ECB 16-byte block algorithm. All can be enabled simultaneously.

// The #ifndef-guard allows it to be configured before #include'ing or at compile time.
#ifndef CBC
  #define CBC (1u)
#endif

#ifndef ECB
  #define ECB (1u)
#endif

#ifndef CTR
  #define CTR (1u)
#endif

#define ENCRYPT   (1u)
#define DECRYPT   (2u)

#define AES128    (1u)
//#define AES192 1
//#define AES256 1

#define AES_BLOCKLEN 16 // Block length in bytes - AES is 128 bit block only
#define IV_OFFSET 16 // IV Offset

#if defined(AES256) && (AES256 == 1)
    #define AES_KEYLEN 32
    #define AES_KEYEXPSIZE 240
#elif defined(AES192) && (AES192 == 1)
    #define AES_KEYLEN 24
    #define AES_KEYEXPSIZE 208
#else
    #define AES_KEYLEN 16   // Key length in bytes
    #define AES_KEYEXPSIZE 176
#endif

typedef enum
{
    AES_OPERATION_ENCRYPT=0,
    AES_OPERATION_DECRYPT,
    AES_OPERATION_COUNT

}AES_OPERATION;

typedef enum
{
   AES_KEY_ONE = 0,
   AES_KEY_TWO,
   AES_KEY_COUNT

} AES_KEY;

typedef struct
{
  uint8_t RoundKey[AES_KEYEXPSIZE];
#if (defined(CBC) && (CBC == 1)) || (defined(CTR) && (CTR == 1))
  uint8_t Iv[AES_BLOCKLEN];
#endif
} AES_CTX;


void AesInitCtx(AES_CTX *Ctx, const uint8_t *pKey);
#if (defined(CBC) && (CBC == 1)) || (defined(CTR) && (CTR == 1))
void AesInitCtxIv(AES_CTX *Ctx, const uint8_t *pKey, const uint8_t *pIv);
void AesCtxSetIv(AES_CTX *Ctx, const uint8_t *pIv);
#endif

#if defined(ECB) && (ECB == 1)
// buffer size is exactly AES_BLOCKLEN bytes; 
// you need only AesInitCtx as IV is not used in ECB 
// NB: ECB is considered insecure for most uses
void AesEcbEncrypt(const AES_CTX *Ctx, uint8_t *pBuf);
void AesEcbDecrypt(const AES_CTX *Ctx, uint8_t *pBuf);

#endif // #if defined(ECB) && (ECB == !)


#if defined(CBC) && (CBC == 1)
// buffer size MUST be multiple of AES_BLOCKLEN;
// Suggest https://en.wikipedia.org/wiki/Padding_(cryptography)#PKCS7 for padding scheme
// NOTES: you need to set IV in ctx via AesInitCtxIv() or AesCtxSetIv()
//        no IV should ever be reused with the same key 
void AesCbcEncryptBuffer(AES_CTX *Ctx, uint8_t* Buf, const size_t Length);
void AesCbcDecryptBuffer(AES_CTX *Ctx, uint8_t* Buf, const size_t Length);

#endif // #if defined(CBC) && (CBC == 1)


#if defined(CTR) && (CTR == 1)

// Same function for encrypting as for decrypting. 
// IV is incremented for every block, and used after encryption as XOR-compliment for output
// Suggesting https://en.wikipedia.org/wiki/Padding_(cryptography)#PKCS7 for padding scheme
// NOTES: you need to set IV in ctx with AesInitCtxIv() or AesCtxSetIv()
//        no IV should ever be reused with the same key 
void AesCtrXcryptBuffer(AES_CTX *Ctx, uint8_t* Buf, const size_t Length);

void DecryptBinaryBuffer(uint8_t* pBinary, const uint32_t Size, bool SetIV);
void ProcessPassphrase(uint8_t* pPhrase, AES_KEY key, AES_OPERATION AesOperation);


#endif // #if defined(CTR) && (CTR == 1)


#endif // _AES_H_
