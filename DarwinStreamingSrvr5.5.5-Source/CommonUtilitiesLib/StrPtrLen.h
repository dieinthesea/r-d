#ifndef __STRPTRLEN_H__
#define __STRPTRLEN_H__

#include <string.h>
#include "OSHeaders.h"
#include <ctype.h> 
#include "MyAssert.h"
#include "SafeStdLib.h"

#define STRPTRLENTESTING 0

class StrPtrLen
{
    public:

        StrPtrLen() : Ptr(NULL), Len(0) {}
        StrPtrLen(char* sp) : Ptr(sp), Len(sp != NULL ? strlen(sp) : 0) {}
        StrPtrLen(char *sp, UInt32 len) : Ptr(sp), Len(len) {}
        virtual ~StrPtrLen() {}

        Bool16 Equal(const StrPtrLen &compare) const;
        Bool16 EqualIgnoreCase(const char* compare, const UInt32 len) const;
        Bool16 EqualIgnoreCase(const StrPtrLen &compare) const { return EqualIgnoreCase(compare.Ptr, compare.Len); }
        Bool16 Equal(const char* compare) const;
        Bool16 NumEqualIgnoreCase(const char* compare, const UInt32 len) const;
        
        void Delete() { delete [] Ptr; Ptr = NULL; Len = 0; }
        char *ToUpper() { for (UInt32 x = 0; x < Len ; x++) Ptr[x] = toupper (Ptr[x]); return Ptr;}
        
        char *FindStringCase(char *queryCharStr, StrPtrLen *resultStr, Bool16 caseSensitive) const;

        char *FindString(StrPtrLen *queryStr, StrPtrLen *outResultStr)              {   Assert(queryStr != NULL);   Assert(queryStr->Ptr != NULL); Assert(0 == queryStr->Ptr[queryStr->Len]);
                                                                                        return FindStringCase(queryStr->Ptr, outResultStr,true);    
                                                                                    }
        
        char *FindStringIgnoreCase(StrPtrLen *queryStr, StrPtrLen *outResultStr)    {   Assert(queryStr != NULL);   Assert(queryStr->Ptr != NULL); Assert(0 == queryStr->Ptr[queryStr->Len]); 
                                                                                        return FindStringCase(queryStr->Ptr, outResultStr,false); 
                                                                                    }

        char *FindString(StrPtrLen *queryStr)                                       {   Assert(queryStr != NULL);   Assert(queryStr->Ptr != NULL); Assert(0 == queryStr->Ptr[queryStr->Len]); 
                                                                                        return FindStringCase(queryStr->Ptr, NULL,true);    
                                                                                    }
        
        char *FindStringIgnoreCase(StrPtrLen *queryStr)                             {   Assert(queryStr != NULL);   Assert(queryStr->Ptr != NULL); Assert(0 == queryStr->Ptr[queryStr->Len]); 
                                                                                        return FindStringCase(queryStr->Ptr, NULL,false); 
                                                                                    }
                                                                                    
        char *FindString(char *queryCharStr)                                        { return FindStringCase(queryCharStr, NULL,true);   }
        char *FindStringIgnoreCase(char *queryCharStr)                              { return FindStringCase(queryCharStr, NULL,false);  }
        char *FindString(char *queryCharStr, StrPtrLen *outResultStr)               { return FindStringCase(queryCharStr, outResultStr,true);   }
        char *FindStringIgnoreCase(char *queryCharStr, StrPtrLen *outResultStr)     { return FindStringCase(queryCharStr, outResultStr,false);  }

        char *FindString(StrPtrLen &query, StrPtrLen *outResultStr)                 { return FindString( &query, outResultStr);             }
        char *FindStringIgnoreCase(StrPtrLen &query, StrPtrLen *outResultStr)       { return FindStringIgnoreCase( &query, outResultStr);   }
        char *FindString(StrPtrLen &query)                                          { return FindString( &query);           }
        char *FindStringIgnoreCase(StrPtrLen &query)                                { return FindStringIgnoreCase( &query); }
        
        StrPtrLen& operator=(const StrPtrLen& newStr) { Ptr = newStr.Ptr; Len = newStr.Len;
                                                        return *this; }
         char operator[](int i) { /*Assert(i<Len);i*/ return Ptr[i]; }
        void Set(char* inPtr, UInt32 inLen) { Ptr = inPtr; Len = inLen; }
        void Set(char* inPtr) { Ptr = inPtr; Len = (inPtr) ?  ::strlen(inPtr) : 0; }

        char*       Ptr;
        UInt32      Len;

        char*   GetAsCString() const;
        void    PrintStr();
        void    PrintStr(char *appendStr);
        void    PrintStrEOL(char* stopStr = NULL, char *appendStr = NULL);

        UInt32    TrimTrailingWhitespace();
        UInt32    TrimLeadingWhitespace();
   
        UInt32  RemoveWhitespace();
        void  TrimWhitespace() { TrimLeadingWhitespace(); TrimTrailingWhitespace(); }

#if STRPTRLENTESTING
        static Bool16   Test();
#endif

    private:

        static UInt8    sCaseInsensitiveMask[];
        static UInt8    sNonPrintChars[];
};



class StrPtrLenDel : public StrPtrLen
{
  public:
     StrPtrLenDel() : StrPtrLen() {}
     StrPtrLenDel(char* sp) : StrPtrLen(sp) {}
     StrPtrLenDel(char *sp, UInt32 len) : StrPtrLen(sp,len) {}
     ~StrPtrLenDel() { Delete(); }
};

#endif 
