#ifndef _MD5DIGEST_H_
#define _MD5DIGEST_H_

#include "StrPtrLen.h"

enum {
    kHashHexLen =   32,
    kHashLen    =   16
};

// HashToString allocates memory for hashStr->Ptr 
void HashToString(unsigned char aHash[kHashLen], StrPtrLen* hashStr);

// allocates memory for hashA1Hex16Bit->Ptr                   
void CalcMD5HA1(StrPtrLen* userName, StrPtrLen* realm, StrPtrLen* userPassword, StrPtrLen* hashA1Hex16Bit);

// allocates memory to hA1->Ptr
void CalcHA1( StrPtrLen* algorithm, 
              StrPtrLen* userName, 
              StrPtrLen* realm,
              StrPtrLen* userPassword, 
              StrPtrLen* nonce, 
              StrPtrLen* cNonce,
              StrPtrLen* hA1
            );

// allocates memory to hA1->Ptr
void CalcHA1Md5Sess(StrPtrLen* hashA1Hex16Bit, StrPtrLen* nonce, StrPtrLen* cNonce, StrPtrLen* hA1);

// allocates memory for requestDigest->Ptr               
void CalcRequestDigest( StrPtrLen* hA1, 
                        StrPtrLen* nonce, 
                        StrPtrLen* nonceCount, 
                        StrPtrLen* cNonce,
                        StrPtrLen* qop,
                        StrPtrLen* method, 
                        StrPtrLen* digestUri, 
                        StrPtrLen* hEntity, 
                        StrPtrLen* requestDigest
                      );


void to64(register char *s, register long v, register int n);

void MD5Encode( char *pw, char *salt, char *result, int nbytes);

#endif
