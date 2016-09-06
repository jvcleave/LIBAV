#pragma once

#include "Component.h"



class ClockComponent: public Component
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

    
   
    ClockComponent()
    {
        name = OMX_CLOCK;
        
    }

    
    void setReference(bool useAudioAsReference)
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
        
        int speed = 1;
        
        OMX_TIME_CONFIG_SCALETYPE scaleType;
        OMX_INIT_STRUCTURE(scaleType);
        scaleType.xScale = (speed << 16) / DVD_PLAYSPEED_NORMAL; 
        
        error = OMX_SetConfig(handle, OMX_IndexConfigTimeScale, &scaleType);
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