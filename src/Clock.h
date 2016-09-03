#pragma once

#include "OMX_Maps.h"



class Clock
{
public:
    
    
    
    
    
    OMX_TICKS ToOMXTime(int64_t pts)
    {
        OMX_TICKS ticks;
        ticks.nLowPart = pts;
        ticks.nHighPart = pts >> 32;
        return ticks;
    }
    
    int64_t FromOMXTime(OMX_TICKS ticks)
    {
        int64_t pts = ticks.nLowPart | ((uint64_t)(ticks.nHighPart) << 32);
        return pts;
    }
    
    
    
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
    void start(double pts=0.0)
    {
        OMX_TIME_CONFIG_CLOCKSTATETYPE clockConfig;
        OMX_INIT_STRUCTURE(clockConfig);
        
        clockConfig.eState = OMX_TIME_ClockStateRunning;
        clockConfig.nStartTime = ToOMXTime((uint64_t)pts);
        OMX_ERRORTYPE error = OMX_SetConfig(handle, OMX_IndexConfigTimeClockState, &clockConfig);
        OMX_TRACE(error);
    }
    
    double getMediaTime()
    {
        
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        double pts = 0;
        
        OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
        OMX_INIT_STRUCTURE(timeStamp);
        timeStamp.nPortIndex = OMX_CLOCK_OUTPUT_PORT0;
        error = OMX_GetConfig(handle, OMX_IndexConfigTimeCurrentMediaTime, &timeStamp);
        
        if(error != OMX_ErrorNone)
        {
            ofLogError(__func__) << "FAIL";
            ofSleepMillis(5000);
            return 0;
        }
        
        pts = (double)FromOMXTime(timeStamp.nTimestamp);
        
        return pts;
    }

    
    
};