#include "QueryParamList.h"

#include "StringParser.h"
#include "OSMemory.h"

#include <string.h>
#include <stdlib.h>
#include "SafeStdLib.h"
QueryParamList::QueryParamList( StrPtrLen* querySPL )
{
    fNameValueQueryParamlist = NEW PLDoubleLinkedList<QueryParamListElement>;

    this->BulidList( querySPL );    
}


QueryParamList::QueryParamList( char* queryString )
{
    StrPtrLen       querySPL( queryString );

    fNameValueQueryParamlist = NEW PLDoubleLinkedList<QueryParamListElement>;
        
    this->BulidList( &querySPL );
}


void QueryParamList::BulidList( StrPtrLen* querySPL )
{
    
    StringParser    queryParser( querySPL );
    
    while  ( queryParser.GetDataRemaining() > 0 )
    {
        StrPtrLen       theCGIParamName;
        StrPtrLen       theCGIParamValue;
        
        queryParser.ConsumeUntil(&theCGIParamName, '=');        
        
        if ( queryParser.GetDataRemaining() > 1  )
        {
            queryParser.ConsumeLength(&theCGIParamValue, 1 );  
        
            queryParser.ConsumeUntil(&theCGIParamValue, '&');   
            
            AddNameValuePairToList( theCGIParamName.GetAsCString(), theCGIParamValue.GetAsCString() );
            
            queryParser.ConsumeLength(&theCGIParamValue, 1 );  
            
        }
    }
}


static void  PrintNameAndValue( PLDoubleLinkedListNode<QueryParamListElement> *node,  void *userData )
{
  
    QueryParamListElement*  nvPair = node->fElement;
    
    qtss_printf( "qpl: %s, name %s, val %s\n", (char*)userData, nvPair->mName, nvPair->mValue );
}


void QueryParamList::PrintAll( char *idString )
{
   
    fNameValueQueryParamlist->ForEach( PrintNameAndValue, idString );
}


static bool  CompareStrToName( PLDoubleLinkedListNode<QueryParamListElement> *node,  void *userData )
{
  
    QueryParamListElement*  nvPair = node->fElement;
    StrPtrLen               name( nvPair->mName );
    
    if ( name.EqualIgnoreCase( (char*)userData, strlen( (char*)userData ) )  )
        return true;
    
    return false;
}


const char *QueryParamList::DoFindCGIValueForParam( char *name )
{
   
    PLDoubleLinkedListNode<QueryParamListElement>*  node;

    node = fNameValueQueryParamlist->ForEachUntil( CompareStrToName, name );
    
    if ( node != NULL )
    {   
        QueryParamListElement*  nvPair = (QueryParamListElement*)node->fElement;
        
        return  nvPair->mValue;
    }
    
    return NULL;
    
}
    
    
void QueryParamList::AddNameValuePairToList( char* name, char* value  )
{
    
    PLDoubleLinkedListNode<QueryParamListElement>*      nvNode;
    QueryParamListElement*      nvPair;
    
    this->DecodeArg( name );
    this->DecodeArg( value );
    
    nvPair = NEW  QueryParamListElement( name, value );
    

    nvNode = NEW PLDoubleLinkedListNode<QueryParamListElement> ( nvPair );
    
    // add it to the list
    fNameValueQueryParamlist->AddNode( nvNode );
}



void QueryParamList::DecodeArg( char *ioCodedPtr )
{
    
    if ( !ioCodedPtr ) 
        return;

    char*   destPtr;
    char*   curChar;
    short   lineState = kLastWasText;
    char    hexBuff[32];

    destPtr = curChar = ioCodedPtr;
    
    while( *curChar )
    {
        switch( lineState )
        {
            case kRcvHexDigitOne:
                if ( IsHex( *curChar ) )
                {   
                    hexBuff[3] = *curChar;
                    hexBuff[4] = 0;
                    
                    *destPtr++ = (char)::strtoul( hexBuff, NULL, 0 );
                }
                else
                {   
                    *destPtr++ = '%';           
                    *destPtr++ = hexBuff[2];    
                    *destPtr++ = *curChar;     
                    
                }
                lineState = kLastWasText;
                break;

            case kLastWasText:
                if ( *curChar == '%' )
                    lineState = kPctEscape;
                else
                {
                    if (  *curChar == '+' )
                        *destPtr++ = ' ';
                    else
                        *destPtr++ = *curChar;
                }
                break;

            case kPctEscape:
                if ( *curChar == '%' )
                {
                    *destPtr++ = '%';
                    lineState = kLastWasText;
                }
                else
                {
                    if ( IsHex( *curChar ) )
                    {   
                        hexBuff[0] = '0';
                        hexBuff[1] = 'x';
                        hexBuff[2] = *curChar;
                        lineState = kRcvHexDigitOne;
                    }
                    else
                    {   
                        *destPtr++ = '%';       
                        *destPtr++ = *curChar;  
                        lineState = kLastWasText;
                    }
                }
                break;
                
                    
        }   
        
        curChar++;
    }

    *destPtr = *curChar;
}

Bool16 QueryParamList::IsHex( char c )
{
    
    if ( c >= '0' && c <= '9' )
        return true;

    if ( c >= 'A' && c <= 'F' )
        return true;

    if ( c >= 'a' && c <= 'f' )
        return true;

    return false;
}


