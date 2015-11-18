#include "Clock.h"

Clock::Clock()
{
    OMX_Init();
    
    OMX_CALLBACKTYPE  callbacks;
    //callbacks.EventHandler    = NULL;
    //callbacks.EmptyBufferDone = NULL;
    //callbacks.FillBufferDone  = NULL;
    
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    error = OMX_GetHandle(&handle, OMX_CLOCK, this, &callbacks);
    OMX_TRACE(error);
}

void Clock::setup(bool useAudioAsReference)
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