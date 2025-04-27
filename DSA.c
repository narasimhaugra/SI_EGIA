#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup DSA
 * \{
 *
 * \brief   DSA functions
 *
 * \details 
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    DSA.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/

/* Copyright 2014, Kenneth MacKay. Licensed under the BSD 2-clause license. */
#include "Common.h"
#include "uECC.h"
#include "SHA.h"
#include "DSA.h"  
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER  (LOG_GROUP_FILE_SYS) ///< Log Group Identifier
#define SHA256_BLOCK_LENGTH   (64u)
#define SHA256_DIGEST_LENGTH  (32u)
#define SIZE_RANDOM_NUMBER    (8u)
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct SHA256_HashContext 
{
    uECC_HashContext uECC;
    SHA256_CTX Ctx;
} SHA256_HashContext;
/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   init_SHA256
 *
 * \details ECC Specific init hash function
 *
 * \param   
 *
 * \return  None
 *
 * ========================================================================== */
static void init_SHA256(const uECC_HashContext *Base) 
{
    SHA256_HashContext *Context = (SHA256_HashContext *)Base;
    sha256_init(&Context->Ctx);
}

/* ========================================================================== */
/**
 * \brief   update_SHA256
 *
 * \details ECC Specific update hash function
 *
 * \param   
 *
 * \return  None
 *
 * ========================================================================== */

static void update_SHA256(const uECC_HashContext *Base,
                          const uint8_t *Message,
                          unsigned message_size) 
{
    SHA256_HashContext *Context = (SHA256_HashContext *)Base;
    sha256_update(&Context->Ctx, Message, message_size);
}
/* ========================================================================== */
/**
 * \brief   finish_SHA256
 *
 * \details ECC Specific finish hash function
 *
 * \param   Base: hash context
 *          HashResult: pointer to hash output
 *
 * \return  None
 *
 * ========================================================================== */
static void finish_SHA256(const uECC_HashContext *Base, uint8_t *HashResult) 
{
    SHA256_HashContext *Context = (SHA256_HashContext *)Base;
    sha256_final(&Context->Ctx, HashResult);
}
/* ========================================================================== */
/**
 * \brief   Print
 *
 * \details Function to print hex array
 *
 * \param   pStr: data name
 *          pData: pointer todata
 *
 * \return  None
 *
 * ========================================================================== */
void Print(const uint8_t *pStr, uint8_t *pData,int len)
{
    Log(DBG,"%s :",pStr);
    for(int count=0;count<len;count++)
    {
       printf("%02X ", pData[count]);
      // Log(DBG," %x",pData[count]);
    }
}
/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Random Number Generator
 *
 * \details Use this function to generate random number 
 *
 * \param   pDest : Pointer to the destination byte addressable  buffer
 *          ByteCount: number of bytes of random number required
 *
 * \return  None
 *
 * ========================================================================== */
int RandomNmGenerator(uint8_t *pDest, unsigned ByteCount)
{
    int Count;
    uint32_t RandomNumber[SIZE_RANDOM_NUMBER];

    for( Count=0; Count<(ByteCount/4); Count++)
    {
         RandomNumber[Count] = rand();
    } 

    memcpy(pDest,(uint8_t*)RandomNumber,ByteCount); 
    //Print("Random Number", pDest,ByteCount);

    return 1;
}
/* ========================================================================== */
/**
 * \brief   Get Message Hash
 *
 * \details Calculate the 32 byte Hash of the message
 *
 * \param   pMessage:   pointer to the original message
 *          MessageLen: length of the message
 *
 * \return  None
 *
 * ========================================================================== */
void GetMessageHash(uint8_t *pMessage, uint32_t MessageLen,uint8_t *pHash)
{
    SHA256_CTX ShaContxt;    
    
    sha256_init (&ShaContxt);
    sha256_update(&ShaContxt, pMessage, MessageLen);
    sha256_final(&ShaContxt, pHash);        
    //Print("Hash",pHash,SHA256_HASH_SIZE);
} 
/* ========================================================================== */
/**
 * \brief   ComputeDigitalSignature
 *
 * \details This function computes signature for the message passed.
 *          The signature generature process begins with generating a 32 byte hash for
 *          the message . Public and private keys are calculated using a random number
 *          generator as seed. The last part is optional which helps in verifying the 
 *          generated signature on the handle itself.
 *
 * \param   
 *
 * \return  None
 *
 * ========================================================================== */
int ComputeDigitalSignature(uint8_t *Message, uint16_t Count)
{    
    const struct uECC_Curve_t *Curve;
    uint8_t Hash[SHA256_HASH_SIZE]; 
    uint8_t PrivateKey[PRIVATE_KEY_SIZE] = {0};
    uint8_t PublicKey[PUBLIC_KEY_SIZE] = {0};    
    uint8_t Signature[SIGNATURE_SIZE] = {0};           
    uint8_t Buff[2 * SHA256_DIGEST_LENGTH + SHA256_BLOCK_LENGTH];
      
    SHA256_HashContext Ctx = 
    {
        {
            &init_SHA256,
            &update_SHA256,
            &finish_SHA256,
            SHA256_BLOCK_LENGTH,
            SHA256_DIGEST_LENGTH,
            Buff
        }
    };    
    
    Curve = uECC_secp256k1();
    
    GetMessageHash(Message, Count, Hash);        
    uECC_set_rng(&RandomNmGenerator);    
    
    do
    {
        if (!uECC_make_key(PublicKey, PrivateKey, Curve)) 
        {
            Log(DBG, "uECC_make_key() failed\n");           
            break;
        }        
        if (!uECC_sign_deterministic(PrivateKey, Hash, sizeof(Hash), &Ctx.uECC, Signature, Curve)) 
        {
            Log(DBG,"uECC_sign() failed\n");
            break;
        }        
        memcpy(&Message[Count],PublicKey, PUBLIC_KEY_SIZE);
        memcpy(&Message[Count+PUBLIC_KEY_SIZE],Signature, SIGNATURE_SIZE);
        
        if (!uECC_verify(PublicKey, Hash, sizeof(Hash), Signature, Curve)) 
        {
            Log(DBG,"uECC_verify() failed\n");
            break;
        }
        Log(DBG,"Signature Verified Successfuly..!");
        
    } while(false);
    
    return 0;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif