#pragma once

#include "Clock.h"
#include "VideoFile.h"

class Decoder
{
public:
    Decoder();
    void setup(Clock*, Stream* );
    bool doOnIdle;
    void onIdle();
    Clock* clock;
    Stream* stream;
    OMX_HANDLETYPE decoder;
    OMX_HANDLETYPE renderer;
    OMX_HANDLETYPE scheduler;
    OMX_HANDLETYPE imageEffects;
    OMX_ERRORTYPE disableAllPortsForComponent(OMX_HANDLETYPE);
    OMX_ERRORTYPE onDecoderEventHandler(OMX_HANDLETYPE,
                                        OMX_PTR,
                                        OMX_EVENTTYPE,
                                        OMX_U32,
                                        OMX_U32,
                                        OMX_PTR);
    
    static OMX_ERRORTYPE 
    onEncoderEmptyBufferDone(OMX_HANDLETYPE, 
                             OMX_PTR, 
                             OMX_BUFFERHEADERTYPE*);

    
    static OMX_ERRORTYPE 
    onEncoderFillBufferDone(OMX_HANDLETYPE, 
                            OMX_PTR, 
                            OMX_BUFFERHEADERTYPE*);

    
    static OMX_ERRORTYPE 
    onEncoderEvent(OMX_HANDLETYPE hComponent, 
                   OMX_PTR, 
                   OMX_EVENTTYPE, 
                   OMX_U32, 
                   OMX_U32, 
                   OMX_PTR);
    
    static OMX_ERRORTYPE 
    nullEmptyBufferDone(OMX_HANDLETYPE hComponent, 
                        OMX_PTR pAppData, 
                        OMX_BUFFERHEADERTYPE* pBuffer)
    {return OMX_ErrorNone;};
    
    static OMX_ERRORTYPE 
    nullFillBufferDone(OMX_HANDLETYPE hComponent, 
                       OMX_PTR pAppData, 
                       OMX_BUFFERHEADERTYPE* pBuffer)
    {return OMX_ErrorNone;};
    
    static OMX_ERRORTYPE 
    nullEventHandlerCallback(OMX_HANDLETYPE hComponent, 
                             OMX_PTR pAppData, 
                             OMX_EVENTTYPE eEvent, 
                             OMX_U32 nData1, 
                             OMX_U32 nData2, 
                             OMX_PTR pEventData)
    {return OMX_ErrorNone;};

};
