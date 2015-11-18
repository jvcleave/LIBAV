//
//  Decoder.cpp
//  JVCRPI2_LOCAL
//
//  Created by jason van cleave on 11/17/15.
//  Copyright (c) 2015 jason van cleave. All rights reserved.
//

#include "Decoder.h"


Decoder::Decoder()
{
    clock = NULL;
    stream = NULL;
    doOnIdle = false;
}

//if this is crashing it you probably need callbacks set for components
OMX_ERRORTYPE Decoder::disableAllPortsForComponent(OMX_HANDLETYPE handle)
{
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    
    OMX_INDEXTYPE indexTypes[] = 
    {
        OMX_IndexParamAudioInit,
        OMX_IndexParamImageInit,
        OMX_IndexParamVideoInit, 
        OMX_IndexParamOtherInit
    };
    
    OMX_PORT_PARAM_TYPE ports;
    OMX_INIT_STRUCTURE(ports);
    for(int i=0; i < 4; i++)
    {
        error = OMX_GetParameter(handle, indexTypes[i], &ports);
        OMX_TRACE(error);
        if(error == OMX_ErrorNone) 
        {
            for(int j=0; j<ports.nPorts; j++)
            {
  
                OMX_PARAM_PORTDEFINITIONTYPE portFormat;
                OMX_INIT_STRUCTURE(portFormat);
                portFormat.nPortIndex = ports.nStartPortNumber+j;
                
                error = OMX_GetParameter(handle, OMX_IndexParamPortDefinition, &portFormat);
                OMX_TRACE(error);
                if(error != OMX_ErrorNone)
                {
                    if(portFormat.bEnabled == OMX_FALSE)
                    {
                        continue;
                    }
                }
                ofLogVerbose() << __LINE__;
                error = OMX_SendCommand(handle, OMX_CommandPortDisable, ports.nStartPortNumber+j, NULL);
                OMX_TRACE(error);
                if(error != OMX_ErrorNone)
                {
                    /*
                    ofLog(OF_LOG_VERBOSE, 
                          "disableAllPortsForComponent - Error disable port %d on component %s error: 0x%08x", 
                          (int)(ports.nStartPortNumber) + j,
                          "m_componentName", 
                          (int)error);
                     */
                }
            }
            
        }else
        {
            OMX_TRACE(error);
        }
    }
    
    return OMX_ErrorNone;
}


OMX_ERRORTYPE Decoder::onEncoderFillBufferDone(OMX_HANDLETYPE hComponent,
                                             OMX_PTR pAppData,
                                             OMX_BUFFERHEADERTYPE* pBuffer)
{	
    ofLogVerbose(__func__) << "";
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Decoder::onEncoderEmptyBufferDone(OMX_HANDLETYPE hComponent,
                                               OMX_PTR pAppData,
                                               OMX_BUFFERHEADERTYPE* pBuffer)
{	
    ofLogVerbose(__func__) << "";
    return OMX_ErrorNone;
}


OMX_ERRORTYPE Decoder::onEncoderEvent(OMX_HANDLETYPE hComponent,
                                                            OMX_PTR pAppData,
                                                            OMX_EVENTTYPE eEvent,
                                                            OMX_U32 nData1,
                                                            OMX_U32 nData2,
                                                            OMX_PTR pEventData)
{
    ofLog(
          OF_LOG_VERBOSE, 
          "%s - eEvent(0x%x), nData1(0x%lx), nData2(0x%lx), pEventData(0x%p)\n",
          __func__, 
          eEvent, 
          nData1, 
          nData2, 
          pEventData);
    ofLogVerbose(__func__) << OMX_Maps::getInstance().eventTypes[eEvent];
    Decoder *decoder = static_cast<Decoder*>(pAppData);

    switch (eEvent) 
    {
        case OMX_EventCmdComplete:
        {
            decoder->onIdle();
            return OMX_ErrorNone;
        }
        case OMX_EventParamOrConfigChanged:
        {
            
            //return decoder->onCameraEventParamOrConfigChanged();
            ofLogVerbose(__func__) << "OMX_EventParamOrConfigChanged";
            return OMX_ErrorNone;
        }	
            
        case OMX_EventError:
        {
            OMX_TRACE((OMX_ERRORTYPE)nData1);
            if((OMX_ERRORTYPE)nData1 == OMX_ErrorUnsupportedSetting)
            {
                decoder->onIdle();
                return OMX_ErrorNone;
            }
            
        }
        default: 
        {
           /* ofLog(OF_LOG_VERBOSE, 
             "Decoder::onEncoderEvent::%s - eEvent(0x%x), nData1(0x%lx), nData2(0x%lx), pEventData(0x%p)\n",
             __func__, eEvent, nData1, nData2, pEventData);*/
            
            break;
        }
    }
    return OMX_ErrorNone;
}

void Decoder::setup(Clock* clock_, Stream* stream_)
{
    clock = clock_;
    stream = stream_;
    
    OMX_CALLBACKTYPE  decoderCallbacks;
    decoderCallbacks.EventHandler    = &Decoder::onEncoderEvent;
    decoderCallbacks.EmptyBufferDone = &Decoder::onEncoderEmptyBufferDone;
    decoderCallbacks.FillBufferDone  = &Decoder::onEncoderFillBufferDone;
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    error = OMX_GetHandle(&decoder, OMX_VIDEO_DECODER, this, &decoderCallbacks);
    OMX_TRACE(error);
    
    error = disableAllPortsForComponent(decoder);
    OMX_TRACE(error);
    
    OMX_CALLBACKTYPE  rendererCallbacks;
    rendererCallbacks.EventHandler    = &Decoder::nullEventHandlerCallback;
    rendererCallbacks.EmptyBufferDone = &Decoder::nullEmptyBufferDone;
    rendererCallbacks.FillBufferDone  = &Decoder::nullFillBufferDone;
    
    error = OMX_GetHandle(&renderer, OMX_VIDEO_RENDER, this, &rendererCallbacks);
    OMX_TRACE(error);
    
    error = disableAllPortsForComponent(renderer);
    OMX_TRACE(error);
    
    
    OMX_CALLBACKTYPE  schedulerCallbacks;
    schedulerCallbacks.EventHandler    = &Decoder::nullEventHandlerCallback;
    schedulerCallbacks.EmptyBufferDone = &Decoder::nullEmptyBufferDone;
    schedulerCallbacks.FillBufferDone  = &Decoder::nullFillBufferDone;
    
    error = OMX_GetHandle(&scheduler, OMX_VIDEO_SCHEDULER, this, &schedulerCallbacks);
    OMX_TRACE(error);
    
    error = disableAllPortsForComponent(scheduler);
    OMX_TRACE(error);


    
    OMX_VIDEO_PARAM_PORTFORMATTYPE formatType;
    OMX_INIT_STRUCTURE(formatType);
    formatType.nPortIndex = VIDEO_DECODER_INPUT_PORT;
    formatType.eCompressionFormat = OMX_VIDEO_CodingUnused;
    
    StreamInfo& streamInfo = stream->streamInfo;
    if (streamInfo.fpsScale > 0 && streamInfo.fpsRate > 0)
    {
        formatType.xFramerate = (long long)(1<<16) * streamInfo.fpsRate / streamInfo.fpsScale;
    }
    else
    {
        formatType.xFramerate = 25 * (1<<16);
    }
    ofLogVerbose() << "formatType.xFramerate: " << formatType.xFramerate;
    error = OMX_SetParameter(decoder, OMX_IndexParamVideoPortFormat, &formatType);
    OMX_TRACE(error);
    
 
    
    OMX_PARAM_PORTDEFINITIONTYPE portParam;
    OMX_INIT_STRUCTURE(portParam);
    portParam.nPortIndex = VIDEO_DECODER_INPUT_PORT;
    
    error = OMX_GetParameter(decoder, OMX_IndexParamPortDefinition, &portParam);
    OMX_TRACE(error);
    
    portParam.nBufferCountActual = 20;
    portParam.format.video.nFrameWidth  = streamInfo.width;
    portParam.format.video.nFrameHeight = streamInfo.height;
    error = OMX_SetParameter(decoder, OMX_IndexParamPortDefinition, &portParam);
    OMX_TRACE(error);
    
    
    OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
    OMX_INIT_STRUCTURE(concanParam);
    concanParam.bStartWithValidFrame = OMX_FALSE;
    
    error = OMX_SetParameter(decoder, OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam);
    OMX_TRACE(error);
    

    if(streamInfo.doFormatting)
    {
        OMX_NALSTREAMFORMATTYPE nalStreamFormat;
        OMX_INIT_STRUCTURE(nalStreamFormat);
        nalStreamFormat.nPortIndex = VIDEO_DECODER_INPUT_PORT;
        nalStreamFormat.eNaluFormat = OMX_NaluFormatStartCodes;
        error = OMX_SetParameter(decoder, (OMX_INDEXTYPE)OMX_IndexParamNalStreamFormatSelect, &nalStreamFormat);
        OMX_TRACE(error);
    }
    
    OMX_CONFIG_BOOLEANTYPE timeStampMode;
    OMX_INIT_STRUCTURE(timeStampMode);
    timeStampMode.bEnabled = OMX_TRUE;
    error = OMX_SetParameter(decoder, OMX_IndexParamBrcmVideoTimestampFifo, &timeStampMode);
    OMX_TRACE(error);
    
 
    
    
    
    
    OMX_STATETYPE state;
    error = OMX_GetState(decoder, &state);
    OMX_TRACE(error);
    ofLogVerbose() << "state: " << OMX_Maps::getInstance().omxStateNames[state];

    error = OMX_SendCommand(decoder, OMX_CommandPortEnable, VIDEO_DECODER_INPUT_PORT, NULL);
    OMX_TRACE(error);
    
    

    
    
    doOnIdle = true;
    error = OMX_SendCommand(decoder, OMX_CommandStateSet, OMX_StateIdle, 0);
    OMX_TRACE(error);
    
   
    
 
    
    //error = OMX_SendCommand(decoder, OMX_CommandStateSet, OMX_StateExecuting, 0);
   // OMX_TRACE(error);

}

void Decoder::onIdle()
{
    if(!doOnIdle)
    {
        ofLogVerbose(__func__) << "RETURNING EARLY";
        return;
    }
    
    doOnIdle = false;
    
    ofLogVerbose(__func__) << "";
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
 
    
    OMX_STATETYPE state;
    error = OMX_GetState(decoder, &state);
    OMX_TRACE(error);
    ofLogVerbose() << "state: " << OMX_Maps::getInstance().omxStateNames[state];
    
 
    
    
    OMX_PARAM_PORTDEFINITIONTYPE portFormat;
    OMX_INIT_STRUCTURE(portFormat);
    portFormat.nPortIndex = VIDEO_DECODER_INPUT_PORT;
    
    error = OMX_GetParameter(decoder, OMX_IndexParamPortDefinition, &portFormat);
    OMX_TRACE(error);
    
    
    ofLogVerbose() << "portFormat.nBufferCountActual: " << portFormat.nBufferCountActual;
    ofLogVerbose() << "portFormat.nBufferSize: " << portFormat.nBufferSize;
    
    for (size_t i = 0; i < portFormat.nBufferCountActual; i++)
    {
        OMX_BUFFERHEADERTYPE *buffer = NULL;
        //OMX_U8* data = NULL;
        
        error = OMX_AllocateBuffer(decoder, &buffer, VIDEO_DECODER_INPUT_PORT, NULL, portFormat.nBufferSize);
        OMX_TRACE(error);
        
        buffer->nInputPortIndex = VIDEO_DECODER_INPUT_PORT;
        buffer->nFilledLen      = 0;
        buffer->nOffset         = 0;
        buffer->pAppPrivate     = (void*)i;
    }
    

#if 0   
    //decoder->scheduler
    error = OMX_SetupTunnel(decoder, VIDEO_DECODER_OUTPUT_PORT, scheduler, VIDEO_SCHEDULER_INPUT_PORT);
    OMX_TRACE(error);
    
    //scheduler->renderer
    error = OMX_SetupTunnel(scheduler, VIDEO_SCHEDULER_OUTPUT_PORT, renderer, VIDEO_RENDER_INPUT_PORT);
    OMX_TRACE(error);
    
    //clock->scheduler
    error = OMX_SetupTunnel(clock->handle, OMX_CLOCK_OUTPUT_PORT0, scheduler, VIDEO_SCHEDULER_CLOCK_PORT);
    OMX_TRACE(error);
    
#endif
}






