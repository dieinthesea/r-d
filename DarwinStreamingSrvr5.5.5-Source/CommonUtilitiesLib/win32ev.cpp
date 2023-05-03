#include "ev.h"
#include "OSHeaders.h"
#include "OSThread.h"
#include "MyAssert.h"

//create a window to get socket events
static HWND sMsgWindow = NULL;

LRESULT CALLBACK select_wndproc(HWND inWIndow, UINT inMsg, WPARAM inParam, LPARAM inOtherParam);

void select_startevents()
{
    //
    //occurs from the main thread, create the window from select_waitevent
}

int select_removeevent(int /*which*/)
{
    return 0;
}

int select_watchevent(struct eventreq *req, int which)
{
    return select_modwatch(req, which);
}

int select_modwatch(struct eventreq *req, int which)
{
    //only happen when select_modwatch is being called as the server is starting up.
    while (sMsgWindow == NULL)
        OSThread::Sleep(10);
    
    long theEvent = 0;
    
    if (which & EV_RE)
        theEvent |= FD_READ | FD_ACCEPT | FD_CLOSE;
    if (which & EV_WR)
        theEvent |= FD_WRITE | FD_CONNECT;

    unsigned int theMsg = (unsigned int)(req->er_data);
    
    return ::WSAAsyncSelect(req->er_handle, sMsgWindow, theMsg, theEvent);
}

int select_waitevent(struct eventreq *req, void* /*onlyForMacOSX*/)
{
    if (sMsgWindow == NULL)
    {
        //first call, initialze the window
        WNDCLASSEX theWndClass;
        theWndClass.cbSize = sizeof(theWndClass);
        theWndClass.style = 0;
        theWndClass.lpfnWndProc = &select_wndproc;
        theWndClass.cbClsExtra = 0;
        theWndClass.cbWndExtra = 0;
        theWndClass.hInstance = NULL;
        theWndClass.hIcon = NULL;
        theWndClass.hCursor = NULL;
        theWndClass.hbrBackground = NULL;
        theWndClass.lpszMenuName = NULL;
        theWndClass.lpszClassName = "DarwinStreamingServerWindow";
        theWndClass.hIconSm = NULL;
        
        ATOM theWndAtom = ::RegisterClassEx(&theWndClass);
        Assert(theWndAtom != NULL);
        if (theWndAtom == NULL)
            ::exit(-1); 
                
        sMsgWindow = ::CreateWindow(    "DarwinStreamingServerWindow",  // Window class name
                                        "DarwinStreamingServerWindow",  // Window title bar
                                        WS_POPUP,   // Window style ( a popup doesn't need a parent )
                                        0,          // x pos
                                        0,          // y pos
                                        CW_USEDEFAULT,  // default width
                                        CW_USEDEFAULT,  // default height
                                        NULL,           // No parent
                                        NULL,           // No menu handle
                                        NULL,           // Ignored on WinNT
                                        NULL);          // data for message proc. Who cares?
        Assert(sMsgWindow != NULL);
        if (sMsgWindow == NULL)
            ::exit(-1);
    }
    
    MSG theMessage;
    
    //blocks until there is a message
    UInt32 theErr = ::GetMessage(&theMessage, sMsgWindow, 0, 0);
    
    if (theErr > 0)
    {
        UInt32 theSelectErr = WSAGETSELECTERROR(theMessage.lParam);
        UInt32 theEvent = WSAGETSELECTEVENT(theMessage.lParam);
        
        req->er_handle = theMessage.wParam; 
        req->er_eventbits = EV_RE;          

        req->er_data = (void*)(theMessage.message);
        
        (void)::WSAAsyncSelect(req->er_handle, sMsgWindow, 0, 0); 
        
        return 0;
    }
    else
    {
        Assert(0);
        return EINTR;
    }
}


LRESULT CALLBACK select_wndproc(HWND /*inWIndow*/, UINT inMsg, WPARAM /*inParam*/, LPARAM /*inOtherParam*/)
{
    // If we don't return true for this message, window creation will not proceed
    if (inMsg == WM_NCCREATE)
        return TRUE;
    
    return 0;
}
