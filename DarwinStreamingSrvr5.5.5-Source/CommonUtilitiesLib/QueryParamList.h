#ifndef __query_param_list__
#define __query_param_list__

#include "PLDoubleLinkedList.h"
#include "StrPtrLen.h"


class QueryParamListElement {

    public:
        QueryParamListElement( char* name, char* value )
        {
            mName   = name;
            mValue  = value;
                        
        }       
        
        virtual ~QueryParamListElement() 
        { 
            delete [] mName;
            delete [] mValue;
        }
        
        char    *mName;
        char    *mValue;

};


class QueryParamList
{
    public:
        QueryParamList( char* queryString );
        QueryParamList( StrPtrLen* querySPL );
        ~QueryParamList() { delete fNameValueQueryParamlist; }
        
        void AddNameValuePairToList( char* name, char* value );
        const char *DoFindCGIValueForParam( char *name );
        void PrintAll( char *idString );
        
    protected:
        void            BulidList( StrPtrLen* querySPL );
        void            DecodeArg( char *ioCodedPtr );
        enum { 
            // escaping states
              kLastWasText
            , kPctEscape
            , kRcvHexDigitOne
        };
        
        Bool16          IsHex( char c );
            
        PLDoubleLinkedList<QueryParamListElement>   *fNameValueQueryParamlist;
    

};





#endif
