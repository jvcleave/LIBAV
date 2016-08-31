#pragma once

#include "OMX_Maps.h"



class Clock
{
public:
    
    
    static OMX_ERRORTYPE 
    onClockEmptyBufferDone(OMX_HANDLETYPE hComponent, 
                        OMX_PTR pAppData, 
                        OMX_BUFFERHEADERTYPE* pBuffer)
    {
        return OMX_ErrorNone;
    };
    
    static OMX_ERRORTYPE 
    onClockFillBufferDone(OMX_HANDLETYPE hComponent, 
                       OMX_PTR pAppData, 
                       OMX_BUFFERHEADERTYPE* pBuffer)
    {
        return OMX_ErrorNone;
    };
    
    static OMX_ERRORTYPE 
   onClockEvent(OMX_HANDLETYPE hComponent, 
                             OMX_PTR pAppData, 
                             OMX_EVENTTYPE eEvent, 
                             OMX_U32 nData1, 
                             OMX_U32 nData2, 
                             OMX_PTR pEventData)
    {
        return OMX_ErrorNone;
    };
    
    
    OMX_HANDLETYPE handle;
    Clock()
    {
        OMX_Init();
        
        OMX_CALLBACKTYPE  callbacks;
        callbacks.EventHandler    = &Clock::onClockEvent;
        callbacks.EmptyBufferDone = &Clock::onClockEmptyBufferDone;
        callbacks.FillBufferDone  = &Clock::onClockFillBufferDone;
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        
        error = OMX_GetHandle(&handle, OMX_CLOCK, this, &callbacks);
        OMX_TRACE(error);
        ofLogVerbose() << "CLOCK DISABLE START";
        disableAllPortsForComponent(handle);
        ofLogVerbose() << "CLOCK DISABLE END";
        
    }
    
    void setup(bool useAudioAsReference)
    {
        OMX_ERRORTYPE error = OMX_ErrorNone;
        
        OMX_PORT_PARAM_TYPE portParam;
        OMX_INIT_STRUCTURE(portParam);
        error = OMX_GetParameter(handle, OMX_IndexParamOtherInit, &portParam);
        OMX_TRACE(error);
        
        OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE refClockConfig;
        OMX_INIT_STRUCTURE(refClockConfig);
        if(useAudioAsReference)
        {
            refClockConfig.eClock = OMX_TIME_RefClockAudio;
        }
        else
        {
            refClockConfig.eClock = OMX_TIME_RefClockVideo;
        }
        
        error = OMX_SetConfig(handle, OMX_IndexConfigTimeActiveRefClock, &refClockConfig);
        OMX_TRACE(error);
    }
    
    
};