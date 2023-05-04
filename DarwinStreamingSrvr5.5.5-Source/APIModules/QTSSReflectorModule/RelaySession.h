/*
Inherited from the ReflectorSession class. 
It has some static methods, such as Register() and Initialize(),
which are used to call in the registration and initialization role of the relay module.
It also has some properties and functions, 
such as SetupReplaySession() for setting up relay sessions, 
and GetReplaySessionObject () for obtaining relay session objects.
The instantiation of this class requires passing in a pointer to a StrPtrLen type object 
and an optional pointer to a SourceInfo type object
*/
/*
    File:       RelaySession.h

    Contains:   Subclass of ReflectorSession. It has two static
                attributes (QTSSRelayModule Attributes object
                and the ReflectorSession attribute ID)  

*/

#include "QTSS.h"
#include "ReflectorSession.h"
#include "StrPtrLen.h"
#include "SourceInfo.h"
#include "RTSPSourceInfo.h"

#ifndef _RELAY_SESSION_
#define _RELAY_SESSION_

class RelaySession : public ReflectorSession
{
    public:
        
        // Call Register in the Relay Module's Register Role
        static void Register();
        
        //
        // Initialize
        // Call Initialize in the Relay Module's Initialize Role
        static void Initialize(QTSS_Object inAttrObject);
        
        RelaySession(StrPtrLen* inSourceID, SourceInfo* inInfo = NULL):ReflectorSession(inSourceID, inInfo){};
        ~RelaySession();
         
        QTSS_Error SetupRelaySession(SourceInfo* inInfo);
        
        QTSS_Object GetRelaySessionObject() { return fRelaySessionObject; }
        static QTSS_AttributeID     sRelayOutputObject;

        static char         sRelayUserAgent[20];
                
    private:
        
        QTSS_Object                 fRelaySessionObject;
        
        // gets set in the initialize method
        static QTSS_Object          relayModuleAttributesObject;
        
        static QTSS_ObjectType      qtssRelaySessionObjectType;
        
        static QTSS_AttributeID     sRelaySessionObjectID;
        static QTSS_AttributeID     sRelayName;
        static QTSS_AttributeID     sSourceType;
        static QTSS_AttributeID     sSourceIPAddr;
        static QTSS_AttributeID     sSourceInIPAddr;
        static QTSS_AttributeID     sSourceUDPPorts;
        static QTSS_AttributeID     sSourceRTSPPort;
        static QTSS_AttributeID     sSourceURL;
        static QTSS_AttributeID     sSourceUsername;
        static QTSS_AttributeID     sSourcePassword;
        static QTSS_AttributeID     sSourceTTL;
                
                
};

#endif
