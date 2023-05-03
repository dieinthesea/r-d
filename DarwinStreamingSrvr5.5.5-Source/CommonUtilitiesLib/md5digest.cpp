#include "md5.h"
#include "md5digest.h"
#include "StrPtrLen.h"
#include <string.h>
#include "OSMemory.h"

static StrPtrLen sColon(":", 1);
static StrPtrLen sMD5Sess("md5-sess", 8);
static StrPtrLen sQopAuth("auth", 4);
static StrPtrLen sQopAuthInt("auth-int", 8);

// allocates memory for hashStr->Ptr
void HashToString(unsigned char aHash[kHashLen], StrPtrLen* hashStr){
    UInt16 i;
    UInt8 hexDigit;
    char* str = NEW char[kHashHexLen+1];
    str[kHashHexLen] = 0;
        
    for(i = 0; i < kHashLen; i++) {
        hexDigit = (aHash[i] >> 4) & 0xF;
        str[i*2] = (hexDigit <= 9) ? (hexDigit + '0') : (hexDigit + 'a' - 10);
        hexDigit = aHash[i] & 0xF;
        str[i*2 + 1] = (hexDigit <= 9) ? (hexDigit + '0') : (hexDigit + 'a' - 10);
    }
    
    hashStr->Ptr = str;
    hashStr->Len = kHashHexLen;
}

// allocates memory for hashA1Hex16Bit->Ptr
void CalcMD5HA1( StrPtrLen* userName, 
                 StrPtrLen* realm,
                 StrPtrLen* userPassword, 
                 StrPtrLen* hashA1Hex16Bit
               ) 
{
    Assert(userName);
    Assert(realm);
    Assert(userPassword);
    Assert(hashA1Hex16Bit);
    Assert(hashA1Hex16Bit->Ptr == NULL); //This is the result. A Ptr here will be replaced. Value should be NULL.
    
    MD5_CTX context;
    unsigned char* aHash = NEW unsigned char[kHashLen];
    
    MD5_Init(&context);
    MD5_Update(&context, (unsigned char *)userName->Ptr, userName->Len);
    MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    MD5_Update(&context, (unsigned char *)realm->Ptr, realm->Len);
    MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    MD5_Update(&context, (unsigned char *)userPassword->Ptr, userPassword->Len);
    MD5_Final(aHash, &context);
    hashA1Hex16Bit->Ptr = (char *)aHash;
    hashA1Hex16Bit->Len = kHashLen;
}

void CalcHA1( StrPtrLen* algorithm, 
              StrPtrLen* userName, 
              StrPtrLen* realm,
              StrPtrLen* userPassword, 
              StrPtrLen* nonce, 
              StrPtrLen* cNonce,
              StrPtrLen* hA1
            ) 
{
    Assert(algorithm);
    Assert(userName);
    Assert(realm);
    Assert(userPassword);
    Assert(nonce);
    Assert(cNonce);
    Assert(hA1);
    Assert(hA1->Ptr == NULL); 
    
    MD5_CTX context;
    unsigned char aHash[kHashLen];
    
    MD5_Init(&context);
    MD5_Update(&context, (unsigned char *)userName->Ptr, userName->Len);
    MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    MD5_Update(&context, (unsigned char *)realm->Ptr, realm->Len);
    MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    MD5_Update(&context, (unsigned char *)userPassword->Ptr, userPassword->Len);
    MD5_Final(aHash, &context);
    if(algorithm->Equal(sMD5Sess)) {
        MD5_Init(&context);
        MD5_Update(&context, aHash, kHashLen);
        MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
        MD5_Update(&context, (unsigned char *)nonce->Ptr, nonce->Len);
        MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
        MD5_Update(&context, (unsigned char *)cNonce->Ptr, cNonce->Len);
        MD5_Final(aHash, &context);
    }
    HashToString(aHash, hA1);
}

void CalcHA1Md5Sess(StrPtrLen* hashA1Hex16Bit, StrPtrLen* nonce, StrPtrLen* cNonce, StrPtrLen* hA1)
{
    Assert(hashA1Hex16Bit);
    Assert(hashA1Hex16Bit->Len == kHashLen);
    Assert(nonce);
    Assert(cNonce);
    Assert(hA1);
    Assert(hA1->Ptr == NULL); 
    
    MD5_CTX context;
    unsigned char aHash[kHashLen];
    
    MD5_Init(&context);
    MD5_Update(&context, (unsigned char *)hashA1Hex16Bit->Ptr, kHashLen);
    MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    MD5_Update(&context, (unsigned char *)nonce->Ptr, nonce->Len);
    MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    MD5_Update(&context, (unsigned char *)cNonce->Ptr, cNonce->Len);
    MD5_Final(aHash, &context);
    HashToString(aHash, hA1);
}

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
                      )
{
    Assert(hA1);
    Assert(nonce);
    Assert(nonceCount);
    Assert(cNonce);
    Assert(qop);
    Assert(method);
    Assert(digestUri);
    Assert(hEntity);
    Assert(requestDigest);
    Assert(requestDigest->Ptr == NULL); 

    unsigned char aHash[kHashLen], requestHash[kHashLen];
    StrPtrLen hA2;
    MD5_CTX context;

    MD5_Init(&context);
    MD5_Update(&context, (unsigned char *)method->Ptr, method->Len);
    MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    MD5_Update(&context, (unsigned char *)digestUri->Ptr, digestUri->Len);
    if(qop->Equal(sQopAuthInt)) {
        MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
        MD5_Update(&context, (unsigned char *)hEntity->Ptr, hEntity->Len);
    }
    MD5_Final(aHash, &context);

    HashToString(aHash, &hA2);
    MD5_Init(&context);
    MD5_Update(&context, (unsigned char *)hA1->Ptr, hA1->Len);
    MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    MD5_Update(&context, (unsigned char *)nonce->Ptr, nonce->Len);
    MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    if(qop->Ptr != NULL) {
        MD5_Update(&context, (unsigned char *)nonceCount->Ptr, nonceCount->Len);
        MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
        MD5_Update(&context, (unsigned char *)cNonce->Ptr, cNonce->Len);
        MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
        MD5_Update(&context, (unsigned char *)qop->Ptr, qop->Len);
        MD5_Update(&context, (unsigned char *)sColon.Ptr, sColon.Len);
    }
    MD5_Update(&context, (unsigned char *)hA2.Ptr, hA2.Len);
    MD5_Final(requestHash, &context);
    HashToString(requestHash, requestDigest);
    
    delete [] hA2.Ptr;
}



/* From local_passwd.c (C) Regents of Univ. of California blah blah */
static unsigned char itoa64[] = /* 0 ... 63 => ascii - 64 */
"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void to64(register char *s, register long v, register int n)
{
    while (--n >= 0) {
    *s++ = itoa64[v & 0x3f];
    v >>= 6;
    }
}

static char *dufr_id = "$dufr$";

void MD5Encode(char *pw, char *salt, char *result, int nbytes)
{
  char passwd[120], *p;
  char *sp, *ep;
  unsigned char final[kHashLen];
  int sl, pl, i;
  MD5_CTX ctx, ctx1;
  unsigned long l;

  sp = salt;
  if (!strncmp(sp, dufr_id, strlen(dufr_id))) 
  {
    sp += strlen(dufr_id);
  }

  for (ep = sp; (*ep != '\0') && (*ep != '$') && (ep < (sp + 8)); ep++) 
  {
    continue;
  }

  //get the length of the true salt
  sl = ep - sp;
  
  MD5_Init(&ctx);

  //password
  MD5_Update(&ctx, (unsigned char *)pw, strlen(pw));

  //string
  MD5_Update(&ctx, (unsigned char *)dufr_id, strlen(dufr_id));

  //the raw salt
  MD5_Update(&ctx, (unsigned char *)sp, sl);

  MD5_Init(&ctx1);
  MD5_Update(&ctx1, (unsigned char *)pw, strlen(pw));
  MD5_Update(&ctx1, (unsigned char *)sp, sl);
  MD5_Update(&ctx1, (unsigned char *)pw, strlen(pw));
  MD5_Final(final, &ctx1);
  for (pl = strlen(pw); pl > 0; pl -= kHashLen)
  {
    MD5_Update(&ctx, (unsigned char *)final,(pl > kHashLen) ? kHashLen : pl);
  }

  memset(final, 0, sizeof(final));

  for (i = strlen(pw); i != 0; i >>= 1)
  {
    if (i & 1) {
      MD5_Update(&ctx, (unsigned char *)final, 1);
    }
    else {
      MD5_Update(&ctx, (unsigned char *)pw, 1);
    }
  }

  strcpy(passwd, dufr_id);
  strncat(passwd, sp, sl);
  strcat(passwd, "$");

  MD5_Final(final, &ctx);

  for (i = 0; i < 1000; i++)
  {
    MD5_Init(&ctx1);
    if (i & 1) {
      MD5_Update(&ctx1, (unsigned char *)pw, strlen(pw));
    }
    else {
      MD5_Update(&ctx1, final, kHashLen);
    }
    if (i % 3) {
      MD5_Update(&ctx1, (unsigned char *)sp, sl);
    }

    if (i % 7) {
      MD5_Update(&ctx1, (unsigned char *)pw, strlen(pw));
    }

    if (i & 1) {
      MD5_Update(&ctx1, (unsigned char *)final, kHashLen);
    }
    else {
      MD5_Update(&ctx1, (unsigned char *)pw, strlen(pw));
    }
    MD5_Final(final,&ctx1);
  }

  p = passwd + strlen(passwd);

  l = (final[ 0]<<16) | (final[ 6]<<8) | final[12]; to64(p, l, 4); p += 4;
  l = (final[ 1]<<16) | (final[ 7]<<8) | final[13]; to64(p, l, 4); p += 4;
  l = (final[ 2]<<16) | (final[ 8]<<8) | final[14]; to64(p, l, 4); p += 4;
  l = (final[ 3]<<16) | (final[ 9]<<8) | final[15]; to64(p, l, 4); p += 4;
  l = (final[ 4]<<16) | (final[10]<<8) | final[ 5]; to64(p, l, 4); p += 4;
  l =                    final[11]                ; to64(p, l, 2); p += 2;
  *p = '\0';
    
  memset(final, 0, sizeof(final));

  strncpy(result, passwd, nbytes - 1);
}
