/*
https://github.com/kokke/tiny-AES-c

This is an implementation of the AES algorithm ECB, CTR and CBC modes.
The block size is chosen in Aes.h: AES128, AES192, or AES256.

The implementation is verified against the test vectors in:
National Institute of Standards and Technology (NIST)
Special Publication 800-38A 2001 ED ECB-AES128

Validate:
  Input bytes:
    6bc1bee22e409f96
    e93d7e117393172a
    ae2d8a571e03ac9c
    9eb76fac45af8e51
    30c81c46a35ce411
    e5fbc1191a0a52ef
    f69f2445df4f9b17
    ad2b417be66c3710

  128 bit key (AES128):
    2b7e151628aed2a6
	abf7158809cf4f3c

  Output bytes (cipher):
    3ad77bb40d7a3660
    a89ecaf32466ef97
    f5d3d58503b9699d
    e785895a96fdbaaf
    43b1cd7f598ece23
    881b00e3ed030688
    7b0c785e27e8ad3f
    8223207104725dd4

The input bytes must be an integer multiple of 16 bytes.
If not, pad the end of the data with zeros to reach the 16 byte boundary.
*/

/*****************************************************************************/
/* Includes:                                                                 */
/*****************************************************************************/
#include <string.h> // CBC mode, for memset
#include "Aes.h"

/*****************************************************************************/
/* Defines:                                                                  */
/*****************************************************************************/
// The number of columns in a state in AES. This is always 4 in AES.
#define STATE_COLUMNS 4

#if defined(AES256) && (AES256 == 1)
    #define KEY_WORDS 8
    #define CIPHER_ROUNDS 14
#elif defined(AES192) && (AES192 == 1)
    #define KEY_WORDS 6
    #define CIPHER_ROUNDS 12
#else
    #define KEY_WORDS 4        // The number of 32 bit words in a key.
    #define CIPHER_ROUNDS 10   // The number of rounds in AES Cipher.
#endif

// jcallan@github points out that declaring Multiply as a function 
// reduces code size considerably with the Keil ARM compiler.
// See this link for more information: https://github.com/kokke/tiny-AES-C/pull/3
#ifndef MULTIPLY_AS_A_FUNCTION
  #define MULTIPLY_AS_A_FUNCTION 1
#endif

const uint8_t Iv[16]  =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
const uint8_t EncryptionKey[16] =
{
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

const uint8_t AuthenticateKey1[AES_BLOCKLEN] = 
{
    0x1a, 0xb2, 0x9c, 0x4d, 0xef, 0x45, 0xa0, 0xa6,
    0xab, 0x93, 0x15, 0x88, 0x09, 0xcf, 0xef, 0xcc
};

const uint8_t AuthenticateKey2[AES_BLOCKLEN] = 
{
    0x2a, 0x32, 0x9c, 0x4d, 0x8f, 0xf5, 0xa0, 0xa6,
    0xcb, 0x93, 0x15, 0x88, 0x09, 0xdf, 0xef, 0xcc
};
/*****************************************************************************/
/* Private variables:                                                        */
/*****************************************************************************/
// state array for intermediate results during decryption.
typedef uint8_t state_t[4][4];


// The lookup tables are 'const' so they can be placed in flash instead of RAM.
// The numbers below can be computed dynamically, trading ROM for RAM.
static const uint8_t SBox[256] =
{
    //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

#if (defined(CBC) && CBC == 1) || (defined(ECB) && ECB == 1)
static const uint8_t RSBox[256] =
{
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};
#endif

// The round constant word array, RoundConstant[i], contains the values given by 
// x to the power (i-1) being powers of x (x is denoted as {02}) in the field GF(2^8)
static const uint8_t RoundConstant[11] =
{
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

/*
 * Jordan Goulder points out in PR #12 (https://github.com/kokke/tiny-AES-C/pull/12),
 * that you can remove most of the elements in the RoundConstant array, because they are unused.
 *
 * From Wikipedia's article on the Rijndael key schedule @ https://en.wikipedia.org/wiki/Rijndael_key_schedule#Rcon
 * 
 * "Only the first some of these constants are actually used – up to RoundConstant[10] for AES-128 (as 11 round keys are needed), 
 *  up to RoundConstant[8] for AES-192, up to RoundConstant[7] for AES-256. RoundConstant[0] is not used in AES algorithm."
 */


/*****************************************************************************/
/* Private functions:                                                        */
/*****************************************************************************/
/*
static uint8_t GetSBoxValue(uint8_t Number)
{
    return SBox[Number];
}
*/

#define GetSBoxValue(Number) (SBox[(Number)])

// This function produces STATE_COLUMNS(CIPHER_ROUNDS+1) round keys. The round keys are used in each round to decrypt the states.
static void KeyExpansion(uint8_t *pRoundKey, const uint8_t *pKey)
{
    unsigned Index1;
    unsigned j;
    unsigned k;

    uint8_t temp[4]; // Used for the column/row operations
  
    // The first round key is the key itself.
    for (Index1 = 0; Index1 < KEY_WORDS; Index1++)
    {
        pRoundKey[(Index1 * 4) + 0] = pKey[(Index1 * 4) + 0];
        pRoundKey[(Index1 * 4) + 1] = pKey[(Index1 * 4) + 1];
        pRoundKey[(Index1 * 4) + 2] = pKey[(Index1 * 4) + 2];
        pRoundKey[(Index1 * 4) + 3] = pKey[(Index1 * 4) + 3];
    }

    // All other round keys are found from the previous round keys.
    for (Index1 = KEY_WORDS; Index1 < STATE_COLUMNS * (CIPHER_ROUNDS + 1); Index1++)
    {
        k = (Index1 - 1) * 4;
        temp[0]=pRoundKey[k + 0];
        temp[1]=pRoundKey[k + 1];
        temp[2]=pRoundKey[k + 2];
        temp[3]=pRoundKey[k + 3];

        if (Index1 % KEY_WORDS == 0)
        {
            // circular shift the 4 bytes in a word to the left.
            // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]
            const uint8_t u8tmp = temp[0];  // !!!
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = u8tmp;

            // take a four-byte input word and apply the S-box to each of the four bytes to produce an output word.

            temp[0] = GetSBoxValue(temp[0]);
            temp[1] = GetSBoxValue(temp[1]);
            temp[2] = GetSBoxValue(temp[2]);
            temp[3] = GetSBoxValue(temp[3]);

            temp[0] = temp[0] ^ RoundConstant[Index1/KEY_WORDS];
        }
#if defined(AES256) && (AES256 == 1)
        if (4 == (Index1 % KEY_WORDS))
        {
            temp[0] = GetSBoxValue(temp[0]);
            temp[1] = GetSBoxValue(temp[1]);
            temp[2] = GetSBoxValue(temp[2]);
            temp[3] = GetSBoxValue(temp[3]);
        }
#endif
        j = Index1 * 4; k=(Index1 - KEY_WORDS) * 4;
        pRoundKey[j + 0] = pRoundKey[k + 0] ^ temp[0];
        pRoundKey[j + 1] = pRoundKey[k + 1] ^ temp[1];
        pRoundKey[j + 2] = pRoundKey[k + 2] ^ temp[2];
        pRoundKey[j + 3] = pRoundKey[k + 3] ^ temp[3];
    }
}


void AesInitCtx(AES_CTX *Ctx, const uint8_t *pKey)
{
    KeyExpansion(Ctx->RoundKey, pKey);
}


#if (defined(CBC) && (CBC == 1)) || (defined(CTR) && (CTR == 1))
void AesInitCtxIv(AES_CTX* Ctx, const uint8_t *pKey, const uint8_t *pIv)
{
    KeyExpansion(Ctx->RoundKey, pKey);
    memcpy (Ctx->Iv, pIv, AES_BLOCKLEN);
}

void AesCtxSetIv(AES_CTX *Ctx, const uint8_t *pIv)
{
    memcpy (Ctx->Iv, pIv, AES_BLOCKLEN);
}
#endif

// This function adds the round key to state by an XOR function.
static void AddRoundKey(uint8_t Round, state_t *pState, const uint8_t *pRoundKey)
{
    uint8_t Index1;
	uint8_t Index2;

    for (Index1 = 0; Index1 < 4; Index1++)
    {
        for (Index2 = 0; Index2 < 4; Index2++)
        {
            (*pState)[Index1][Index2] ^= pRoundKey[(Round * STATE_COLUMNS * 4) + (Index1 * STATE_COLUMNS) + Index2];
        }
    }
}

// The SubBytes Function Substitutes the values in the state matrix with values in an S-box.
static void SubBytes(state_t *pState)
{
    uint8_t Index1;
    uint8_t Index2;

    for (Index1 = 0; Index1 < 4; Index1++)
    {
        for (Index2 = 0; Index2 < 4; Index2++)
        {
            (*pState)[Index2][Index1] = GetSBoxValue((*pState)[Index2][Index1]);
        }
    }
}

// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
static void ShiftRows(state_t *pState)
{
    uint8_t temp;

    // Rotate first row 1 columns to left  
    temp            = (*pState)[0][1];
    (*pState)[0][1] = (*pState)[1][1];
    (*pState)[1][1] = (*pState)[2][1];
    (*pState)[2][1] = (*pState)[3][1];
    (*pState)[3][1] = temp;

    // Rotate second row 2 columns to left  
    temp            = (*pState)[0][2];
    (*pState)[0][2] = (*pState)[2][2];
    (*pState)[2][2] = temp;
    temp            = (*pState)[1][2];
    (*pState)[1][2] = (*pState)[3][2];
    (*pState)[3][2] = temp;

    // Rotate third row 3 columns to left
    temp            = (*pState)[0][3];
    (*pState)[0][3] = (*pState)[3][3];
    (*pState)[3][3] = (*pState)[2][3];
    (*pState)[2][3] = (*pState)[1][3];
    (*pState)[1][3] = temp;
}

static uint8_t xtime(uint8_t X)
{
    return ((X<<1) ^ (((X>>7) & 1) * 0x1b));  // !!!
}

// MixColumns function mixes the columns of the state matrix
static void MixColumns(state_t *pState)
{
    uint8_t Index1;
    uint8_t Tmp;
	uint8_t Tm;
	uint8_t t;

    for (Index1 = 0; Index1 < 4; Index1++)
    {
        t   = (*pState)[Index1][0];
        Tmp = (*pState)[Index1][0] ^ (*pState)[Index1][1] ^ (*pState)[Index1][2] ^ (*pState)[Index1][3];
        Tm  = (*pState)[Index1][0] ^ (*pState)[Index1][1];
        Tm = xtime(Tm);
        (*pState)[Index1][0] ^= Tm ^ Tmp;
        Tm  = (*pState)[Index1][1] ^ (*pState)[Index1][2];
        Tm = xtime(Tm);
        (*pState)[Index1][1] ^= Tm ^ Tmp ;
        Tm  = (*pState)[Index1][2] ^ (*pState)[Index1][3];
        Tm = xtime(Tm);
        (*pState)[Index1][2] ^= Tm ^ Tmp;
        Tm  = (*pState)[Index1][3] ^ t;
        Tm = xtime(Tm);
        (*pState)[Index1][3] ^= Tm ^ Tmp;
    }
}

// Multiply is used to multiply numbers in the field GF(2^8)
// Note: The last call to xtime() is unneeded, but often ends up generating a smaller binary
//       The compiler seems to be able to vectorize the operation better this way.
//       See https://github.com/kokke/tiny-AES-c/pull/34
#if MULTIPLY_AS_A_FUNCTION == 1
static uint8_t Multiply(uint8_t X, uint8_t Y)
{
    return (((Y & 1) * X) ^
           ((Y>>1 & 1) * xtime(X)) ^
           ((Y>>2 & 1) * xtime(xtime(X))) ^
           ((Y>>3 & 1) * xtime(xtime(xtime(X)))) ^
           ((Y>>4 & 1) * xtime(xtime(xtime(xtime(X)))))); /* this last call to xtime() can be omitted */
    }
#else
#define Multiply(X, Y)                                \
    (    ((Y & 1) * X) ^                              \
         ((Y>>1 & 1) * xtime(X)) ^                    \
         ((Y>>2 & 1) * xtime(xtime(X))) ^             \
         ((Y>>3 & 1) * xtime(xtime(xtime(X)))) ^      \
         ((Y>>4 & 1) * xtime(xtime(xtime(xtime(X))))) \
    )
#endif

#if (defined(CBC) && CBC == 1) || (defined(ECB) && ECB == 1)
/*
static uint8_t getSBoxInvert(uint8_t Number)
{
  return RSBox[Number];
}
*/
#define getSBoxInvert(num) (RSBox[(num)])

// MixColumns function mixes the columns of the state matrix.
// The method used to multiply may be difficult to understand for the inexperienced.
// Please use the references to gain more information.
static void InvMixColumns(state_t *pState)
{
    int Index;
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;

    for (Index = 0; Index < 4; Index++)
    {
        a = (*pState)[Index][0];
        b = (*pState)[Index][1];
        c = (*pState)[Index][2];
        d = (*pState)[Index][3];

        (*pState)[Index][0] = Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09);
        (*pState)[Index][1] = Multiply(a, 0x09) ^ Multiply(b, 0x0e) ^ Multiply(c, 0x0b) ^ Multiply(d, 0x0d);
        (*pState)[Index][2] = Multiply(a, 0x0d) ^ Multiply(b, 0x09) ^ Multiply(c, 0x0e) ^ Multiply(d, 0x0b);
        (*pState)[Index][3] = Multiply(a, 0x0b) ^ Multiply(b, 0x0d) ^ Multiply(c, 0x09) ^ Multiply(d, 0x0e);
    }
}


// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
static void InvSubBytes(state_t *pState)
{
    uint8_t Index1;
    uint8_t Index2;

    for (Index1 = 0; Index1 < 4; Index1++)
    {
        for (Index2 = 0; Index2 < 4; Index2++)
        {
            (*pState)[Index2][Index1] = getSBoxInvert((*pState)[Index2][Index1]);
        }
    }
}


static void InvShiftRows(state_t *pState)
{
    uint8_t temp;

    // Rotate first row 1 column to right  
    temp = (*pState)[3][1];

    (*pState)[3][1] = (*pState)[2][1];
    (*pState)[2][1] = (*pState)[1][1];
    (*pState)[1][1] = (*pState)[0][1];
    (*pState)[0][1] = temp;

    // Rotate second row 2 columns to right 
    temp = (*pState)[0][2];
    (*pState)[0][2] = (*pState)[2][2];
    (*pState)[2][2] = temp;

    temp = (*pState)[1][2];
    (*pState)[1][2] = (*pState)[3][2];
    (*pState)[3][2] = temp;

    // Rotate third row 3 columns to right
    temp = (*pState)[0][3];
    (*pState)[0][3] = (*pState)[1][3];
    (*pState)[1][3] = (*pState)[2][3];
    (*pState)[2][3] = (*pState)[3][3];
    (*pState)[3][3] = temp;
}
#endif // #if (defined(CBC) && CBC == 1) || (defined(ECB) && ECB == 1)


// Cipher is the main function that encrypts the PlainText.
static void Cipher(state_t *pState, const uint8_t* RoundKey)
{
    uint8_t Round;

    // Add the First round key to the state before starting the rounds.
    AddRoundKey(0, pState, RoundKey);

    // There will be CIPHER_ROUNDS rounds.
    // The first CIPHER_ROUNDS-1 rounds are identical.
    // These CIPHER_ROUNDS rounds are executed in the loop below.
    // Last one without MixColumns()
    for (Round = 1; ; Round++)
    {
        SubBytes(pState);
        ShiftRows(pState);
        if (CIPHER_ROUNDS == Round)
		{
            break;
        }
        MixColumns(pState);
        AddRoundKey(Round, pState, RoundKey);
    }

    // Add round key to last round
    AddRoundKey(CIPHER_ROUNDS, pState, RoundKey);
}

#if (defined(CBC) && CBC == 1) || (defined(ECB) && ECB == 1)
static void InvCipher(state_t *pState, const uint8_t* RoundKey)
{
    uint8_t Round;

    // Add the First round key to the state before starting the rounds.
    AddRoundKey(CIPHER_ROUNDS, pState, RoundKey);

    // There will be CIPHER_ROUNDS rounds.
    // The first CIPHER_ROUNDS-1 rounds are identical.
    // These CIPHER_ROUNDS rounds are executed in the loop below.
    // Last round without InvMixColumn()
    for (Round = (CIPHER_ROUNDS - 1); ; Round--)
    {
        InvShiftRows(pState);
        InvSubBytes(pState);
        AddRoundKey(Round, pState, RoundKey);
        if (0 == Round)
        {
            break;
        }
        InvMixColumns(pState);
    }
}
#endif // #if (defined(CBC) && CBC == 1) || (defined(ECB) && ECB == 1)


/*****************************************************************************/
/* Public functions:                                                         */
/*****************************************************************************/
#if defined(ECB) && (ECB == 1)

void AesEcbEncrypt(const AES_CTX *Ctx, uint8_t *pBuffer)
{
    // Encrypt the PlainText with the Key using AES algorithm.
    Cipher((state_t*)pBuffer, Ctx->RoundKey);
}

void AesEcbDecrypt(const AES_CTX *Ctx, uint8_t* pBuffer)
{
    // Decrypt the PlainText with the Key using AES algorithm.
    InvCipher((state_t*)pBuffer, Ctx->RoundKey);
}

#endif // #if defined(ECB) && (ECB == 1)


#if defined(CBC) && (CBC == 1)

static void XorWithIv(uint8_t *pBuffer, const uint8_t *Iv)
{
    uint8_t Index;

    for (Index = 0; Index < AES_BLOCKLEN; Index++) // The block in AES is always 128bit no matter the key size
    {
        pBuffer[Index] ^= Iv[Index];
    }
}

void AesCbcEncryptBuffer(AES_CTX *Ctx, uint8_t *pBuffer, const size_t Length)
{
    size_t i;
    uint8_t *Iv = Ctx->Iv;
    for (i = 0; i < Length; i += AES_BLOCKLEN)
    {
        XorWithIv(pBuffer, Iv);
        Cipher((state_t*)pBuffer, Ctx->RoundKey);
        Iv = pBuffer;
        pBuffer += AES_BLOCKLEN;
    }

    /* store Iv in Ctx for next call */
    memcpy(Ctx->Iv, Iv, AES_BLOCKLEN);
}

void AesCbcDecryptBuffer(AES_CTX *Ctx, uint8_t *pBuffer, const size_t Length)
{
    size_t Index;
    uint8_t StoreNextIv[AES_BLOCKLEN];

    for (Index = 0; Index < Length; Index += AES_BLOCKLEN)
    {
        memcpy(StoreNextIv, pBuffer, AES_BLOCKLEN);
        InvCipher((state_t*)pBuffer, Ctx->RoundKey);
        XorWithIv(pBuffer, Ctx->Iv);
        memcpy(Ctx->Iv, StoreNextIv, AES_BLOCKLEN);
        pBuffer += AES_BLOCKLEN;
    }
}

#endif // #if defined(CBC) && (CBC == 1)


#if defined(CTR) && (CTR == 1)

/* Symmetrical operation: same function for encrypting as for decrypting. Note any IV/nonce should never be reused with the same key */
void AesCtrXcryptBuffer(AES_CTX *Ctx, uint8_t *pBuffer, const size_t Length)
{
    uint8_t Buffer[AES_BLOCKLEN];
    size_t Index;
    int BlockIndex;

    for (Index = 0, BlockIndex = AES_BLOCKLEN; Index < Length; Index++, BlockIndex++)
    {
        if (BlockIndex == AES_BLOCKLEN) /* we need to regen xor compliment in buffer */
        {
            memcpy(Buffer, Ctx->Iv, AES_BLOCKLEN);
            Cipher((state_t*)Buffer,Ctx->RoundKey);

            /* Increment Iv and handle overflow */
            for (BlockIndex = (AES_BLOCKLEN - 1); BlockIndex >= 0; BlockIndex--)
            {
                /* inc will overflow */
                if (Ctx->Iv[BlockIndex] == 255)
                {
                    Ctx->Iv[BlockIndex] = 0;
                    continue;
                }
                Ctx->Iv[BlockIndex] += 1;
                break;
            }
            BlockIndex = 0;
        }
        pBuffer[Index] = (pBuffer[Index] ^ Buffer[BlockIndex]);
    }
}

#endif // #if defined(CTR) && (CTR == 1)

/* ========================================================================== */
/**
 * \brief   Decrypt Binary Buffer
 *
 * \details This function is used to decrypt the passed binary data 
 *          using CBC algorithm
 *           
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void DecryptBinaryBuffer(uint8_t* pBinary, const uint32_t Size, bool SetIV)
{
    static  AES_CTX Ctx;    
    uint8_t  paddingRequired;    
    uint8_t  DecryptIV[AES_BLOCKLEN];
        
    do 
    {
        if( ( NULL == Size ) || ( NULL == pBinary ) )  
        {
           break;
        }
        paddingRequired = Size % AES_BLOCKLEN;
        
        
        if ( paddingRequired )
        {
            paddingRequired =  AES_BLOCKLEN - paddingRequired;           
        }
        
        if( SetIV )//doing a random read, set IV first
        {
           memcpy(DecryptIV,pBinary,AES_BLOCKLEN); 
           pBinary = &pBinary[AES_BLOCKLEN];           
        }
        else
        {
           memcpy(DecryptIV,Iv,AES_BLOCKLEN);
        }
        
        AesInitCtxIv(&Ctx, EncryptionKey, DecryptIV);        
                       
        AesCbcDecryptBuffer(&Ctx, pBinary, ( Size + paddingRequired ));
        
    }while (false);
}

void ProcessPassphrase(uint8_t* pPhrase, AES_KEY key, AES_OPERATION AesOperation)
{
    static AES_CTX Ctx;     
    static uint8_t *AuthenticateKey;

    AuthenticateKey = NULL;
    
    if( AES_KEY_ONE == key )
    {
        AuthenticateKey = (uint8_t*)AuthenticateKey1;
    }
    else
    {
        AuthenticateKey = (uint8_t*)AuthenticateKey2;
    }
    
    AesInitCtxIv(&Ctx, AuthenticateKey, Iv);
    
    if ( AES_OPERATION_ENCRYPT == AesOperation )
    {
        AesCbcEncryptBuffer(&Ctx, pPhrase, AES_BLOCKLEN);  
    }
    else
    {
        AesCbcDecryptBuffer(&Ctx, pPhrase, AES_BLOCKLEN);
    }    
}


