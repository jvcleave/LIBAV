#pragma once

#include "Clock.h"
#include "VideoFile.h"

class Decoder
{
public:
    Clock* clock;
    VideoFile* videoFile;
    Stream* stream;
    OMX_HANDLETYPE decoder;
    OMX_HANDLETYPE renderer;
    OMX_HANDLETYPE scheduler;
    OMX_HANDLETYPE imageEffects;

    /*
    static OMX_ERRORTYPE onDecoderEmptyBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    static OMX_ERRORTYPE onDecoderFillBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    static OMX_ERRORTYPE onDecoderEvent(OMX_HANDLETYPE, OMX_PTR, OMX_EVENTTYPE, OMX_U32, OMX_U32, OMX_PTR);
    
    static OMX_ERRORTYPE onSchedulerEmptyBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    static OMX_ERRORTYPE onSchedulerFillBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    static OMX_ERRORTYPE onSchedulerEvent(OMX_HANDLETYPE, OMX_PTR, OMX_EVENTTYPE, OMX_U32, OMX_U32, OMX_PTR);
    
    static OMX_ERRORTYPE onRenderEmptyBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    static OMX_ERRORTYPE onRenderFillBufferDone(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    static OMX_ERRORTYPE onRenderEvent(OMX_HANDLETYPE, OMX_PTR, OMX_EVENTTYPE, OMX_U32, OMX_U32, OMX_PTR);
    
    static OMX_ERRORTYPE nullEmptyBufferDone(OMX_HANDLETYPE, OMX_PTR,OMX_BUFFERHEADERTYPE*) {return OMX_ErrorNone;};
    static OMX_ERRORTYPE nullFillBufferDone(OMX_HANDLETYPE, OMX_PTR,OMX_BUFFERHEADERTYPE*){return OMX_ErrorNone;};
    static OMX_ERRORTYPE nullEventHandlerCallback(OMX_HANDLETYPE, OMX_PTR, OMX_EVENTTYPE, OMX_U32, OMX_U32, OMX_PTR) {return OMX_ErrorNone;};
    */


    
    Decoder()
    {
        clock = NULL;
        videoFile = NULL;
        stream = NULL;
    }
    
    
        
    void setup(Clock* clock_, VideoFile* videoFile_)
    {
        clock = clock_;
        videoFile = videoFile_;
        stream = videoFile->getBestVideoStream();
        
        OMX_CALLBACKTYPE  decoderCallbacks;
        decoderCallbacks.EventHandler    = &Decoder::onDecoderEvent;
        decoderCallbacks.EmptyBufferDone = &Decoder::onDecoderEmptyBufferDone;
        decoderCallbacks.FillBufferDone  = &Decoder::onDecoderFillBufferDone;
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        error = OMX_GetHandle(&decoder, OMX_VIDEO_DECODER, this, &decoderCallbacks);
        OMX_TRACE(error);
        
        ofLogVerbose() << "decoder DISABLE START";
        error = disableAllPortsForComponent(decoder);
        OMX_TRACE(error);
        ofLogVerbose() << "decoder DISABLE END";
        
        
        OMX_CALLBACKTYPE  rendererCallbacks;
        rendererCallbacks.EventHandler    = &Decoder::onRenderEvent;
        rendererCallbacks.EmptyBufferDone = &Decoder::onRenderEmptyBufferDone;
        rendererCallbacks.FillBufferDone  = &Decoder::onRenderFillBufferDone;
        
        error = OMX_GetHandle(&renderer, OMX_VIDEO_RENDER, this, &rendererCallbacks);
        OMX_TRACE(error);
        
        ofLogVerbose() << "renderer DISABLE START";
        error = disableAllPortsForComponent(renderer);
        OMX_TRACE(error);
        ofLogVerbose() << "renderer DISABLE END";
        
        
        OMX_CALLBACKTYPE  schedulerCallbacks;
        schedulerCallbacks.EventHandler    = &Decoder::onSchedulerEvent;
        schedulerCallbacks.EmptyBufferDone = &Decoder::onSchedulerEmptyBufferDone;
        schedulerCallbacks.FillBufferDone  = &Decoder::onSchedulerFillBufferDone;
        
        error = OMX_GetHandle(&scheduler, OMX_VIDEO_SCHEDULER, this, &schedulerCallbacks);
        OMX_TRACE(error);
        
        ofLogVerbose() << "scheduler DISABLE START";
        error = disableAllPortsForComponent(scheduler);
        OMX_TRACE(error);
        ofLogVerbose() << "scheduler DISABLE END";
        
        
        //clock->scheduler
        error = OMX_SetupTunnel(clock->handle, OMX_CLOCK_OUTPUT_PORT0, scheduler, VIDEO_SCHEDULER_CLOCK_PORT);
        OMX_TRACE(error, "clock->scheduler TUNNEL");
        
        
        
        OMX_VIDEO_PARAM_PORTFORMATTYPE formatType;
        OMX_INIT_STRUCTURE(formatType);
        formatType.nPortIndex = VIDEO_DECODER_INPUT_PORT;
        formatType.eCompressionFormat = OMX_VIDEO_CodingAVC;
        
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
        
        ofLogVerbose() << "streamInfo.doFormatting: " << streamInfo.doFormatting;
        
        //
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
        
        error = OMX_SendCommand(decoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
        OMX_TRACE(error);

        OMX_STATETYPE state;
        error = OMX_GetState(decoder, &state);
        OMX_TRACE(error);
        ofLogVerbose() << "decoder state: " << OMX_Maps::getInstance().omxStateNames[state];
            
        int counter = 0;
        if(state != OMX_StateIdle)
        {
            while(state != OMX_StateIdle)
            {
                error = OMX_SendCommand(decoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
                OMX_TRACE(error);
                counter++;
                error = OMX_GetState(decoder, &state);
                //OMX_TRACE(error);
                ofLogVerbose() << "decoder state: " << OMX_Maps::getInstance().omxStateNames[state] << " : " << counter;
                
            }
            
        }
        error = OMX_SendCommand(decoder, OMX_CommandPortEnable, VIDEO_DECODER_INPUT_PORT, NULL);
        OMX_TRACE(error);
        
        OMX_PARAM_PORTDEFINITIONTYPE portFormat;
        OMX_INIT_STRUCTURE(portFormat);
        portFormat.nPortIndex = VIDEO_DECODER_INPUT_PORT;
        
        error = OMX_GetParameter(decoder, OMX_IndexParamPortDefinition, &portFormat);
        OMX_TRACE(error);
        
        ofLogVerbose() << "decoder portFormat.bEnabled: " << portFormat.bEnabled;
        ofLogVerbose() << "decoder portFormat.nBufferCountActual: " << portFormat.nBufferCountActual;
        ofLogVerbose() << "decoder portFormat.nBufferSize: " << portFormat.nBufferSize;
        
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
        
        
        
        //decoder->scheduler
        error = OMX_SetupTunnel(decoder, VIDEO_DECODER_OUTPUT_PORT, scheduler, VIDEO_SCHEDULER_INPUT_PORT);
        OMX_TRACE(error);
        
        //scheduler->renderer
        error = OMX_SetupTunnel(scheduler, VIDEO_SCHEDULER_OUTPUT_PORT, renderer, VIDEO_RENDER_INPUT_PORT);
        OMX_TRACE(error);
        

        //renderer to Idle
        error = OMX_SendCommand(renderer, OMX_CommandStateSet, OMX_StateIdle, NULL);
        OMX_TRACE(error);
        error = OMX_GetState(renderer, &state);
        OMX_TRACE(error);

        //enable render input
        error = OMX_SendCommand(renderer, OMX_CommandPortEnable, VIDEO_RENDER_INPUT_PORT, NULL);
        OMX_TRACE(error);
        
    
        //allocate render input buffers (unused with direct)
        portFormat.nPortIndex = VIDEO_RENDER_INPUT_PORT;
        error = OMX_GetParameter(renderer, OMX_IndexParamPortDefinition, &portFormat);
        OMX_TRACE(error);
        
        ofLogVerbose() << "renderer portFormat.bEnabled: " << portFormat.bEnabled;
        ofLogVerbose() << "renderer portFormat.nBufferCountActual: " << portFormat.nBufferCountActual;
        ofLogVerbose() << "renderer portFormat.nBufferSize: " << portFormat.nBufferSize;
        for (size_t i = 0; i < portFormat.nBufferCountActual; i++)
        {
            OMX_BUFFERHEADERTYPE *buffer = NULL;
            //OMX_U8* data = NULL;
            
            error = OMX_AllocateBuffer(renderer, &buffer, VIDEO_RENDER_INPUT_PORT, NULL, portFormat.nBufferSize);
            OMX_TRACE(error);
            
            buffer->nInputPortIndex = VIDEO_RENDER_INPUT_PORT;
            buffer->nFilledLen      = 0;
            buffer->nOffset         = 0;
            buffer->pAppPrivate     = (void*)i;
        }
        
        
        //start decoder
        error = OMX_SendCommand(decoder, OMX_CommandStateSet, OMX_StateExecuting, 0);
        OMX_TRACE(error);
        
        
        //start scheduler
        error = OMX_SendCommand(scheduler, OMX_CommandStateSet, OMX_StateExecuting, 0);
        OMX_TRACE(error);
        
        //start renderer
        error = OMX_SendCommand(renderer, OMX_CommandStateSet, OMX_StateExecuting, 0);
        OMX_TRACE(error);
        
        clock->start(0.0);
        
        while(videoFile->read())
        {
            counter++;
        }
        
        for (size_t i=0; i<videoFile->omxPackets.size(); i++) 
        {
            //ofLog() << i << endl << videoFile->omxPackets[i]->toString();
        }
    }
    
    
    
    static OMX_ERRORTYPE onRenderFillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
    {	
        ofLogVerbose(__func__) << "";
        return OMX_ErrorNone;
    }
    
    static OMX_ERRORTYPE onRenderEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
    {	
        ofLogVerbose(__func__) << "";
        return OMX_ErrorNone;
    }
    
    static OMX_ERRORTYPE onRenderEvent(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
    {
        ofLog(
              OF_LOG_VERBOSE, 
              "%s - eEvent(0x%x), nData1(0x%ux), nData2(0x%ux), pEventData(0x%p)\n",
              __func__, 
              eEvent, 
              nData1, 
              nData2, 
              pEventData);
        ofLogVerbose(__func__) << "EVENT TYPE: " << OMX_Maps::getInstance().eventTypes[eEvent];
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        switch (eEvent) 
        {
            case OMX_EventCmdComplete:
            {
                break;
            }
            case OMX_EventParamOrConfigChanged:
            {
                
                ofLogVerbose(__func__) << "OMX_EventParamOrConfigChanged";
                break;
            }	
                
            case OMX_EventError:
            {
                error = (OMX_ERRORTYPE)nData1;
                break;
                
            }
            default: 
            {
                
                break;
            }
        }
        OMX_TRACE(error);
        return error;
    }
    
    static OMX_ERRORTYPE onSchedulerFillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
    {	
        ofLogVerbose(__func__) << "";
        return OMX_ErrorNone;
    }
    
    static OMX_ERRORTYPE onSchedulerEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
    {	
        ofLogVerbose(__func__) << "";
        return OMX_ErrorNone;
    }
    
    static OMX_ERRORTYPE onSchedulerEvent(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
    {
        ofLog(
              OF_LOG_VERBOSE, 
              "%s - eEvent(0x%x), nData1(0x%ux), nData2(0x%ux), pEventData(0x%p)\n",
              __func__, 
              eEvent, 
              nData1, 
              nData2, 
              pEventData);
        ofLogVerbose(__func__) << "EVENT TYPE: " << OMX_Maps::getInstance().eventTypes[eEvent];
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        switch (eEvent) 
        {
            case OMX_EventCmdComplete:
            {
                //Decoder *decoder = static_cast<Decoder*>(pAppData);
                //decoder->onDecoderIdle();
                break;
            }
            case OMX_EventParamOrConfigChanged:
            {
                
                //return decoder->onCameraEventParamOrConfigChanged();
                ofLogVerbose(__func__) << "OMX_EventParamOrConfigChanged";
                break;
            }	
                
            case OMX_EventError:
            {
                error = (OMX_ERRORTYPE)nData1;
                break;
                
            }
            default: 
            {
                /* ofLog(OF_LOG_VERBOSE, 
                 "onEncoderEvent::%s - eEvent(0x%x), nData1(0x%ux), nData2(0x%ux), pEventData(0x%p)\n",
                 __func__, eEvent, nData1, nData2, pEventData);*/
                
                break;
            }
        }
        OMX_TRACE(error);
        return error;
    }
    
    static OMX_ERRORTYPE onDecoderFillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
    {	
        ofLogVerbose(__func__) << "";
        return OMX_ErrorNone;
    }
    
    static OMX_ERRORTYPE onDecoderEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
    {	
        ofLogVerbose(__func__) << "";
        return OMX_ErrorNone;
    }
    
    
    static OMX_ERRORTYPE onDecoderEvent(OMX_HANDLETYPE hComponent,
                                        OMX_PTR pAppData,
                                        OMX_EVENTTYPE eEvent,
                                        OMX_U32 nData1,
                                        OMX_U32 nData2,
                                        OMX_PTR pEventData)
    {
        ofLog(
              OF_LOG_VERBOSE, 
              "%s - eEvent(0x%x), nData1(0x%ux), nData2(0x%ux), pEventData(0x%p)\n",
              __func__, 
              eEvent, 
              nData1, 
              nData2, 
              pEventData);
        ofLogVerbose(__func__) << "EVENT TYPE: " << OMX_Maps::getInstance().eventTypes[eEvent];
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        
        OMX_STATETYPE state;
        error = OMX_GetState(hComponent, &state);
        OMX_TRACE(error);
        ofLogVerbose(__func__) << "state: " << OMX_Maps::getInstance().omxStateNames[state];
        
        switch (eEvent) 
        {
            case OMX_EventCmdComplete:
            {
                break;
            }
            case OMX_EventParamOrConfigChanged:
            {
                
                //return decoder->onCameraEventParamOrConfigChanged();
                ofLogVerbose(__func__) << "OMX_EventParamOrConfigChanged";
                break;
            }	
                
            case OMX_EventError:
            {
                error = (OMX_ERRORTYPE)nData1;
                break;
                
            }
            default: 
            {
                /* ofLog(OF_LOG_VERBOSE, 
                 "onEncoderEvent::%s - eEvent(0x%x), nData1(0x%ux), nData2(0x%ux), pEventData(0x%p)\n",
                 __func__, eEvent, nData1, nData2, pEventData);*/
                
                break;
            }
        }
        OMX_TRACE(error);
        return error;
    }
    



};
