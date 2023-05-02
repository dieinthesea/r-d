/*

*/
/*
    File:       AdminElementNode.cpp

    Contains:   Implements Admin Elements
                    
    
    
*/


#ifndef __Win32__
    #include <unistd.h>     /* for getopt() et al */
#endif

#include <stdio.h>      /* for //qtss_printf */
#include <stdlib.h>     /* for getloadavg & other useful stuff */
#include <time.h>
#include "QTSS.h"
#include "QTSSAdminModule.h"
#include "OSArrayObjectDeleter.h"
#include "StringParser.h"
#include "StrPtrLen.h"
#include "QTSSModuleUtils.h"
#include "OSHashTable.h"
#include "OSMutex.h"
#include "StrPtrLen.h"
#include "OSRef.h"
#include "AdminElementNode.h"
#include "OSMemory.h"

static char* sParameterDelimeter = ";";
static char* sListDelimeter = ",";
static char* sAccess = "a=";
static char* sType = "t=";
static StrPtrLen sDoAllSPL("*");
static StrPtrLen sDoAllIndexIteratorSPL(":");

#define MEMORYDEBUGGING 0
#if MEMORYDEBUGGING
static SInt32 sMaxPtrs = 10000;
static void * sPtrArray[10000];
static char * sSourceArray[10000];
#endif

//Determine whether the incoming argument str is equal to sDoAllSPL or sDoAllIndexIteratorSPL; 
//if so, return true, otherwise return false.

Bool16  ElementNode_DoAll(StrPtrLen* str)
{   
    Assert(str); 
    Bool16 isIterator = false;
    
    if ( str->Equal(sDoAllSPL) || str->Equal(sDoAllIndexIteratorSPL)) 
        isIterator = true;
        
    return isIterator;
}

//Empty the sPtrArray and sSourceArray arrays, which are used to store pointers and the source code information corresponding to the pointers.
//This function is very useful when doing memory debugging.

void ElementNode_InitPtrArray()
{
#if MEMORYDEBUGGING
    memset(sPtrArray, 0, sizeof(sPtrArray));
    memset(sSourceArray, 0, sizeof(sSourceArray));
#endif
}

//Store the incoming pointer ptr and source code information src into the sPtrArray and sSourceArray arrays. 
//If the array is full, an error message is output and the program is terminated.

void ElementNode_InsertPtr(void *ptr, char * src)
{
#if MEMORYDEBUGGING
    if (ptr == NULL)
        return;

    for (SInt32 index = 0; index < sMaxPtrs; index ++)
    {
        if (sPtrArray[index] == NULL)
        {   sPtrArray[index] =ptr;
            sSourceArray[index] = src;
            //qtss_printf("%s INSERTED ptr=%lu countPtrs=%ld\n",src,(UInt32) ptr, ElementNode_CountPtrs());  
            return;
        }   
    }
    
    qtss_printf("ElementNode_InsertPtr no space in ptr array\n");
    Assert(0);
#endif
}

//Find if there is an element in the sPtrArray array that is equal to the incoming pointer ptr. 
//This function can be used to determine if there are duplicate pointers

Bool16 ElementNode_FindPtr(void *ptr, char * src)
{   // use for validating duplicates at some point
#if MEMORYDEBUGGING
    if (ptr == NULL)
        return false;
        
    for (SInt32 index = 0; index < sMaxPtrs; index ++)
    {   if (sPtrArray[index] == ptr)
            return true;
    }
    
#endif
    return false;
}

//Removes the pointer ptr from the dynamic array sPtrArray 
//and sets the value in the source string sSourceArray corresponding to this pointer to NULL.
//The conditional compilation instruction MEMORYDEBUGGING is used for debugging checks.
//If the corresponding element is not found, an error message is output and the program is terminated with an exception.

void ElementNode_RemovePtr(void *ptr, char * src)
{   
#if MEMORYDEBUGGING
    if (ptr == NULL)
        return;
        
    SInt16 foundCount = 0;
    for (SInt32 index = 0; index < sMaxPtrs; index ++)
    {
        if (sPtrArray[index] == ptr)
        {   
            sPtrArray[index] = NULL;
            sSourceArray[index] = NULL;
            //qtss_printf("%s REMOVED ptr countPtrs=%ld\n",src,ElementNode_CountPtrs());
            foundCount ++; // use for validating duplicates at some point
            return;
        }   
    }
    
    if (foundCount == 0)
    {   qtss_printf("PTR NOT FOUND ElementNode_RemovePtr %s ptr=%ld countPtrs=%ld\n",src,(UInt32) ptr,ElementNode_CountPtrs());
        Assert(0);
    }
#endif
}

//Count the number of non-null pointers in the dynamic array sPtrArray.
//When the conditional compilation instruction MEMORYDEBUGGING is defined, 
//the function iterates through all elements of the sPtrArray array, 
//counts the number of non-null pointers, and returns the value.
//Otherwise, the function returns 0.

SInt32 ElementNode_CountPtrs()
{
#if MEMORYDEBUGGING
    SInt32 count = 0;
    for (SInt32 index = 0; index < sMaxPtrs; index ++)
    {
        if (sPtrArray[index] != NULL)
            count ++;
    }
    
    return count;
#else
    return 0;
#endif
}

//Show the value of the non-null pointer in the dynamic array sPtrArray and the corresponding source string. 

void ElementNode_ShowPtrs()
{
#if MEMORYDEBUGGING
    for (SInt32 index = 0; index < sMaxPtrs; index ++)
    {
        if (sPtrArray[index] != NULL)
            qtss_printf("ShowPtrs ptr=%lu source=%s\n", (UInt32)sPtrArray[index],sSourceArray[index]); 
    }
#endif
}

//Used to output the value and length of a string pointer.

void PRINT_STR(StrPtrLen *spl)
{
    
    if (spl && spl->Ptr && spl->Ptr[0] != 0)
    {   char buff[1024] = {0};
        memcpy (buff,spl->Ptr, spl->Len);
        qtss_printf("%s len=%lu\n",buff,spl->Len);
    }
    else
    {   qtss_printf("(null)\n");
    }
}

//copy the contents of one buffer to another, the function throws an assertion exception if the input parameters are incorrect.

void COPYBUFFER(char *dest,char *src,SInt8 size)
{
    if ( (dest != NULL) && (src != NULL) && (size > 0) ) 
        memcpy (dest, src, size);
    else
        Assert(0);
};

//Create a new character array and copy the contents of the source string to that character array. 
//If the input source string pointer is not null,
//the function copies the contents of the string and returns the new character array pointer,
//otherwise it returns NULL.

char* NewCharArrayCopy(StrPtrLen *theStringPtr)
{
    char* newArray = NULL;
    if (theStringPtr != NULL)
    {
        newArray = NEW char[theStringPtr->Len + 1];  
        if (newArray != NULL) 
        {   memcpy(newArray, theStringPtr->Ptr,theStringPtr->Len);
            newArray[theStringPtr->Len] = 0;
        }
    }
    return newArray;
}

//ElementNode constructor. Initialize each member variable pointer to NULL, 
//set the first element of fPathBuffer to 0 and the length of fPathSPL to 0.
//fInitialized is set to false, fIsTop is set to false, and fDataFieldsType is set to eDynamic.

ElementNode::ElementNode()
{
    fDataSource     = NULL;
    fNumFields      = 0;
    fPathLen        = 0;
    fInitialized    = false;

    fFieldIDs       = NULL;
    fFieldDataPtrs  = NULL;
    fFieldOSRefPtrs = NULL;
    fParentNodePtr  = NULL;
    fElementMap     = NULL;
    fSelfPtr        = NULL;
    fPathBuffer[0]=0;
    fPathSPL.Set(fPathBuffer,0);
    fIsTop = false;
    fDataFieldsType = eDynamic;

};

//ElementNode initialization function. 
//If fInitialized is false, the parent node pointer, node source, node information pointer, node name pointer and node data pointer are set.

//Next, the node field is initialised according to the query URI, the current segment pointer and the initialisation parameters, 
//if the node field is empty. Finally set fInitialized to true and make some related settings

void ElementNode::Initialize(SInt32 index, ElementNode *parentPtr, QueryURI *queryPtr, StrPtrLen *currentSegmentPtr,QTSS_Initialize_Params *initParams,QTSS_Object nodeSource, DataFieldsType dataFieldsType)
{   
    //qtss_printf("------ ElementNode::Initialize ---------\n");
    
    if (!fInitialized)
    {
        SetParentNode(parentPtr);
        SetSource(nodeSource);
        
        SetNodeInfo(parentPtr->GetNodeInfoPtr(index));
        SetNodeName(parentPtr->GetName(index));
        SetMyElementDataPtr(parentPtr->GetElementDataPtr(index));

        fDataFieldsType = dataFieldsType;
        
        StrPtrLen nextSegment;
        StrPtrLen nextnextSegment;
        (void) queryPtr->NextSegment(currentSegmentPtr, &nextSegment);      
        (void) queryPtr->NextSegment(&nextSegment, &nextnextSegment);
        Bool16 forceAll = nextSegment.Equal(sDoAllIndexIteratorSPL) | nextnextSegment.Equal(sDoAllIndexIteratorSPL);
        
        if (GetFields() == NULL)
            InitializeAllFields(true, NULL, nodeSource,queryPtr,currentSegmentPtr,forceAll);
            
        fInitialized = true;
    }
    SetupNodes(queryPtr,currentSegmentPtr, initParams);
    
};

//ElementNode destructor. Loops through each field of the node, 
//deleting the node if it is a hash table and clearing it if it is a data pointer. 
//Finally delete the hash table, the data pointer and the field ID array

ElementNode::~ElementNode()
{
    
    //qtss_printf("ElementNode::~ElementNode delete %s Element Node # fields = %lu\n",GetNodeName(), fNumFields);
    
    for(SInt32 index = 0; !IsStopItem(index) ; index ++)
    {
        OSRef *theRefPtr = GetOSRef(index);
        if (theRefPtr != NULL )
        {
            //qtss_printf("deleting hash entry of %s \n", GetName(index));
            SetOSRef(index,NULL);
            (fElementMap->GetHashTable())->Remove(theRefPtr);
            delete (OSRef*) theRefPtr;  ElementNode_RemovePtr(theRefPtr,"ElementNode::~ElementNode OSRef *");
        }
        
        char *dataPtr = GetElementDataPtr(index);
        if (dataPtr != NULL)
        {
            SetElementDataPtr(index,NULL,IsNodeElement(index));
        }
    }
    
    delete fElementMap;  ElementNode_RemovePtr(fElementMap,"ElementNode::~ElementNode fElementMap");
    
    fElementMap = NULL;
    
    UInt32 i = 0;
    for (i = 0; i < GetNumFields(); i ++)
    {   SetElementDataPtr(i,NULL, IsNodeElement(i)) ;
        fFieldDataPtrs[i] = NULL;
    }
    delete fFieldDataPtrs; ElementNode_RemovePtr(fFieldDataPtrs,"ElementNode::~ElementNode fFieldDataPtrs");
    fFieldDataPtrs = NULL;
    
    for (i = 0; i < GetNumFields(); i ++)
    {   delete fFieldOSRefPtrs[i]; ElementNode_RemovePtr(fFieldOSRefPtrs[i],"ElementNode::~ElementNode fFieldOSRefPtrs");
        fFieldOSRefPtrs[i] = NULL;
    }
    delete fFieldOSRefPtrs;  ElementNode_RemovePtr(fFieldOSRefPtrs,"ElementNode::~ElementNode fFieldOSRefPtrs");
    fFieldOSRefPtrs = NULL;
            
    if (fDataFieldsType == eDynamic)
    {   delete fFieldIDs;  ElementNode_RemovePtr(fFieldIDs,"ElementNode::~ElementNode fFieldIDs");
        fFieldIDs = NULL;
    }
        
    SetNodeName(NULL);

};

// assign numFields fields to the ElementNode object
// numFields: the number of fields to be allocated
// Return value: QTSS_Error type, indicating whether the allocation is successful or not

QTSS_Error ElementNode::AllocateFields(UInt32 numFields)
{
    //qtss_printf("-------- ElementNode::AllocateFields ----------\n");
    //qtss_printf("ElementNode::AllocateFields numFields=%lu\n",numFields);
    
    QTSS_Error err = QTSS_NotEnoughSpace;// set the default return value to QTSS_NotEnoughSpace

    
    Assert(GetNumFields() == 0);// Assert that the current number of fields is 0
    SetNumFields(numFields);// set the current number of fields to numFields


    if (numFields > 0) do // If the number of fields to be allocated is greater than 0, then do the following
    {   
        Assert(fFieldIDs == NULL); // Assert that the fFieldIDs array is a null pointer
        fFieldIDs = NEW ElementNode::ElementDataFields[numFields]; ElementNode_InsertPtr(fFieldIDs, "ElementNode::AllocateFields fFieldIDs array" ); // allocate memory for the fFieldIDs array and insert it into the list where the element was inserted
        Assert(fFieldIDs ! = NULL); // Assert that the fFieldIDs array is not a null pointer
        if (fFieldIDs == NULL) break; // if the fFieldIDs array is a null pointer, jump out of the loop
        memset(fFieldIDs, 0, numFields * sizeof(ElementNode::ElementDataFields)); // clear the fFieldIDs array to zero

        Assert(fElementMap == NULL); // Assert fElementMap as a null pointer
        fElementMap = NEW OSRefTable(); ElementNode_InsertPtr(fElementMap, "ElementNode::AllocateFields fElementMap OSRefTable"); // allocate memory for fElementMap allocate memory and insert it into the list where the element is inserted
        Assert(fElementMap ! = NULL); // Assert that fElementMap is not a null pointer
        if (fElementMap == NULL) break; // if fElementMap is a null pointer, then skip the loop

        Assert(fFieldDataPtrs == NULL); // Assert that fFieldDataPtrs is a null pointer
        fFieldDataPtrs = NEW char*[numFields]; ElementNode_InsertPtr(fFieldDataPtrs, "ElementNode::AllocateFields fFieldDataPtrs array"); // Assert(fFieldDataPtrs == NULL); // Assert(fFieldDataPtrs == NULL); // Assert(fFieldDataPtrs == NULL); // Assert(fFieldDataPtrs == NULL) fFieldDataPtrs array to allocate memory and insert into the list of element insertions
        Assert(fFieldDataPtrs ! = NULL); // Assert that fFieldDataPtrs is not a null pointer
        if (fFieldDataPtrs == NULL) break; // if fFieldDataPtrs is a null pointer, then jump out of the loop
        memset(fFieldDataPtrs, 0, numFields * sizeof(char*)); // clear the fFieldDataPtrs array to zero

        Assert(fFieldOSRefPtrs == NULL); // Assert that fFieldOSRefPtrs is a null pointer
        fFieldOSRefPtrs = NEW OSRef*[numFields]; ElementNode_InsertPtr(fFieldOSRefPtrs, "ElementNode::AllocateFields fFieldDataPtrs array"); // Assert(fFieldOSRefPtrs == NULL); // Assert(fFieldOSRefPtrs == NULL); // Assert(fFieldOSRefPtrs == NULL) fFieldOSRefPtrs array to allocate memory and insert into the list of element insertions
        Assert(fFieldOSRefPtrs ! = NULL); // Assert that fFieldOSRefPtrs is not a null pointer
        if (fFieldOSRefPtrs == NULL) break; // if fFieldOSRefPtrs is a null pointer, then skip the loop
        memset(fFieldOSRefPtrs, 0, numFields * sizeof(OSRef*)); // clear the fFieldOSRefPtrs array to zero
        
        err = QTSS_NoErr;// assign successfully, set return value to QTSS_NoErr
    } while (false);
    
    return err; 
};


//Set the attribute field of an element node

void ElementNode::SetFields(UInt32 i, QTSS_Object attrInfoObject)
{
    //qtss_printf("------- ElementNode::SetFields -------- \n");

    UInt32 ioLen = 0;
    QTSS_Error err = QTSS_NoErr;
    if(fFieldIDs[i].fFieldName[0] != 0)
        return;//If the field name is not 0, return directly
    
    //Set the field name 
    if(fFieldIDs[i].fFieldName[0] == 0)
    {
        fFieldIDs[i].fFieldLen = eMaxAttributeNameSize;
        err = QTSS_GetValue (attrInfoObject, qtssAttrName,0, &fFieldIDs[i].fFieldName,&fFieldIDs[i].fFieldLen);
        Assert(err == QTSS_NoErr);  
        if (fFieldIDs[i].fFieldName != NULL)
            fFieldIDs[i].fFieldName[fFieldIDs[i].fFieldLen] = 0;
    }
    
    //Get API ID
    ioLen = sizeof(fFieldIDs[i].fAPI_ID);
    err = QTSS_GetValue (attrInfoObject, qtssAttrID,0, &fFieldIDs[i].fAPI_ID, &ioLen);
    Assert(err == QTSS_NoErr);  
    
    //Get API Type
    ioLen = sizeof(fFieldIDs[i].fAPI_Type);
    err = QTSS_GetValue (attrInfoObject, qtssAttrDataType,0, &fFieldIDs[i].fAPI_Type, &ioLen);
    Assert(err == QTSS_NoErr);  
    if (fFieldIDs[i].fAPI_Type == 0 || err != QTSS_NoErr)
    {
        //qtss_printf("QTSS_GetValue err = %ld attrInfoObject=%lu qtssAttrDataType = %lu \n",err, (UInt32)  attrInfoObject, (UInt32) fFieldIDs[i].fAPI_Type);
    }
    
    // If the API type is QTSS_Object, then set the field type to eNode
    if (fFieldIDs[i].fAPI_Type == qtssAttrDataTypeQTSS_Object)
    fFieldIDs[i].fFieldType = eNode.
    
    // Get access rights
    ioLen = sizeof(fFieldIDs[i].fAccessPermissions).
    err = QTSS_GetValue (attrInfoObject, qtssAttrPermissions,0, &fFieldIDs[i].fAccessPermissions, &ioLen).
    Assert(err == QTSS_NoErr).  

    // Initialize access data
    fFieldIDs[i].fAccessData[0] = 0.
    if (fFieldIDs[i].fAccessPermissions & qtssAttrModeRead)
    { strcat(fFieldIDs[i].fAccessData, "r"); // add 'r' if read permissions are available
    }

    if (fFieldIDs[i].fAccessPermissions & qtssAttrModeWrite && fFieldIDs[i].fAPI_Type ! = qtssAttrDataTypeQTSS_Object)
    { strcat(fFieldIDs[i].fAccessData, "w"); // add 'w' if there are write permissions and the API type is not QTSS_Object
    }

    if (fFieldIDs[i].fAccessPermissions & qtssAttrModeInstanceAttrAllowed && fFieldIDs[i].fAPI_Type == qtssAttrDataTypeQTSS_Object)
    { strcat(fFieldIDs[i].fAccessData, "w"); // add 'w' if it is of type QTSS_Object
    }

    // if the node type is not eNode and the number of fields is greater than 1, add 'd'
    if (GetMyFieldType() ! = eNode && GetNumFields() > 1)
    { strcat(fFieldIDs[i].fAccessData, "d").
    }
    if (fFieldIDs[i].fAccessPermissions & qtssAttrModeDelete)
    { strcat(fFieldIDs[i].fAccessData, "d"); // add 'd' if delete permission is available
    }

    // add 'p' if there is a preemption prevention permission
    if (fFieldIDs[i].fAccessPermissions & qtssAttrModePreempSafe)
    { strcat(fFieldIDs[i].fAccessData, "p").
    }

    fFieldIDs[i].fAccessLen = strlen(fFieldIDs[i].fAccessData); // Calculate the length of the access data

    
    //qtss_printf("ElementNode::SetFields name=%s api_id=%ld \n",fFieldIDs[i].fFieldName, fFieldIDs[i].fAPI_ID);
    //DebugShowFieldDataType(i);    
};


ElementNode* ElementNode::CreateArrayAttributeNode(UInt32 index, QTSS_Object source, QTSS_Object attributeInfo, UInt32 arraySize )
{
    //qtss_printf("------- ElementNode::CreateArrayAttributeNode --------\n");
    //qtss_printf("ElementNode::CreateArrayAttributeNode name = %s index = %lu arraySize =%lu \n",fFieldIDs[index].fFieldName, index,arraySize);

    ElementDataFields* fieldPtr = NULL;
    SetFields(index, attributeInfo);
    fFieldIDs[index].fFieldType = eArrayNode;
    
    ElementNode* nodePtr = NEW ElementNode(); ElementNode_InsertPtr(nodePtr,"ElementNode::CreateArrayAttributeNode ElementNode*");
    this->SetElementDataPtr(index,(char *) nodePtr, true);
    Assert(nodePtr!=NULL);
    if (NULL == nodePtr) return NULL;
    
    nodePtr->SetSource(source); // the node's API source
    nodePtr->AllocateFields(arraySize); 
    
    if (this->GetNodeInfoPtr(index) == NULL)
    {   //qtss_printf("ElementNode::CreateArrayAttributeNode index = %lu this->GetNodeInfoPtr(index) == NULL \n",index);
    }
    nodePtr->SetNodeInfo(this->GetNodeInfoPtr(index));
    
    for (UInt32 i = 0; !nodePtr->IsStopItem(i); i++)
    {   
        fieldPtr = nodePtr->GetElementFieldPtr(i);
        Assert(fieldPtr != NULL);
        
        nodePtr->SetFields(i, attributeInfo);
        
        fieldPtr->fIndex = i; // set the API attribute index

        // set the name of the field to the array index 
        fieldPtr->fFieldName[0]= 0; 
        qtss_sprintf(fieldPtr->fFieldName,"%lu",i);
        fieldPtr->fFieldLen = ::strlen(fieldPtr->fFieldName);
        
        if (fieldPtr->fAPI_Type != qtssAttrDataTypeQTSS_Object)
        {
            //qtss_printf("ElementNode::CreateArrayAttributeNode array field index = %lu name = %s api Source = %lu \n", (UInt32)  i,fieldPtr->fFieldName, (UInt32) source);
            fieldPtr->fAPISource = source; // the attribute's source is the same as node source
        }
        else 
        {   fieldPtr->fFieldType = eNode;
            // this is an array of objects so record each object as the source for a new node
            UInt32 sourceLen = sizeof(fieldPtr->fAPISource);
            QTSS_Error err = QTSS_GetValue (source,fieldPtr->fAPI_ID,fieldPtr->fIndex, &fieldPtr->fAPISource, &sourceLen);
            Warn(err == QTSS_NoErr);
            if (err != QTSS_NoErr)
            {   //qtss_printf("Error Getting Value for %s type = qtssAttrDataTypeQTSS_Object err = %lu\n", fieldPtr->fFieldName,err);
                fieldPtr->fAPISource = NULL;
            }
            
            QTSS_AttributeID id;
            Bool16 foundFilteredAttribute = GetFilteredAttributeID(GetMyName(),nodePtr->GetMyName(), &id);
            if (foundFilteredAttribute)
            {   GetFilteredAttributeName(fieldPtr,id);
            }

            //qtss_printf("ElementNode::CreateArrayAttributeNode array field index = %lu name = %s api Source = %lu \n", i,fieldPtr->fFieldName, (UInt32) fieldPtr->fAPISource);
        }

        nodePtr->fElementMap->Register(nodePtr->GetOSRef(i));
    }
    nodePtr->SetNodeName(GetName(index));
    nodePtr->SetParentNode(this);
    nodePtr->SetSource(source);
    nodePtr->fInitialized = true;
    
    return nodePtr;
    
}

void ElementNode::InitializeAllFields(Bool16 allocateFields, QTSS_Object defaultAttributeInfo, QTSS_Object source, QueryURI *queryPtr, StrPtrLen *currentSegmentPtr , Bool16 forceAll =false)
{
    //qtss_printf("------- ElementNode::InitializeAllFields -------- \n");
    
    QTSS_Error err = QTSS_NoErr;
    QTSS_Object theAttributeInfo;
    
    if (allocateFields)
    {           
        UInt32 numFields = this->CountAttributes(source);
        err = AllocateFields( numFields);
        //qtss_printf("ElementNode::InitializeAllFields AllocateFields numFields =  %lu error = %ld \n",numFields, err);
    }
    
    if (err == QTSS_NoErr)
    {   
        UInt32 numValues = 0;

        for (UInt32 i = 0; !IsStopItem(i); i++)
        {   
            if (defaultAttributeInfo == NULL)
            {   err = QTSS_GetAttrInfoByIndex(source, i, &theAttributeInfo);
                Assert(err == QTSS_NoErr);          
                if (err != QTSS_NoErr)
                {   //qtss_printf("QTSS_GetAttrInfoByIndex returned err = %lu \n",err);
                }
            }
            else
            {   theAttributeInfo = defaultAttributeInfo; 
            }
            
            SetFields(i, theAttributeInfo);         
            
            if ((SInt32) fFieldIDs[i].fAPI_ID < 0)
            {   //qtss_printf("ElementNode::InitializeAllFields name = %s index = %ld numValues =%lu \n",fFieldIDs[i].fFieldName, (SInt32) fFieldIDs[i].fAPI_ID,numValues);
            }
            numValues = this->CountValues(source, fFieldIDs[i].fAPI_ID);
            //qtss_printf("ElementNode::InitializeAllFields name = %s index = %lu numValues =%lu \n",fFieldIDs[i].fFieldName, fFieldIDs[i].fAPI_ID,numValues);

            QTSS_AttributeID id;
            Bool16 foundFilteredAttribute = GetFilteredAttributeID(GetMyName(),GetName(i), &id);
            
            StrPtrLen nextSegment;
            (void) queryPtr->NextSegment(currentSegmentPtr, &nextSegment);

            if (forceAll || nextSegment.Equal(sDoAllIndexIteratorSPL) || queryPtr->IndexParam() || numValues > 1 || foundFilteredAttribute)
            {       
                ElementNode *nodePtr = CreateArrayAttributeNode(i, source, theAttributeInfo,numValues);
                Assert(nodePtr != NULL);
                /*
                if (NULL == nodePtr) 
                {   //qtss_printf("ElementNode::InitializeAllFields(NULL == CreateArrayAttributeNode  nodePtr\n");
                }
                if (NULL == GetElementDataPtr(i)) 
                {   //qtss_printf("ElementNode::InitializeAllFields(NULL == GetElementDataPtr (i=%lu) nodePtr=%lu \n",i, (UInt32) nodePtr);
                }
                */
                    
            }
            else
            {
                //qtss_printf("ElementNode::InitializeAllFields field index = %lu name = %s api Source = %lu \n", i,fFieldIDs[i].fFieldName, (UInt32) source);
            }
            
            err = fElementMap->Register(GetOSRef(i));
            if (err != QTSS_NoErr)
            {   //qtss_printf("ElementNode::InitializeAllFields  Register returned err = %lu field = %s node=%s \n",err,GetName(i),GetMyName());
            }
            Assert(err == QTSS_NoErr);  
        }
    }
};


void ElementNode::SetNodeInfo(ElementDataFields *nodeInfoPtr)
{
    if (nodeInfoPtr == NULL)
    {
        //qtss_printf("---- SetNodeInfo nodeInfoPtr = NULL \n");
    }
    else
    {
        //qtss_printf("---- SetNodeInfo nodeInfoPtr name = %s \n",nodeInfoPtr->fFieldName);
        fSelfPtr = nodeInfoPtr;
    }
};


void ElementNode::SetNodeName(char *namePtr)
{
    if (namePtr == NULL)// If the node name is a null pointer, delete the original node name and return
    {   delete fNodeNameSPL.Ptr; ElementNode_RemovePtr(fNodeNameSPL.Ptr,"ElementNode::SetNodeName char* ");
        fNodeNameSPL.Set(NULL, 0);
        return;
    }
    
    if (fNodeNameSPL.Ptr != NULL) // If the node name pointer is not null, delete the original node name
    {   delete fNodeNameSPL.Ptr; ElementNode_RemovePtr(fNodeNameSPL.Ptr,"ElementNode::SetNodeName char* ");
        fNodeNameSPL.Set(NULL, 0);
    }
    //qtss_printf(" ElementNode::SetNodeName new NodeName = %s \n",namePtr);
    
    // Allocate new node name memory space and copy the incoming node name into memory
    int len = ::strlen(namePtr);    
    fNodeNameSPL.Ptr = NEW char[len + 1]; ElementNode_InsertPtr(fNodeNameSPL.Ptr,"ElementNode::SetNodeName ElementNode* chars");
    fNodeNameSPL.Len = len; 
    memcpy(fNodeNameSPL.Ptr,namePtr,len);
    fNodeNameSPL.Ptr[len] = 0;
};

//returns a pointer to a specific index element
ElementNode::ElementDataFields *ElementNode::GetElementFieldPtr(SInt32 index) 
{ 
    ElementNode::ElementDataFields *resultPtr = NULL; 
    Assert (fFieldIDs != NULL);
    Assert ((index >= 0) && (index < (SInt32) fNumFields));
    if ((index >= 0) && (index < (SInt32) fNumFields)) 
        resultPtr = &fFieldIDs[index]; 
    return resultPtr; 
}

//returns a pointer to a specific index data
char *ElementNode::GetElementDataPtr(SInt32 index) 
{ 
    char *resultPtr = NULL; 
    Assert((index >= 0) && (index < (SInt32) fNumFields));
    if (fInitialized && (fFieldDataPtrs != NULL) && (index >= 0) && (index < (SInt32) fNumFields)) 
    {   resultPtr = fFieldDataPtrs[index]; 
    }
    return resultPtr; 
}

//sets the pointer to the specific index data to the given value
void ElementNode::SetElementDataPtr(SInt32 index,char *data, Bool16 isNode) 
{ 
    //qtss_printf("------ElementNode::SetElementDataPtr----- \n");
    //qtss_printf("ElementNode::SetElementDataPtr index = %ld fNumFields = %ld \n", index,fNumFields);
    Assert  ((index >= 0) && (index < (SInt32) fNumFields));
    if      ((index >= 0) && (index < (SInt32) fNumFields)) 
    {   //Assert(fFieldDataPtrs[index] == NULL);
        if (fDataFieldsType != eStatic)
        {   
            if (isNode)
            {   delete (ElementNode*) fFieldDataPtrs[index];ElementNode_RemovePtr(fFieldDataPtrs[index],"ElementNode::SetElementDataPtr ElementNode* fFieldDataPtrs");
            }
            else
            {   delete fFieldDataPtrs[index]; ElementNode_RemovePtr(fFieldDataPtrs[index],"ElementNode::SetElementDataPtr char* fFieldDataPtrs");
            }
        }
        fFieldDataPtrs[index] = data; 
        //qtss_printf("ElementNode::SetElementDataPtr index = %ld \n", index);
    }
}


//debugging functions
inline void ElementNode::DebugShowFieldDataType(SInt32 /*index*/)
{
    //char field[100];
    //field[0] = ' ';
    //char* typeStringPtr = GetAPI_TypeStr(index);
    //qtss_printf("debug: %s=%s\n",GetName(index),typeStringPtr);
        
}

inline void ElementNode::DebugShowFieldValue(SInt32 /*index*/)
{
    //qtss_printf("debug: %s=%s\n",GetName(index),GetElementDataPtr(index));
}

//Handling dynamic arrays, pointers and strings

ElementNode::ElementDataFields *ElementNode::GetNodeInfoPtr(SInt32 index) 
{ 
    ElementNode::ElementDataFields *resultPtr = GetElementFieldPtr(index);
    Assert (resultPtr != NULL);
    
    if ((resultPtr != NULL) && ((eNode != resultPtr->fFieldType) && (eArrayNode != resultPtr->fFieldType) )) 
        resultPtr = NULL; 
    return resultPtr; 
}       


void ElementNode::SetUpSingleNode(QueryURI *queryPtr,  StrPtrLen *currentSegmentPtr, StrPtrLen *nextSegmentPtr, SInt32 index, QTSS_Initialize_Params *initParams) 
{
    //qtss_printf("--------ElementNode::SetUpSingleNode ------------\n");
    if (queryPtr && currentSegmentPtr && nextSegmentPtr&& initParams) do
    {
        if (!queryPtr->RecurseParam() && (nextSegmentPtr->Len == 0) ) break;
    
        ElementNode::ElementDataFields *theNodePtr = GetNodeInfoPtr(index); 
        if (NULL == theNodePtr) 
        {
            //qtss_printf(" ElementNode::SetUpSingleNode (NULL == theNodePtr(%ld)) name=%s \n",index,GetName(index));
            break;
        }
        
        if (!IsNodeElement(index) ) 
        {
            //qtss_printf(" ElementNode::SetUpSingleNode (apiType != qtssAttrDataTypeQTSS_Object) \n");
            break;
        }
        
        // filter unnecessary nodes     
        char *nodeName = GetName(index); 
        if (nodeName != NULL)
        {   StrPtrLen nodeNameSPL(nodeName);
            if  (   (!nodeNameSPL.Equal(*nextSegmentPtr) && !ElementNode_DoAll(nextSegmentPtr))
                    &&
                    !(queryPtr->RecurseParam() && (nextSegmentPtr->Len == 0))
                )
            {
                //qtss_printf(" ElementNode::SetUpSingleNode SPL TEST SKIP NodeElement= %s\n",GetName(index));
                //qtss_printf("ElementNode::SetUpAllNodes skip nextSegmentPtr=");PRINT_STR(nextSegmentPtr);
                break;
            }
                
        }
        
        ElementNode *nodePtr = NULL;
        nodePtr = (ElementNode *) GetElementDataPtr(index);     
        if (nodePtr == NULL)
        {   
            //qtss_printf("ElementNode::SetUpSingleNode %s nodePtr == NULL make NEW nodePtr index = %ld\n", GetMyName(),index);
            nodePtr = NEW ElementNode(); ElementNode_InsertPtr(nodePtr,"ElementNode::SetUpSingleNode ElementNode* NEW ElementNode() ");     
            SetElementDataPtr(index,(char *) nodePtr, true); 
        }
        
        if (nodePtr != NULL)
        {
            StrPtrLen tempSegment;
            ( void)queryPtr->NextSegment(nextSegmentPtr, &tempSegment);
            currentSegmentPtr = nextSegmentPtr;
            nextSegmentPtr = &tempSegment;
            

            if (!nodePtr->fInitialized)
            {
                //qtss_printf("ElementNode::SetUpSingleNode Node !fInitialized -- Initialize %s\n",GetName(index));
                //qtss_printf("ElementNode::SetUpSingleNode GetValue source = %lu name = %s id = %lu \n",(UInt32)  GetSource(),(UInt32)  GetName(index),(UInt32) GetAPI_ID(index));
                
                ElementDataFields* fieldPtr = GetElementFieldPtr(index);
                if (fieldPtr != NULL && fieldPtr->fAPI_Type == qtssAttrDataTypeQTSS_Object)
                {   UInt32 sourceLen = sizeof(fieldPtr->fAPISource);
                    (void) QTSS_GetValue (GetSource(),fieldPtr->fAPI_ID,fieldPtr->fIndex, &fieldPtr->fAPISource, &sourceLen);
                }
            
                QTSS_Object theSourceObject = GetAPISource(index);
                //Warn(theSourceObject != NULL);

                nodePtr->Initialize(index, this, queryPtr,nextSegmentPtr,initParams, theSourceObject, eDynamic);
                nodePtr->SetUpAllElements(queryPtr, currentSegmentPtr,nextSegmentPtr, initParams);
                fInitialized = true;
    
                break;

            }
            else
            {
                nodePtr->SetUpAllElements(queryPtr, currentSegmentPtr,nextSegmentPtr, initParams);      
            }
        }
        
    } while (false);
    
    return;
}

Bool16 ElementNode::SetUpOneDataField( UInt32 index)
{
    //qtss_printf("----ElementNode::SetUpOneDataField----\n");       
    //qtss_printf(" ElementNode::SetUpOneDataField parent = %s field name=%s\n",GetNodeName(), GetName(index));  
    
    QTSS_AttributeID inID = GetAPI_ID(index); 
    Bool16 isNodeResult = IsNodeElement(index);
    char *testPtr =  GetElementDataPtr(index);
    //Warn(NULL == testPtr);
    if (NULL != testPtr) 
    {   //qtss_printf(" ElementNode::SetUpOneDataField skip field already setup parent = %s field name=%s\n",GetNodeName(), GetName(index)); 
        return isNodeResult;
    }
    
    if (!isNodeResult)
    {
        //qtss_printf("ElementNode::SetUpOneDataField %s Source=%lu Field index=%lu API_ID=%lu value index=%lu\n",GetName(index),GetSource(), index,inID,GetAttributeIndex(index));  
        SetElementDataPtr(index, NewIndexElement (GetSource() , inID, GetAttributeIndex(index)), false);
    }
    else
    {
        //qtss_printf("ElementNode::SetUpOneDataField %s Source=%lu Field index=%lu API_ID=%lu value index=%lu\n",GetName(index),(UInt32) GetSource(),(UInt32)  index,(UInt32) inID,(UInt32) GetAttributeIndex(index));  
        //qtss_printf("ElementNode::SetUpOneDataField %s IsNodeElement index = %lu\n",GetName(index),(UInt32)  index);   
        //DebugShowFieldDataType(index);
    }

    DebugShowFieldValue( index);

    return isNodeResult;
}

void ElementNode::SetUpAllElements(QueryURI *queryPtr, StrPtrLen *currentSegmentPtr,StrPtrLen *nextSegmentPtr,  QTSS_Initialize_Params *initParams) 
{
    //qtss_printf("---------ElementNode::SetUpAllElements------- \n");

    for(SInt32 index = 0; !IsStopItem(index); index ++)
    {
        SetUpSingleElement(queryPtr, currentSegmentPtr,nextSegmentPtr, index,initParams);
    }
}




void ElementNode::SetUpSingleElement(QueryURI *queryPtr, StrPtrLen *currentSegmentPtr,StrPtrLen *nextSegmentPtr, SInt32 index, QTSS_Initialize_Params *initParams) 
{
    //qtss_printf("---------ElementNode::SetUpSingleElement------- \n");
    StrPtrLen indexNodeNameSPL;
    GetNameSPL(index,&indexNodeNameSPL);
    if  (   (queryPtr->RecurseParam() && (nextSegmentPtr->Len == 0))
            || 
            (indexNodeNameSPL.Equal(*nextSegmentPtr) || ElementNode_DoAll(nextSegmentPtr)) 
        ) // filter unnecessary elements        
    {
    
        Bool16 isNode = SetUpOneDataField((UInt32) index);      
        if (isNode)
        {
            //qtss_printf("ElementNode::SetUpSingleElement isNode=true calling SetUpSingleNode \n");
            SetUpSingleNode(queryPtr,currentSegmentPtr, nextSegmentPtr, index, initParams);
        }
    }
    else
    {   //qtss_printf("ElementNode::SetUpSingleElement filter element=%s\n",GetName(index));
    }
}


void ElementNode::SetUpAllNodes(QueryURI *queryPtr, StrPtrLen *currentSegmentPtr, StrPtrLen *nextSegmentPtr, QTSS_Initialize_Params *initParams) 
{
    //qtss_printf("--------ElementNode::SetUpAllNodes------- \n");
    for(SInt32 index = 0; !IsStopItem(index); index ++)
    {
        if (!queryPtr->RecurseParam() && (nextSegmentPtr->Len == 0) ) break;
        
        //qtss_printf("ElementNode::SetUpAllNodes index = %ld nextSegmentPtr=", index);PRINT_STR(nextSegmentPtr);
        StrPtrLen indexNodeNameSPL;
        GetNameSPL(index,&indexNodeNameSPL);
        if  (   IsNodeElement(index) 
                &&
                (   (queryPtr->RecurseParam() && (nextSegmentPtr->Len == 0))
                    || 
                    (indexNodeNameSPL.Equal(*nextSegmentPtr) || ElementNode_DoAll(nextSegmentPtr)) 
                )
            ) // filter unnecessary nodes       
            SetUpSingleNode(queryPtr, currentSegmentPtr, nextSegmentPtr, index, initParams);
        else
        {
            //qtss_printf("ElementNode::SetUpAllNodes skip index = %ld indexNodeName=", index);PRINT_STR(&indexNodeNameSPL);
            //qtss_printf("ElementNode::SetUpAllNodes skip nextSegmentPtr=");PRINT_STR(nextSegmentPtr);
        }
    }
}

// Gets the length of the specified attribute
QTSS_Error ElementNode::GetAttributeSize (QTSS_Object inObject, QTSS_AttributeID inID, UInt32 inIndex, UInt32* outLenPtr)
{
    return QTSS_GetValue (inObject, inID, inIndex, NULL, outLenPtr);
}

//gets the value of the element at the specified index and returns a string pointer
char *ElementNode::NewIndexElement (QTSS_Object inObject, QTSS_AttributeID inID, UInt32 inIndex)
{   
    QTSS_Error err = QTSS_NoErr;
    char *resultPtr = NULL;

    Assert(inObject != NULL);
    
    if (inObject != NULL)
    {   err = QTSS_GetValueAsString (inObject, inID, inIndex, &resultPtr); ElementNode_InsertPtr(resultPtr,"ElementNode::NewIndexElement QTSS_GetValueAsString ");
        if (err != QTSS_NoErr)
        {   //qtss_printf("ElementNode::NewIndexElement QTSS_GetValueAsString object= %lu id=%lu index=%lu err= %ld \n",inObject,inID, inIndex, err);
        }
    }
    return resultPtr;
}

//converts a key value of type string to an index of type integer
inline  SInt32 ElementNode::ResolveSPLKeyToIndex(StrPtrLen *keyPtr)
{   
    SInt32 index = -1; 
    OSRef* osrefptr = NULL; 
            
    if (fElementMap != NULL && keyPtr != NULL && keyPtr->Len > 0) 
    {   osrefptr = fElementMap->Resolve(keyPtr);
        if (osrefptr != NULL) 
            index = (SInt32) osrefptr->GetObject();
    }

    return index;   
}

//gets the number of attributes of a node
UInt32 ElementNode::CountAttributes(QTSS_Object source)
{
    //qtss_printf("------ElementNode::CountAttributes-------\n");
    //qtss_printf("ElementNode::CountAttributes SOURCE = %lu \n", (UInt32) source);

    UInt32 numFields = 0;

    (void) QTSS_GetNumAttributes (source, &numFields);
    
    //qtss_printf("ElementNode::CountAttributes %s = %lu \n",GetNodeName() ,numFields);

    return numFields;
}

// Gets the number of elements of a node
UInt32 ElementNode::CountValues(QTSS_Object source, UInt32 apiID)
{
    //qtss_printf("------ElementNode::CountValues-------\n");
    UInt32 numFields = 0;
    
    (void) QTSS_GetNumValues (source, apiID, &numFields);

    //qtss_printf("ElementNode::CountValues %s = %lu \n",GetNodeName() ,numFields);

    return numFields;
}


//Gets the OSRef pointer of a node, 
//used to handle objects associated with the node.
//If the pointer does not exist,
//create a new OSRef object and initialize it, 
//then associate it with the node.

OSRef* ElementNode::GetOSRef(SInt32 index)
{   
    StrPtrLen   theName;
    OSRef*      resultPtr = NULL;
    
    resultPtr = fFieldOSRefPtrs[index];
//      Assert(resultPtr != NULL);
    if (resultPtr == NULL)
    {   
        fFieldOSRefPtrs[index] = NEW OSRef(); Assert(fFieldOSRefPtrs[index] != NULL); ElementNode_InsertPtr(fFieldOSRefPtrs[index],"ElementNode::GetOSRef NEW OSRef() fFieldOSRefPtrs ");   
        GetNameSPL(index,&theName); Assert(theName.Len != 0);
        //qtss_printf("ElementNode::GetOSRef index = %ld name = %s \n", index, theName.Ptr);
        fFieldOSRefPtrs[index]->Set(theName,(void *) index);
        resultPtr = fFieldOSRefPtrs[index];
    }

    
    return resultPtr;
}

//Set the OSRef pointer of a node to update the objects associated with the node.
void ElementNode::SetOSRef(SInt32 index, OSRef* refPtr)
{
    Assert  ((index >= 0) && (index < (SInt32) fNumFields));
    if      (fInitialized && (index >= 0) && (index < (SInt32) fNumFields)) 
        fFieldOSRefPtrs[index] = refPtr; 
}

// Gets the complete path of a node, including the names of all its ancestor nodes and the name of the node itself.
void ElementNode::GetFullPath(StrPtrLen *resultPtr)
{
    //qtss_printf("ElementNode::GetFullPath this node name %s \n",GetNodeName());
    Assert(fPathSPL.Ptr != NULL);   
    
    if (fPathSPL.Len != 0)
    {   
        resultPtr->Set(fPathSPL.Ptr,fPathSPL.Len);
        //qtss_printf("ElementNode::GetFullPath has path=%s\n",resultPtr->Ptr);
        return;
    }   
    
    ElementNode *parentPtr = GetParentNode();
    if (parentPtr != NULL)
    {
        StrPtrLen parentPath;
        parentPtr->GetFullPath(&parentPath);
        memcpy(fPathSPL.Ptr,parentPath.Ptr,parentPath.Len);
        fPathSPL.Ptr[parentPath.Len] = 0;
        fPathSPL.Len = parentPath.Len;
    }
    
    UInt32 nodeNameLen = GetNodeNameLen();
    if (nodeNameLen > 0)
    {
        fPathSPL.Len += nodeNameLen + 1;
        Assert(fPathSPL.Len < kmaxPathlen);
        if (fPathSPL.Len < kmaxPathlen)
        {   strcat(fPathSPL.Ptr, GetNodeName());
            strcat(fPathSPL.Ptr,"/"); 
            fPathSPL.Len = strlen(fPathSPL.Ptr);
        }
     }

    resultPtr->Set(fPathSPL.Ptr,fPathSPL.Len);
    //qtss_printf("ElementNode::GetFullPath element=%s received full path=%s \n",GetMyName(),resultPtr->Ptr);
}

//Add the information of the node itself to the response data for processing query requests.
void ElementNode::RespondWithSelfAdd(QTSS_StreamRef inStream, QueryURI *queryPtr)
{
    static char *nullErr = "(null)";
    Bool16 nullData = false;
    QTSS_Error err = QTSS_NoErr;
    char messageBuffer[1024] = "";
    StrPtrLen bufferSPL(messageBuffer);
    
    //qtss_printf("ElementNode::RespondWithSelfAdd NODE = %s index = %ld \n",GetNodeName(), (SInt32) index);
    
    if (!fInitialized) 
    {   //qtss_printf("ElementNode::RespondWithSelfAdd not Initialized EXIT\n");
        return;
    }
    if (NULL == queryPtr) 
    {   //qtss_printf("ElementNode::RespondWithSelfAdd NULL == queryPtr EXIT\n");
        return;
    }
    
    if (NULL == inStream) 
    {   //qtss_printf("ElementNode::RespondWithSelfAdd NULL == inStream EXIT\n");
        return;
    }
    
    char *dataPtr = GetMyElementDataPtr();
    if (NULL == dataPtr) 
    {   //qtss_printf("ElementNode::RespondWithSelfAdd NULL == dataPtr EXIT\n");
        dataPtr = nullErr;
        nullData = true;
    }
    
    queryPtr->SetQueryHasResponse();    



#if CHECKACCESS
/*
    //Mainly used to determine the permissions of the query object and return error messages when the permissions are insufficient
    
    StrPtrLen *accessParamsPtr=queryPtr->GetAccess();
    if (accessParamsPtr == NULL)
    {
            UInt32 result = 400;
            qtss_sprintf(messageBuffer,  "Attribute Access is required");
            (void) queryPtr->EvalQuery(&result, messageBuffer);
            return;
    }   
    
    accessFlags = queryPtr->GetAccessFlags();   
    if (0 == (accessFlags & qtssAttrModeWrite)) 
    {
            UInt32 result = 400;
            qtss_sprintf(messageBuffer,  "Attribute must have write access");
            (void) queryPtr->EvalQuery(&result, messageBuffer);
            return;
    }   
*/
#endif

    //This step determines non-null
    StrPtrLen* valuePtr = queryPtr->GetValue();
    OSCharArrayDeleter value(NewCharArrayCopy(valuePtr));
    if (!valuePtr || !valuePtr->Ptr)
    {   UInt32 result = 400;
        qtss_sprintf(messageBuffer,  "Attribute value is required");
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        return;
    }   

    //This step determines non-null
    StrPtrLen *typePtr = queryPtr->GetType();
    OSCharArrayDeleter dataType(NewCharArrayCopy(typePtr));
    if (!typePtr || !typePtr->Ptr)
    {   UInt32 result = 400;
        qtss_sprintf(messageBuffer,  "Attribute type is required");
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        return;
    }   

    //converts the string type pointed to by typePtr to the corresponding QTSS_AttrDataType enumeration 
    //and uses the Assert() function to assert it.
    QTSS_AttrDataType attrDataType = qtssAttrDataTypeUnknown;
    if (typePtr && typePtr->Len > 0)
    {   
        err = QTSS_TypeStringToType(dataType.GetObject(), &attrDataType);
        Assert(err == QTSS_NoErr);  
        //qtss_printf("ElementNode::RespondWithSelfAdd theType=%s typeID=%lu \n",dataType.GetObject(), attrDataType);
    }

    //converts the string pointed to by valuePtr to a value of the corresponding type and stores it in the valueBuff array.
    //qtss_printf("ElementNode::RespondWithSelfAdd theValue= %s theType=%s typeID=%lu \n",value.GetObject(), typePtr->Ptr, attrDataType);
    char valueBuff[2048] = "";
    UInt32 len = 2048;
    err = QTSS_StringToValue(value.GetObject(),attrDataType, valueBuff, &len);
    if (err) 
    {   UInt32 result = 400;
        qtss_sprintf(messageBuffer,  "QTSS_Error=%ld from ElementNode::RespondWithSelfAdd QTSS_ConvertStringToType",err);
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        return;
    }

    //determines whether the type of the current node is eNode
    if (GetMyFieldType() != eNode)
    {   UInt32 result = 500;
        qtss_sprintf(messageBuffer,  "Internal error");
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        return;
    }

    //a non-null and length judgment is made on the namePtr
    StrPtrLen *namePtr = queryPtr->GetName();
    OSCharArrayDeleter nameDeleter(NewCharArrayCopy(namePtr));
    if (!namePtr || !namePtr->Ptr || namePtr->Len == 0)
    {   UInt32 result = 400;
        qtss_sprintf(messageBuffer,  "Missing name for attribute");
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        return;
    }   
    
    //adds an attribute to the current node, where the attrDataType parameter is the QTSS_AttrDataType enumeration value from the previous conversion.
    err = QTSS_AddInstanceAttribute(GetSource(),nameDeleter.GetObject(), NULL, attrDataType);
    //qtss_printf("QTSS_AddInstanceAttribute(source=%lu, name=%s, NULL, %d, %lu)\n",GetSource(),nameDeleter.GetObject(),attrDataType,accessFlags);
    if (err) 
    {   UInt32 result = 400;
        if (err == QTSS_AttrNameExists)
        {   qtss_sprintf(messageBuffer,  "The name %s already exists QTSS_Error=%ld from QTSS_AddInstanceAttribute",nameDeleter.GetObject(),err);
        }
        else
        {   qtss_sprintf(messageBuffer,  "QTSS_Error=%ld from QTSS_AddInstanceAttribute",err);
        }
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        return;
    }
    QTSS_Object attrInfoObject;
    err = QTSS_GetAttrInfoByName(GetSource(), nameDeleter.GetObject(), &attrInfoObject);//Get the attribute information for the specified name
    if (err) 
    {   UInt32 result = 400;
        qtss_sprintf(messageBuffer,  "QTSS_Error=%ld from QTSS_GetAttrInfoByName",err);
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        return;
    }
    QTSS_AttributeID attributeID = 0;
    UInt32 attributeLen = sizeof(attributeID);
    err = QTSS_GetValue (attrInfoObject, qtssAttrID,0, &attributeID, &attributeLen);//get the unique identifier of the attribute attributeID
    if (err) 
    {   UInt32 result = 400;
        qtss_sprintf(messageBuffer,  "QTSS_Error=%ld from QTSS_GetValue",err);
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        return;
    }
    
    err = QTSS_SetValue (GetSource(), attributeID, 0, valueBuff, len);//set the value of the attribute to the current node
    if (err) 
    {   UInt32 result = 400;
        qtss_sprintf(messageBuffer,  "QTSS_Error=%ld from QTSS_SetValue",err);
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        return;
    }
    
}

void ElementNode::RespondWithSelf(QTSS_StreamRef inStream, QueryURI *queryPtr)
{
    //qtss_printf("ElementNode::RespondWithSelf = %s \n",GetNodeName());

    static char *nullErr = "(null)";
    if (QueryURI::kADDCommand == queryPtr->GetCommandID())//first determines whether the query command is an add command
    {
        if (GetMyFieldType() == eArrayNode)
        {   RespondToAdd(inStream, 0,queryPtr);
        }
        else
        {   RespondWithSelfAdd(inStream,queryPtr);
        }
        
        return;
        
    }
    
    if (QueryURI::kDELCommand == queryPtr->GetCommandID())//Determines if the query command is a delete command
    {   GetParentNode()->RespondToDel(inStream, GetMyKey(),queryPtr,true);
        return; 
    }
    
        
    //If the name of the current node is NULL or uninitialised, it is returned directly. 
    if (GetNodeName() == NULL) 
        
    {   //qtss_printf("ElementNode::RespondWithSelf Node = %s is Uninitialized no name so LEAVE\n",GetNodeName() );
        return;
    }
    
    if (!fInitialized) 
    {   //qtss_printf("ElementNode::RespondWithSelf not Initialized EXIT\n");
        return;
    }
    
    if (NULL == queryPtr) //If the queryPtr is empty , it is returned directly.
    {   //qtss_printf("ElementNode::RespondWithSelf NULL == queryPtr EXIT\n");
        return;
    }

    if (queryPtr->fNumFilters > 0)//If the query object queryPtr contains a filter, then a match is required to continue processing.
    {           
        Bool16 foundFilter = false;
        StrPtrLen*  theFilterPtr;
        for (SInt32 count = 0; count < queryPtr->fNumFilters; count ++)
        {
            theFilterPtr = queryPtr->GetFilter(count);
            if (theFilterPtr && theFilterPtr->Equal(StrPtrLen(GetMyName())) ) 
            {   foundFilter = true;
                //qtss_printf("ElementNode::RespondWithSelf found filter = ");PRINT_STR(theFilterPtr);
                break;
            }
        }
        if (!foundFilter) return;
    }
        
    StrPtrLen bufferSPL;

    UInt32 parameters = queryPtr->GetParamBits();
    parameters &= ~QueryURI::kRecurseParam; // clear recurse flag
    parameters &= ~QueryURI::kDebugParam; // clear verbose flag
    parameters &= ~QueryURI::kIndexParam; // clear index flag
    
    //Different processing is chosen depending on the type of node, 
    //outputting the node name and node value if it is an element node, 
    //or the node name if it is an array node.
    Bool16 isVerbosePath = 0 != (parameters & QueryURI::kVerboseParam);
    if (isVerbosePath) 
    {   
        parameters &= ~QueryURI::kVerboseParam; // clear verbose flag
        GetFullPath(&bufferSPL);
        (void)QTSS_Write(inStream, bufferSPL.Ptr, ::strlen(bufferSPL.Ptr), NULL, 0);
        //qtss_printf("ElementNode::RespondWithSelf Path=%s \n",bufferSPL.Ptr);
    }
    
    if (IsNodeElement())
    {   if (!isVerbosePath) // this node name already in path
        {
            (void)QTSS_Write(inStream, GetNodeName(), GetNodeNameLen(), NULL, 0);
            (void)QTSS_Write(inStream, "/", 1, NULL, 0);
            //qtss_printf("ElementNode::RespondWithSelf %s/ \n",GetNodeName());
        }
    }
    else
    {   //qtss_printf(" ****** ElementNode::RespondWithSelf NOT a node **** \n");
        (void)QTSS_Write(inStream, GetNodeName(), GetNodeNameLen(), NULL, 0);
        (void)QTSS_Write(inStream, "=", 1, NULL, 0);
        //qtss_printf(" %s=",GetNodeName());
            
        char *dataPtr = GetMyElementDataPtr();
        if (dataPtr == NULL)
        {
            (void)QTSS_Write(inStream, nullErr, ::strlen(nullErr), NULL, 0);
        }
        else
        {
            (void)QTSS_Write(inStream, dataPtr, ::strlen(dataPtr), NULL, 0);
        }
        //qtss_printf(" %s",buffer);
    
    }
    
    //Processing is carried out according to the query parameters.
    //If the query parameter contains the access rights parameter, 
    //the access rights value of the current node is output. 
    //If the query parameter contains a type parameter, the type information of the current node is output.
    
    if (parameters)
    {   (void)QTSS_Write(inStream, sParameterDelimeter, 1, NULL, 0);
        //qtss_printf(" %s",sParameterDelimeter);
    }
    
    if (parameters & QueryURI::kAccessParam)
    {   
        (void)QTSS_Write(inStream, sAccess, 2, NULL, 0);
        //qtss_printf(" %s",sAccess);
        (void)QTSS_Write(inStream, GetMyAccessData(),  GetMyAccessLen(), NULL, 0);
        //qtss_printf("%s",GetMyAccessData());
        parameters &= ~QueryURI::kAccessParam; // clear access flag
        
        if (parameters)
        {   (void)QTSS_Write(inStream, sListDelimeter, 1, NULL, 0);
            //qtss_printf("%s",sListDelimeter);
        }
    }
    
    if (parameters & QueryURI::kTypeParam)
    {   
        (void)QTSS_Write(inStream, sType, 2, NULL, 0);
        //qtss_printf(" %s",sType);
        char *theTypeString = GetMyAPI_TypeStr();
        if (theTypeString == NULL)
        {
            (void)QTSS_Write(inStream, nullErr, ::strlen(nullErr), NULL, 0);
        }
        else
        {
            (void)QTSS_Write(inStream,theTypeString,strlen(theTypeString), NULL, 0);
            //qtss_printf("%s",theTypeString);
        }
        
        parameters &= ~QueryURI::kTypeParam; // clear type flag
        
        if (parameters)
        {   (void)QTSS_Write(inStream, sListDelimeter, 1, NULL, 0);
            //qtss_printf("%s",sListDelimeter);
        }
    }
    queryPtr->SetQueryHasResponse();
    (void)QTSS_Write(inStream, "\n", 1, NULL, 0);
    //qtss_printf("\n");
    
}

void    ElementNode::RespondToAdd(QTSS_StreamRef inStream, SInt32 index,QueryURI *queryPtr)
{
    char messageBuffer[1024] = "";
    //Define a message buffer

    //If the attribute does not have a field, it is not allowed to add
    //qtss_printf("ElementNode::RespondToAdd NODE = %s index = %ld \n",GetNodeName(), (SInt32) index);
    if (GetNumFields() == 0) 
    {
        UInt32 result = 405;
        qtss_sprintf(messageBuffer,  "Attribute does not allow adding. Action not allowed");
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        //qtss_printf("ElementNode::RespondToAdd error = %s \n",messageBuffer);
        return;
    }   
    
    //Call the RespondWithSelfAdd function if the property type is a node
    if (GetFieldType(index) == eNode)
    {   RespondWithSelfAdd(inStream, queryPtr);
        return;
    }
        
    static char *nullErr = "(null)";
    Bool16 nullData = false;
    QTSS_Error err = QTSS_NoErr;
    StrPtrLen bufferSPL(messageBuffer);
    
    // If not initialized, return directly
    if (!fInitialized) 
    {   //qtss_printf("ElementNode::RespondToAdd not Initialized EXIT\n");
        return;
    }
    
    // If the query URI is empty, return directly
    if (NULL == queryPtr) 
    {   //qtss_printf("ElementNode::RespondToAdd NULL == queryPtr EXIT\n");
        return;
    }
    
    // If the stream is empty, return directly
    if (NULL == inStream) 
    {   //qtss_printf("ElementNode::RespondToAdd NULL == inStream EXIT\n");
        return;
    }
    
    // Get the attribute value pointer, set to nullErr if it's null
    char *dataPtr = GetElementDataPtr(index);
    if (NULL == dataPtr) 
    {   //qtss_printf("ElementNode::RespondToAdd NULL == dataPtr EXIT\n");
        //  return;
        dataPtr = nullErr;
        nullData = true;
    }
    
    // Set the query URI to have a response
    queryPtr->SetQueryHasResponse();    

     // get access rights, use the parameters if there are access parameters, otherwise use the property's rights
    UInt32 accessFlags = 0.
    StrPtrLen *accessParamsPtr = queryPtr->GetAccess().
    if (accessParamsPtr ! = NULL)
        accessFlags = queryPtr->GetAccessFlags().
    else
        accessFlags = GetAccessPermissions(index).
    
    // Get the value of the query URI
    StrPtrLen* valuePtr = queryPtr->GetValue().
    OSCharArrayDeleter value(NewCharArrayCopy(valuePtr)).
    if (!valuePtr || !valuePtr->Ptr)
    { UInt32 result = 400.
        qtss_sprintf(messageBuffer, "No value found").
        (void) queryPtr->EvalQuery(&result, messageBuffer).
        return.
    }   

    // Convert the value of the query URI to a string
    char valueBuff[2048] = "".
    UInt32 len = 2048.
    err = QTSS_StringToValue(value.GetObject(), GetAPI_Type(index), valueBuff, &len).
    if (err) 
    { UInt32 result = 400.
        qtss_sprintf(messageBuffer, "QTSS_Error=%ld from QTSS_ConvertStringToType",err).
        (void) queryPtr->EvalQuery(&result, messageBuffer).
        return.
    }

    // If the property type is not a node, do the following
    if (GetFieldType(index) ! = eNode)
    {   
        // Get the type of the query URI and compare it to the attribute type, or return an error if it is not the same
        OSCharArrayDeleter typeDeleter(NewCharArrayCopy(queryPtr->GetType())).
        StrPtrLen theQueryType(typeDeleter.GetObject()).

        if (typeDeleter.GetObject())
        {
            StrPtrLen attributeString(GetAPI_TypeStr(index)).
            if (!attributeString.Equal(theQueryType))
            { UInt32 result = 400.
                qtss_sprintf(messageBuffer, "Type %s does not match attribute type %s",typeDeleter.GetObject(), attributeString.Ptr).
                (void) queryPtr->EvalQuery(&result, messageBuffer).
                return.
            }   
        }
        
        // Get the value of the attribute and compare it with the value of the query URI, or return an error if it is not the same
        QTSS_Object source = GetSource().

        UInt32 tempBuff.
        UInt32 attributeLen = sizeof(tempBuff).
        (void) QTSS_GetValue(source, GetAPI_ID(index), 0, &tempBuff, &attributeLen).
        if (attributeLen ! = len)
        { UInt32 result = 400.
            qtss_sprintf(messageBuffer, "Data length %lu does not match attribute len %lu",len, attributeLen).
            (void) queryPtr->EvalQuery(&result, messageBuffer).
            return.
        }

        // Get the number of attributes and add the values to the attribute
        UInt32 numValues = 0.
        err = QTSS_GetNumValues (source,  GetAPI_ID(index), &numValues);
        if (err) 
        {   UInt32 result = 400;
            qtss_sprintf(messageBuffer,  "QTSS_Error=%ld from QTSS_GetNumValues",err);
            (void) queryPtr->EvalQuery(&result, messageBuffer);
            return;
        }
        
        //qtss_printf("ElementNode::RespondToAdd QTSS_SetValue object=%lu attrID=%lu, index = %lu valuePtr=%lu valuelen =%lu \n",GetSource(),GetAPI_ID(index), GetAttributeIndex(index), valueBuff,len);
        err = QTSS_SetValue (source, GetAPI_ID(index), numValues, valueBuff, len);
        if (err) 
        {   UInt32 result = 400;
            qtss_sprintf(messageBuffer,  "QTSS_Error=%ld from QTSS_SetValue",err);
            (void) queryPtr->EvalQuery(&result, messageBuffer);
            return;
        }

    }
    
}

void    ElementNode::RespondToSet(QTSS_StreamRef inStream, SInt32 index,QueryURI *queryPtr)
{
    static char *nullErr = "(null)";//If the data pointer is NULL, this string will be used instead
    Bool16 nullData = false;
    QTSS_Error err = QTSS_NoErr;
    char messageBuffer[1024] = "";
    StrPtrLen bufferSPL(messageBuffer);
    
    //qtss_printf("ElementNode::RespondToSet NODE = %s index = %ld \n",GetNodeName(), (SInt32) index);
    
    if (!fInitialized) 
    {   
        // If the node is not initialized, return directly
        // qtss_printf("ElementNode::RespondToSet not Initialized EXIT\n").
        return.
    }
    if (NULL == queryPtr) 
    {   
        // If the query object is NULL, return it directly
        // qtss_printf("ElementNode::RespondToSet NULL == queryPtr EXIT\n").
        return.
    }

    if (NULL == inStream) 
    {   
        // If the input stream is NULL, return it directly
        // qtss_printf("ElementNode::RespondToSet NULL == inStream EXIT\n").
        return.
    }

    char *dataPtr = GetElementDataPtr(index).
    if (NULL == dataPtr) 
    {   
        // If the data pointer is NULL, the string pointed to by nullErr will be used instead
        // qtss_printf("ElementNode::RespondToSet NULL == dataPtr EXIT\n").
        // return.
        dataPtr = nullErr.
        nullData = true.
    }

    // Set the query flag
    queryPtr->SetQueryHasResponse().    

    // Get the query type
    OSCharArrayDeleter typeDeleter(NewCharArrayCopy(queryPtr->GetType())).
    StrPtrLen theQueryType(typeDeleter.GetObject()).

    if (theQueryType.Len > 0)
    {   
        // If the query type is not equal to the attribute type, return an error message
        StrPtrLen attributeString(GetAPI_TypeStr(index)).
        if (!attributeString.Equal(theQueryType))
        {   
            UInt32 result = 400.
            qtss_sprintf(messageBuffer, "Type %s does not match attribute type %s",typeDeleter.GetObject(), attributeString.Ptr).
            (void) queryPtr->EvalQuery(&result, messageBuffer).
            return.
        }   
    }

    if (0 == (GetAccessPermissions(index) & qtssAttrModeWrite)) 
    {
        // If the attribute is read-only, return an error message
        UInt32 result = 400.
        qtss_sprintf(messageBuffer, "Attribute is read only. Action not allowed").
        (void) queryPtr->EvalQuery(&result, messageBuffer).
        return.
    }

    if (GetFieldType(index) == eNode)
    {   
        // If the property type is eNode, return an error message
        UInt32 result = 400.
        qtss_sprintf(messageBuffer, "Set of type %s not allowed",typeDeleter.GetObject()).
        //qtss_printf("ElementNode::RespondToSet (GetFieldType(index) == eNode) %s\n",messageBuffer).
        (void) queryPtr->EvalQuery(&result, messageBuffer).
        return.
    }
    else do 
    {   
        // Convert query values to property types
        StrPtrLen* valuePtr = queryPtr->GetValue().
        if (!valuePtr || !valuePtr->Ptr) break.

        char valueBuff[2048] = "".
        UInt32 len = 2048.
        OSCharArrayDeleter value(NewCharArrayCopy(valuePtr)).

        //qtss_printf("ElementNode::RespondToSet valuePtr->Ptr= %s\n",value.GetObject()).

        err = QTSS_StringToValue(value.GetObject(),GetAPI_Type(index), valueBuff, &len).
        if (err) 
        { //qtss_sprintf(messageBuffer, "QTSS_Error=%ld from QTSS_ConvertStringToType",err).
            break.
        }

        //qtss_printf("ElementNode::RespondToSet QTSS_SetValue object=%lu attrID=%lu, index = %lu valuePtr=%lu valuelen =%lu \n",GetSource(),GetAPI_ ID(index), GetAttributeIndex(index), valueBuff,len).
        // Set the attribute value
        err = QTSS_SetValue (GetSource(), GetAPI_ID(index), GetAttributeIndex(index), valueBuff, len).
        if (err) 
        { //qtss_sprintf(messageBuffer, "QTSS_Error=%ld from QTSS_SetValue",err).
            break.
        }   

    } while (false).

    if (err ! = QTSS_NoErr)
    {   
        // If an error occurs, return the error message
        UInt32 result = 400.
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        //qtss_printf("ElementNode::RespondToSet %s len = %lu ",messageBuffer, result);
        return;
    }
    
}

//The main function is to delete properties
void    ElementNode::RespondToDel(QTSS_StreamRef inStream, SInt32 index,QueryURI *queryPtr,Bool16 delAttribute)
{
    //First some pointers are checked for non-null, 
    //then the GetNumFields function checks if the attribute to be deleted exists and returns an error message if the attribute cannot be deleted
    static char *nullErr = "(null)";
    Bool16 nullData = false;
    QTSS_Error err = QTSS_NoErr;
    char messageBuffer[1024] = "";
    StrPtrLen bufferSPL(messageBuffer);
    
    //qtss_printf("ElementNode::RespondToDel NODE = %s index = %ld \n",GetNodeName(), (SInt32) index);
    
    
    if (!fInitialized) 
    {   //qtss_printf("ElementNode::RespondToDel not Initialized EXIT\n");
        return;
    }
    if (NULL == queryPtr) 
    {   //qtss_printf("ElementNode::RespondToDel NULL == queryPtr EXIT\n");
        return;
    }
    
    if (NULL == inStream) 
    {   //qtss_printf("ElementNode::RespondToDel NULL == inStream EXIT\n");
        return;
    }

    //qtss_printf("ElementNode::RespondToDel NODE = %s index = %ld \n",GetNodeName(), (SInt32) index);
    if (    GetNumFields() == 0 
        || ( 0 == (GetAccessPermissions(index) & qtssAttrModeDelete) && GetMyFieldType() == eArrayNode && GetNumFields() == 1)  
        || ( 0 == (GetAccessPermissions(index) & qtssAttrModeDelete) && GetMyFieldType() != eArrayNode)  
        ) 
    {
        UInt32 result = 405;
        qtss_sprintf(messageBuffer,  "Attribute does not allow deleting. Action not allowed");
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        //qtss_printf("ElementNode::RespondToDel error = %s \n",messageBuffer);
        return;
    }      
    
    //gets the data pointer to the property to be deleted and checks it for non-null.
    //If the data pointer is null, the null pointer is represented by a string
    char *dataPtr = GetElementDataPtr(index);
    if (NULL == dataPtr) 
    {   //qtss_printf("ElementNode::RespondToDel NULL == dataPtr EXIT\n");
        //  return;
        dataPtr = nullErr;
        nullData = true;
    }
    
    queryPtr->SetQueryHasResponse();    

    //remove the attribute
    if (GetMyFieldType() == eArrayNode && !delAttribute)
    {   
        UInt32 result = 500;
        err = QTSS_RemoveValue (GetSource(),GetAPI_ID(index), GetAttributeIndex(index));
        qtss_sprintf(messageBuffer,  "QTSS_Error=%ld from QTSS_RemoveValue", err);
        //qtss_printf("ElementNode::RespondToDel QTSS_RemoveValue object=%lu attrID=%lu index=%lu %s\n",GetSource(),GetAPI_ID(index),GetAttributeIndex(index),messageBuffer);
        if (err) 
        {   (void) queryPtr->EvalQuery(&result, messageBuffer);
        }   
    }
    else  
    {   
        //qtss_printf("ElementNode::RespondToDel QTSS_RemoveInstanceAttribute object=%lu attrID=%lu \n",GetSource(),GetAPI_ID(index));
        err = QTSS_RemoveInstanceAttribute(GetSource(),GetAPI_ID(index));       
        if (err) 
        {
            qtss_sprintf(messageBuffer,  "QTSS_Error=%ld from QTSS_RemoveInstanceAttribute",err);
        }   
            
    } 
    
    if (err != QTSS_NoErr)
    {   UInt32 result = 400;
        (void) queryPtr->EvalQuery(&result, messageBuffer);
        //qtss_printf("ElementNode::RespondToDel %s len = %lu ",messageBuffer, result);
        return;
    }
    
}

//Find the filter in the queryPtr object for the node whose name is specified by the index parameter
Bool16 ElementNode::IsFiltered(SInt32 index,QueryURI *queryPtr)
{
    Bool16 foundFilter = false;
    StrPtrLen*  theFilterPtr;
    for (SInt32 count = 0; count < queryPtr->fNumFilters; count ++)
    {
        theFilterPtr = queryPtr->GetFilter(count);
        if (theFilterPtr && theFilterPtr->Equal(StrPtrLen(GetName(index))) ) 
        {   foundFilter = true;
            break;
        }
    }
    return foundFilter;
}
    
//This function is a function used to handle the add operation and accepts two parameters: 
//one for the element to be added and another for the container to store the element. 
//The function completes the add operation by inserting the element to be added at the end of the container.

void ElementNode::RespondToGet(QTSS_StreamRef inStream, SInt32 index,QueryURI *queryPtr)
{
    static char *nullErr = "(null)";
    Bool16 nullData = false;
    
    //qtss_printf("ElementNode::RespondToGet NODE = %s index = %ld \n",GetNodeName(), (SInt32) index);
    
    if (!fInitialized) 
    {   //qtss_printf("ElementNode::RespondToGet not Initialized EXIT\n");
        return;
    }
    if (NULL == queryPtr) 
    {   //qtss_printf("ElementNode::RespondToGet NULL == queryPtr EXIT\n");
        return;
    }
    
    if (NULL == inStream) 
    {   //qtss_printf("ElementNode::RespondToGet NULL == inStream EXIT\n");
        return;
    }
    
    char *dataPtr = GetElementDataPtr(index);
    if (NULL == dataPtr) 
    {   //qtss_printf("ElementNode::RespondToGet NULL == dataPtr EXIT\n");
        //  return;
        dataPtr = nullErr;
        nullData = true;
    }
    
    StrPtrLen bufferSPL;
    
    UInt32 parameters = queryPtr->GetParamBits();
    parameters &= ~QueryURI::kRecurseParam; // clear verbose flag
    parameters &= ~QueryURI::kDebugParam; // clear debug flag
    parameters &= ~QueryURI::kIndexParam; // clear index flag
    
    //qtss_printf("ElementNode::RespondToGet QTSS_SetValue object=%lu attrID=%lu, index = %lu \n",GetSource(),GetAPI_ID(index), GetAttributeIndex(index));


    if ((parameters & QueryURI::kVerboseParam) ) 
    {   
        parameters &= ~QueryURI::kVerboseParam; // clear verbose flag
        GetFullPath(&bufferSPL);
        (void)QTSS_Write(inStream, bufferSPL.Ptr, ::strlen(bufferSPL.Ptr), NULL, 0);
        //qtss_printf("ElementNode::RespondToGet Path=%s \n",bufferSPL.Ptr);
    }
    

    (void)QTSS_Write(inStream, GetName(index), GetNameLen(index), NULL, 0);
    //qtss_printf("ElementNode::RespondToGet %s:len = %lu",GetName(index),(UInt32) GetNameLen(index));
        
    if (IsNodeElement(index))
    {
        (void)QTSS_Write(inStream, "/\"", 1, NULL, 0);
        //qtss_printf(" %s/\"",GetNodeName());
    }
    else
    {
        if (nullData)
        {
            (void)QTSS_Write(inStream, "=", 1, NULL, 0);
            (void)QTSS_Write(inStream, dataPtr, ::strlen(dataPtr), NULL, 0);
        }
        else
        {
            (void)QTSS_Write(inStream, "=\"", 2, NULL, 0);
            (void)QTSS_Write(inStream, dataPtr, ::strlen(dataPtr), NULL, 0);
            (void)QTSS_Write(inStream, "\"", 1, NULL, 0);
        }
    }
    
    //qtss_printf(" %s len = %d ",buffer, ::strlen(buffer));
    //DebugShowFieldDataType(index);
    
    if (parameters)
    {   (void)QTSS_Write(inStream, sParameterDelimeter, 1, NULL, 0);
        //qtss_printf(" %s",sParameterDelimeter);
    }
    
    if (parameters & QueryURI::kAccessParam)
    {   
        (void)QTSS_Write(inStream, sAccess, 2, NULL, 0);
        //qtss_printf(" %s",sAccess);
        (void)QTSS_Write(inStream, GetAccessData(index),  GetAccessLen(index), NULL, 0);
        //qtss_printf("%s",GetAccessData(index));
        parameters &= ~QueryURI::kAccessParam; // clear access flag
        
        if (parameters)
        {   (void)QTSS_Write(inStream, sListDelimeter, 1, NULL, 0);
            //qtss_printf("%s",sListDelimeter);
        }
    }
    
    if (parameters & QueryURI::kTypeParam)
    {   
        (void)QTSS_Write(inStream, sType, 2, NULL, 0);
        //qtss_printf(" %s",sType);
        char* typeStringPtr = GetAPI_TypeStr(index);
        if (typeStringPtr == NULL)
        {
            //qtss_printf("ElementNode::RespondToGet typeStringPtr is NULL for type = %s \n", typeStringPtr);
            (void)QTSS_Write(inStream, nullErr, ::strlen(nullErr), NULL, 0);
        }
        else
        {
            //qtss_printf("ElementNode::RespondToGet type = %s \n", typeStringPtr);
            (void)QTSS_Write(inStream,typeStringPtr,strlen(typeStringPtr), NULL, 0);
        }

        parameters &= ~QueryURI::kTypeParam; // clear type flag
        
        if (parameters)
        {   (void)QTSS_Write(inStream, sListDelimeter, 1, NULL, 0);
            //qtss_printf("%s",sListDelimeter);
        }
    }
    
    
    (void)QTSS_Write(inStream, "\n", 1, NULL, 0);
    //qtss_printf(" %s","\n");
    
    queryPtr->SetQueryHasResponse();

}

//The RespondToKey function calls the appropriate RespondToGet, RespondToSet, RespondToAdd or RespondToDel functions depending on the command type in the QueryURI object, 

void    ElementNode::RespondToKey(QTSS_StreamRef inStream, SInt32 index,QueryURI *queryPtr)
{
    SInt32 command = queryPtr->GetCommandID();
    //qtss_printf("ElementNode::RespondToKey command = %ld node =%s index=%ld\n",command, GetNodeName(),index);
    
    switch (command)
    {
        case QueryURI::kGETCommand: RespondToGet(inStream,index,queryPtr);
        break;
        
        case QueryURI::kSETCommand: RespondToSet(inStream,index,queryPtr);
        break;
        
        case QueryURI::kADDCommand: RespondToAdd(inStream,index,queryPtr);
        break;
        
        case QueryURI::kDELCommand: RespondToDel(inStream,index,queryPtr,false);
        break;
    }
        
}

//the RespondWithNodeName function returns the path to the current node.
void ElementNode::RespondWithNodeName(QTSS_StreamRef inStream, QueryURI * /*unused queryPtr*/) 
{
    
    //qtss_printf("ElementNode::RespondWithNodeName NODE = %s \n",GetNodeName());
    
    if (!fInitialized) 
    {    //qtss_printf("ElementNode::RespondWithNodeName not Initialized EXIT\n");
         return;
    }
        
    StrPtrLen fullPathSPL;
    GetFullPath(&fullPathSPL);
    
    (void)QTSS_Write(inStream, "Container=\"/", ::strlen("Container=\"/"), NULL, 0);
    
    (void)QTSS_Write(inStream, fPathSPL.Ptr, ::strlen(fPathSPL.Ptr), NULL, 0);
    //qtss_printf("ElementNode::RespondWithNodeName Path=%s \n",fPathSPL.Ptr);
    
    (void)QTSS_Write(inStream, "\"", 1, NULL, 0);
    //qtss_printf("\"");     
    
    (void)QTSS_Write(inStream, "\n", 1, NULL, 0);
    //qtss_printf("\n");
    
}

//Based on the path information in the QueryURI object, 
//the node is recursively queried to the specified node and the RespondToQuery function is called to process it.
//If the current node is a leaf node and the path has reached the end, 
//the RespondWithNodeName function is called to return the current node path,
//otherwise it continues to recursively query the next level node.
//If the current node is not a leaf node and the QueryURI object contains a filter, 
//the corresponding child node is queried and processed according to the filter, 
//otherwise the RespondToKey function is called.  

void ElementNode::RespondWithSingleElement(QTSS_StreamRef inStream, QueryURI *queryPtr, StrPtrLen *currentSegmentPtr) 
{
        
    //qtss_printf("ElementNode::RespondWithSingleElement Current Node = %s\n",GetNodeName() );
    
    if (!fInitialized) 
    {   
        //qtss_printf("ElementNode::RespondWithSingleElement failed Not Initialized %s\n",GetNodeName() );
        return;
    }

    if (GetNodeName() == NULL) 
    {   
        //qtss_printf("ElementNode::RespondWithSingleElement Node = %s is Uninitialized LEAVE\n",GetNodeName() );
        return;
    }

    Assert(queryPtr != NULL);
    Assert(currentSegmentPtr != NULL);
    Assert(currentSegmentPtr->Ptr != 0);
    Assert(currentSegmentPtr->Len != 0);
    
    SInt32 key = ResolveSPLKeyToIndex(currentSegmentPtr);
    //qtss_printf("ElementNode::RespondWithSingleElement key = %ld\n",key);
    //qtss_printf("currentSegmentPtr="); PRINT_STR(currentSegmentPtr);
    
    if (key < 0) 
    {
        //qtss_printf("ElementNode::RespondWithSingleElement key = %ld NOT FOUND no ELEMENT\n",key);
        return;
    }
    
    if ((queryPtr == NULL) || (currentSegmentPtr == NULL) || (currentSegmentPtr->Ptr == NULL) || (currentSegmentPtr->Len == 0))
    {
        //qtss_printf("ElementNode::RespondWithSingleElement currentSegmentPtr || queryPtr = NULL\n");
        return;
    }
    
    //add here
    
    StrPtrLen nextSegment;
    ( void)queryPtr->NextSegment(currentSegmentPtr, &nextSegment);

    if ( (nextSegment.Len == 0)  && !queryPtr->RecurseParam() ) // only respond if we are at the end of the path
    {
        //qtss_printf("currentSegmentPtr="); PRINT_STR(currentSegmentPtr);
        //qtss_printf("nextSegment="); PRINT_STR(&nextSegment);
        //qtss_printf("ElementNode::RespondWithSingleElement Current Node = %s Call RespondWithNodeName\n",GetNodeName() );
        if (QueryURI::kGETCommand == queryPtr->GetCommandID())
            RespondWithNodeName( inStream,queryPtr);
    }
            
    if (IsNodeElement(key))
    {
        ElementNode *theNodePtr = (ElementNode *)GetElementDataPtr(key);
        if (theNodePtr) 
        {   
            //qtss_printf("ElementNode::RespondWithSingleElement Current Node = %s Call RespondToQuery\n",GetNodeName() );
            theNodePtr->RespondToQuery(inStream, queryPtr,currentSegmentPtr);
        }
    }
    else
    {
        //qtss_printf("ElementNode::RespondWithSingleElement call RespondToKey\n");
        if ( (queryPtr->fNumFilters > 0) && (QueryURI::kGETCommand == queryPtr->GetCommandID()) )
        {
            StrPtrLen*  theFilterPtr;
            SInt32 index;
            for (SInt32 count = 0; count < queryPtr->fNumFilters; count ++)
            {
                theFilterPtr = queryPtr->GetFilter(count);
                index = ResolveSPLKeyToIndex(theFilterPtr);
                if (index < 0) continue;
                RespondToKey(inStream, index, queryPtr);
                //qtss_printf("ElementNode::RespondWithSingleElement found filter = ");PRINT_STR(theFilterPtr);
                break;
            }
            //qtss_printf("ElementNode::RespondWithSingleElement found filter = ?");PRINT_STR(theFilterPtr);
        }
        else
        {   RespondToKey(inStream, key, queryPtr);
        }
    }
    
}

//Returns the values of all element nodes.
void ElementNode::RespondWithAllElements(QTSS_StreamRef inStream, QueryURI *queryPtr, StrPtrLen *currentSegmentPtr)
{
    //qtss_printf("ElementNode::RespondWithAllElements %s\n",GetNodeName());
    //qtss_printf("ElementNode::RespondWithAllElements fDataFieldsStop = %d \n",fDataFieldsStop);
    
    if (GetNodeName() == NULL) 
    {   //qtss_printf("ElementNode::RespondWithAllElements %s is Uninitialized LEAVE\n",GetNodeName());
        return;
    }

    if (!fInitialized) 
    {   //qtss_printf("ElementNode::RespondWithAllElements %s is Uninitialized LEAVE\n",GetNodeName());
        return;
    }
    
    StrPtrLen nextSegment;
    ( void)queryPtr->NextSegment(currentSegmentPtr, &nextSegment);

    if ( (nextSegment.Len == 0 || nextSegment.Ptr == 0)  )  // only respond if we are at the end of the path
        if (QueryURI::kGETCommand == queryPtr->GetCommandID())
            RespondWithNodeName( inStream,queryPtr);
        
    if ( (queryPtr->fNumFilters > 0) && (QueryURI::kGETCommand == queryPtr->GetCommandID()) )
    {
        StrPtrLen*  theFilterPtr;
        SInt32 index;
        for (SInt32 count = 0; count < queryPtr->fNumFilters; count ++)
        {
            theFilterPtr = queryPtr->GetFilter(count);
            index = ResolveSPLKeyToIndex(theFilterPtr);
            if (index < 0) continue;
                        
            if ( (nextSegment.Len == 0 || nextSegment.Ptr == 0) )
            {   if ( (!IsNodeElement(index)) || (IsNodeElement(index) && queryPtr->RecurseParam() ))    // only respond if we are at the end of the path 
                    RespondToKey(inStream, index, queryPtr);    
            }

            //qtss_printf("ElementNode::RespondWithAllElements found filter = ");PRINT_STR(theFilterPtr);
        }
    }
    else
    {
        UInt32 index = 0;
        for ( index = 0; !IsStopItem(index) ;index++)
        {
            //qtss_printf("RespondWithAllElements = %d \n",index);
            //qtss_printf("ElementNode::RespondWithAllElements nextSegment="); PRINT_STR(&nextSegment);
            
            if ( (nextSegment.Len == 0 || nextSegment.Ptr == 0) )
            {   if ( (!IsNodeElement(index)) || (IsNodeElement(index) && queryPtr->RecurseParam() ) )   // only respond if we are at the end of the path
                    RespondToKey(inStream, index, queryPtr);    
            }
    
        }
    }   
    
    UInt32 index = 0;
    for ( index = 0; !IsStopItem(index);index++)
    {
        
        if  (IsNodeElement(index)) 
        {   
            //qtss_printf("ElementNode::RespondWithAllElements FoundNode\n");
            //qtss_printf("ElementNode::RespondWithAllElements currentSegmentPtr="); PRINT_STR(currentSegmentPtr);
            //qtss_printf("ElementNode::RespondWithAllElements nextSegment="); PRINT_STR(&nextSegment);
            ElementNode *theNodePtr = (ElementNode *)GetElementDataPtr(index);
            
            if (theNodePtr ) 
            {   
                //qtss_printf("ElementNode::RespondWithAllElements Current Node = %s Call RespondToQuery\n",GetNodeName() );
                theNodePtr->RespondToQuery(inStream, queryPtr,&nextSegment);
            }
            else
            {
                //qtss_printf("ElementNode::RespondWithAllElements Current Node index= %lu NULL = %s\n",index,GetName(index));
            
            }           
        }
    }
}


//Returns the values of all nodes.
void ElementNode::RespondWithAllNodes(QTSS_StreamRef inStream, QueryURI *queryPtr, StrPtrLen *currentSegmentPtr) 
{
    
    //qtss_printf("ElementNode::RespondWithAllNodes %s\n",GetNodeName());

    if (!fInitialized) 
    {   //qtss_printf("ElementNode::RespondWithAllNodes %s is Uninitialized LEAVE\n",GetNodeName());
        return;
    }
    
    if (GetNodeName() == NULL) 
    {   //qtss_printf("ElementNode::RespondWithAllNodes %s is Uninitialized LEAVE\n",GetNodeName());
        return;
    }
    
    StrPtrLen nextSegment;
    ( void)queryPtr->NextSegment(currentSegmentPtr, &nextSegment);  
    
    for(SInt32 index = 0; !IsStopItem(index); index ++)
    {
        if (!queryPtr->RecurseParam() && (currentSegmentPtr->Len == 0) ) 
        {   Assert(0);
            break;
        }
        

        if  (IsNodeElement(index)) 
        {   
            //qtss_printf("ElementNode::RespondWithAllNodes FoundNode\n");
            //qtss_printf("ElementNode::RespondWithAllNodes currentSegmentPtr="); PRINT_STR(currentSegmentPtr);
            ElementNode *theNodePtr = (ElementNode *)GetElementDataPtr(index);
            //qtss_printf("ElementNode::RespondWithAllNodes nextSegment="); PRINT_STR(&nextSegment);
            if (theNodePtr) 
            {   
                //qtss_printf("ElementNode::RespondWithAllNodes Current Node = %s Call RespondToQuery\n",GetNodeName() );
                theNodePtr->RespondToQuery(inStream, queryPtr,currentSegmentPtr);           
            }
        }
    }
}

//Returns the corresponding result based on the query URI.
void ElementNode::RespondToQuery(QTSS_StreamRef inStream, QueryURI *queryPtr,StrPtrLen *currentPathPtr)
{
    //qtss_printf("----- ElementNode::RespondToQuery ------\n");
    //qtss_printf("ElementNode::RespondToQuery NODE = %s\n",GetNodeName());

    Assert(NULL != queryPtr);
    Assert(NULL != currentPathPtr);

    if (!fInitialized) 
    {   //qtss_printf("ElementNode::RespondToQuery %s is Uninitialized LEAVE\n",GetNodeName());
        return;
    
    }
        
    if (GetNodeName() == NULL) 
    {   //qtss_printf("ElementNode::RespondToQuery %s is Uninitialized LEAVE\n",GetNodeName());
        return;
    }
        
    
    Bool16 recurse = queryPtr->RecurseParam() ;
    Bool16 doAllNext = false;
    Bool16 doAllNextNext = false;
    StrPtrLen nextSegment;
    StrPtrLen nextnextSegment;
    StrPtrLen nextnextnextSegment;
    
    
    if (queryPtr && currentPathPtr) do
    {   
        ( void)queryPtr->NextSegment(currentPathPtr, &nextSegment);
        ( void)queryPtr->NextSegment(&nextSegment, &nextnextSegment);
        ( void)queryPtr->NextSegment(&nextnextSegment, &nextnextnextSegment);
            
        //qtss_printf("ElementNode::RespondToQuery currentPathPtr="); PRINT_STR( currentPathPtr);
        //qtss_printf("ElementNode::RespondToQuery nextSegment="); PRINT_STR( &nextSegment);
        //qtss_printf("ElementNode::RespondToQuery nextnextSegment="); PRINT_STR( &nextnextSegment);

         // recurse param is set and this is the end of the path
        if  (recurse && ( (0 == currentPathPtr->Len) || (0 == nextSegment.Len) ) )
        {   // admin 
            //qtss_printf("ElementNode::RespondToQuery 1)RespondToQuery -> RespondWithAllElements ") ;PRINT_STR( GetNodeNameSPL());
            RespondWithAllElements(inStream, queryPtr, &nextSegment); 
            break;                          
        }

         // recurse param is not set and this is the end of the path
        if  ( (!recurse && ( (0 == currentPathPtr->Len) || (0 == nextSegment.Len) ) )
            )
        {   // admin 
            //qtss_printf("ElementNode::RespondToQuery 2)RespondToQuery -> RespondWithSelf ") ;PRINT_STR( GetNodeNameSPL());
            if (fIsTop)
                (void)QTSS_Write(inStream, "Container=\"/\"\n", ::strlen("Container=\"/\"\n"), NULL, 0);

            RespondWithSelf(inStream, queryPtr);
            break;                          
        }


        doAllNext = ElementNode_DoAll(&nextSegment);    
        doAllNextNext = ElementNode_DoAll(&nextnextSegment);    
        
        if  (   doAllNext && (0 == nextnextSegment.Len) )
        {   // admin/*
            //qtss_printf("ElementNode::RespondToQuery 3)RespondToQuery -> RespondWithAllElements ");PRINT_STR( &nextSegment);
            RespondWithAllElements(inStream, queryPtr, &nextSegment);       
            break;                          
        }
        
        if  (   doAllNext && doAllNextNext) 
        {   // admin/*/*
            //qtss_printf("ElementNode::RespondToQuery 4)RespondToQuery -> RespondWithAllNodes ");PRINT_STR( currentPathPtr);
            RespondWithAllNodes(inStream, queryPtr, &nextSegment); 
            break;                          
        }
                

        if  (   doAllNext && (nextnextSegment.Len > 0) )
        {   // admin/*/attribute
            //qtss_printf("ElementNode::RespondToQuery 5)RespondToQuery -> RespondWithAllNodes  ");PRINT_STR( &nextSegment);
            RespondWithAllNodes(inStream, queryPtr, &nextSegment); 
            break;                          
        }
        
        // admin/attribute
        //qtss_printf("ElementNode::RespondToQuery 6)RespondToQuery -> RespondWithSingleElement ");PRINT_STR( &nextSegment);
        RespondWithSingleElement(inStream, queryPtr,&nextSegment);

    } while (false);

    if (QueryURI::kGETCommand != queryPtr->GetCommandID() && (!queryPtr->fIsPref))
    {   queryPtr->fIsPref = IsPreferenceContainer(GetMyName(),NULL);
    }
    //qtss_printf("ElementNode::RespondToQuery LEAVE\n");
}

//The purpose of this function is to set nodes. It accepts QueryURI objects, current path, and QTSS_ Initialize_ Params object as a parameter.
//The function first performs some initialization operations on parameters and variables.
//Next, it retrieves the next path segment and determines whether all elements need to be set.
//If all elements need to be set, call the SetUpAllElements() function.
//If recursion is required, call the SetUpAllNodes() function.
//Otherwise, obtain the index of the next path segment and call the SetUpSingleElement() function.

void ElementNode::SetupNodes(QueryURI *queryPtr,StrPtrLen *currentPathPtr,QTSS_Initialize_Params *initParams)
{
    //qtss_printf("----- ElementNode::SetupNodes ------ NODE = %s\n", GetNodeName()); 
    //qtss_printf("ElementNode::SetupNodes currentPathPtr ="); PRINT_STR(currentPathPtr);
    if (fSelfPtr == NULL) 
    {   //qtss_printf("******* ElementNode::SetupNodes (fSelfPtr == NULLL) \n");
    }
    Assert(NULL != queryPtr);
    Assert(NULL != currentPathPtr);
    
    if (queryPtr && currentPathPtr) do
    {   
        Bool16 doAll = false;
        StrPtrLen nextSegment;

        ( void)queryPtr->NextSegment(currentPathPtr, &nextSegment);
        doAll = ElementNode_DoAll(&nextSegment);
        
        StrPtrLen *thisNamePtr = GetNodeNameSPL();
        //qtss_printf("ElementNode::SetupNodes thisNamePtr="); PRINT_STR(thisNamePtr);

        if (    ( (doAll) && (currentPathPtr->Equal(*thisNamePtr) || ElementNode_DoAll(currentPathPtr)) )
             || (queryPtr->RecurseParam())
            )
        {
            SetUpAllElements(queryPtr,currentPathPtr, &nextSegment, initParams);
            break;
        }
                 
        SInt32 index = ResolveSPLKeyToIndex(&nextSegment);
        if (index < 0) 
        {   
            //qtss_printf("ElementNode::SetupNodes FAILURE ResolveSPLKeyToIndex = %d NODE = %s\n", index, GetNodeName());
            break;
        }

        SetUpAllNodes(queryPtr, currentPathPtr, &nextSegment, initParams); 
    
        if (NULL == GetElementDataPtr(index))
        {   //qtss_printf("ElementNode::SetupNodes call SetUpSingleElement index=%lu nextSegment=");PRINT_STR( &nextSegment);
            SetUpSingleElement(queryPtr,currentPathPtr, &nextSegment, index, initParams);
        }
        
        
    } while (false);

}

//The purpose of this function is to obtain the filtered attribute names. It accepts an ElementDataFields structure and QTSS_ AttributeID object as a parameter.
//It calls QTSS_ GetValueAsString() function to obtain the property name.
//If the length of the name is less than eMaxAttributeNameSize, it is copied into the structure.
void ElementNode::GetFilteredAttributeName(ElementDataFields* fieldPtr, QTSS_AttributeID theID)
{   
    fieldPtr->fFieldLen = 0;
    char *theName = NULL;
    (void) QTSS_GetValueAsString (fieldPtr->fAPISource, theID,0, &theName);
    OSCharArrayDeleter nameDeleter(theName); 
    if (theName != NULL )
    {   UInt32 len = strlen(theName);
        if (len < eMaxAttributeNameSize)
        {   memcpy(fieldPtr->fFieldName, theName, len); 
            fieldPtr->fFieldName[len] = 0;
            fieldPtr->fFieldLen = len;
        }
    }
}

//Obtain the filtered attribute ID. It accepts two strings and one QTSS_ AttributeID pointer as parameter
//It checks whether the parent node name is' server 'and whether the child node name is' qtssSvrClientSessions' or' qtssSvrModuleObjects'.
//If the attribute ID is found, store it at the location pointed to by the foundID pointer and set the foundID to true.
Bool16 ElementNode::GetFilteredAttributeID(char *parentName, char *nodeName, QTSS_AttributeID* foundID)
{
    Bool16 found = false;
    
    if (0 == strcmp("server", parentName))
    {
        if (0 == strcmp("qtssSvrClientSessions", nodeName) )
        {   if (foundID)
                *foundID = qtssCliSesCounterID;
            found = true;
        }
        
        if (0 == strcmp("qtssSvrModuleObjects", nodeName))
        {   if (foundID) 
                *foundID = qtssModName;
            found = true;
        }
    }           
    return found;
};

//Determine whether the node is a preference container.
//Check if the node name is' qtssSvrPreferences' or 'qtssModPrefs'.
//If it is a preference setting container, store its corresponding attribute ID at the location pointed to by the foundID pointer and set find to true.
Bool16 ElementNode::IsPreferenceContainer(char *nodeName, QTSS_AttributeID* foundID)
{
     Bool16 found = false;
    if (foundID) *foundID = 0;
    //qtss_printf(" ElementNode::IsPreferenceContainer name = %s \n",nodeName);
    if (0 == strcmp("qtssSvrPreferences", nodeName) )
    {   if (foundID) *foundID = qtssCliSesCounterID;
        found = true;
    }
    
    if (0 == strcmp("qtssModPrefs", nodeName))
    {   if (foundID) *foundID = qtssModName;
        found = true;
    }
        
    return found;
};

ElementNode::ElementDataFields AdminClass::sAdminSelf[] = // special case of top of tree
{   // key, API_ID,     fIndex,     Name,       Name_Len,       fAccessPermissions, Access, access_Len, fAPI_Type, fFieldType ,fAPISource
    {0,     0,          0,          "admin",    strlen("admin"),    qtssAttrModeRead, "r", strlen("r"),0,   ElementNode::eNode, NULL    }
};

ElementNode::ElementDataFields AdminClass::sAdminFieldIDs[] = 
{   // key, API_ID,     fIndex,     Name,       Name_Len,       fAccessPermissions, Access, access_Len, fAPI_Type, fFieldType ,fAPISource
    {0,     0,          0,          "server",   strlen("server"),qtssAttrModeRead,  "r", strlen("r"),qtssAttrDataTypeQTSS_Object,   ElementNode::eNode, NULL    }
};


//Initialize AdminClass object
//Calling ElementNode_ The InsertPtr() function is used to insert some pointers.
//Obtain the root path and call the SetupNodes() function.
//Call the SetNumFields() function to set the number of fields.

void AdminClass::Initialize(QTSS_Initialize_Params *initParams, QueryURI *queryPtr) 
{   

    //qtss_printf("----- Initialize AdminClass ------\n");
    
    SetParentNode(NULL);
    SetNodeInfo(&sAdminSelf[0]);// special case of this node as top of tree so it sets self
    Assert(NULL != GetMyName());
    SetNodeName(GetMyName());
    SetSource(NULL);
    StrPtrLen *currentPathPtr = queryPtr->GetRootID();
    UInt32 numFields = 1;
    SetNumFields(numFields);
    fFieldIDs = sAdminFieldIDs;
    fDataFieldsType = eStatic;
    fPathBuffer[0]=0;
    fPathSPL.Set(fPathBuffer,0);
    fIsTop = true;
    fInitialized = true;
    do
    {   
        Assert(fElementMap == NULL);
        fElementMap = NEW OSRefTable();  ElementNode_InsertPtr(fElementMap,"AdminClass::Initialize ElementNode* fElementMap ");
        Assert(fElementMap != NULL);
        if (fElementMap == NULL) break;
        
        Assert(fFieldDataPtrs == NULL);
        fFieldDataPtrs = NEW char*[numFields];  ElementNode_InsertPtr(fFieldDataPtrs,"AdminClass::Initialize ElementNode* fFieldDataPtrs array ");
        Assert(fFieldDataPtrs != NULL);
        if (fFieldDataPtrs == NULL) break;
        memset(fFieldDataPtrs, 0, numFields * sizeof(char*));
            
        Assert(fFieldOSRefPtrs == NULL);
        fFieldOSRefPtrs = NEW OSRef *[numFields]; ElementNode_InsertPtr(fFieldOSRefPtrs,"AdminClass::Initialize ElementNode* fFieldOSRefPtrs array ");
        Assert(fFieldOSRefPtrs != NULL);
        if (fFieldOSRefPtrs == NULL) break;
        memset(fFieldOSRefPtrs, 0, numFields * sizeof(OSRef*));
        
        QTSS_Error err = fElementMap->Register(GetOSRef(0));        
        Assert(err == QTSS_NoErr);
    } while (false);
    
    if (queryPtr && currentPathPtr) do
    {   
        StrPtrLen nextSegment;
        if (!queryPtr->NextSegment(currentPathPtr, &nextSegment)) break;

        SetupNodes(queryPtr,currentPathPtr,initParams);
    } while(false);


};

//Set up a single node.
//The function first sets the nodes based on the index.
//If the index is eServer, the Initialize() function is called to initialize the node.
void AdminClass::SetUpSingleNode(QueryURI *queryPtr,  StrPtrLen *currentSegmentPtr, StrPtrLen *nextSegmentPtr, SInt32 index, QTSS_Initialize_Params *initParams) 
{
    //qtss_printf("-------- AdminClass::SetUpSingleNode ---------- \n");
    switch (index)
    {
        case eServer:
            //qtss_printf("AdminClass::SetUpSingleNode case eServer\n");
            fNodePtr =  NEW ElementNode();  ElementNode_InsertPtr(fNodePtr, "AdminClass::SetUpSingleNode ElementNode * NEW ElementNode()");
            SetElementDataPtr(index,(char *) fNodePtr, true); 
            if (fNodePtr)
            {   fNodePtr->Initialize(index, this, queryPtr,nextSegmentPtr,initParams, initParams->inServer, eDynamic);
            }
        break;
    };
    
}

//Set individual elements
void AdminClass::SetUpSingleElement(QueryURI *queryPtr, StrPtrLen *currentSegmentPtr,StrPtrLen *nextSegmentPtr, SInt32 index, QTSS_Initialize_Params *initParams) 
{
    //qtss_printf("---------AdminClass::SetUpSingleElement------- \n");
    SetUpSingleNode(queryPtr,currentSegmentPtr, nextSegmentPtr, index, initParams);
}

//The destructor of the AdminClass object.
//Delete the memory pointed to by the fNodePtr pointer and remove the pointer from the pointer table
AdminClass::~AdminClass() 
{   //qtss_printf("AdminClass::~AdminClass() \n");
    delete (ElementNode*) fNodePtr;ElementNode_RemovePtr(fNodePtr,"AdminClass::~AdminClass ElementNode* fNodePtr");
    fNodePtr = NULL;
}

