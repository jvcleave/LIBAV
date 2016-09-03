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
    
    vector<OMX_BUFFERHEADERTYPE*> decoderInputBuffers;
    queue<OMX_BUFFERHEADERTYPE*> decoderInputBuffersAvailable;
    int packetCounter = 0;
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
        packetCounter = 0;
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
        
        if (stream->fpsScale > 0 && stream->fpsRate > 0)
        {
            formatType.xFramerate = (long long)(1<<16) * stream->fpsRate / stream->fpsScale;
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
        portParam.format.video.nFrameWidth  = stream->width;
        portParam.format.video.nFrameHeight = stream->height;
        error = OMX_SetParameter(decoder, OMX_IndexParamPortDefinition, &portParam);
        OMX_TRACE(error);
        
       
        OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
        OMX_INIT_STRUCTURE(concanParam);
        concanParam.bStartWithValidFrame = OMX_FALSE;
        
        error = OMX_SetParameter(decoder, OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam);
        OMX_TRACE(error);
        
        ofLogVerbose() << "stream->doFormatting: " << stream->doFormatting;
        
        //
        if(stream->doFormatting)
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
            decoderInputBuffers.push_back(buffer);
            decoderInputBuffersAvailable.push(buffer);
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
        
        OMX_CONFIG_DISPLAYREGIONTYPE displayConfig;
        OMX_INIT_STRUCTURE(displayConfig);
        displayConfig.nPortIndex = VIDEO_RENDER_INPUT_PORT;
        displayConfig.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_MODE);
        displayConfig.fullscreen  = OMX_TRUE; 
        displayConfig.mode  = OMX_DISPLAY_MODE_FILL; 
        error = OMX_SetConfig(renderer, OMX_IndexConfigDisplayRegion, &displayConfig);
        OMX_TRACE(error);
       
        ofSleepMillis(2000);
        
        while(videoFile->read())
        {
            counter++;
        }
        
        
        if(videoFile->getBestVideoStream()->codecExtraSize > 0 && videoFile->getBestVideoStream()->codecExtraData != NULL)
        {
            int extraSize = videoFile->getBestVideoStream()->codecExtraSize;
            uint8_t* extraData = (uint8_t *)malloc(extraSize);
            memcpy(extraData, videoFile->getBestVideoStream()->codecExtraData, extraSize);
            
            OMX_BUFFERHEADERTYPE* omxBuffer = NULL;
            if(!decoderInputBuffersAvailable.empty())
            {
                omxBuffer = decoderInputBuffersAvailable.front();
                decoderInputBuffersAvailable.pop();
                omxBuffer->nOffset = 0;
                omxBuffer->nFilledLen = extraSize;
                memset((unsigned char *)omxBuffer->pBuffer, 0x0, omxBuffer->nAllocLen);
                memcpy((unsigned char *)omxBuffer->pBuffer, extraData, omxBuffer->nFilledLen);
                omxBuffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;
                error = OMX_EmptyThisBuffer(decoder, omxBuffer);
                OMX_TRACE(error);
            }
            
        }  
        
    }
    bool doSetStartTime = true;
    
    
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
    
    void decodeNext()
    {
        ofLogVerbose() << "decodeNext: " << packetCounter;
        packetCounter++;
        decode(videoFile->omxPackets[packetCounter]);
    }
    bool decode(OMXPacket* pkt)
    {
        
        if(!pkt)
        {
            return false;
        }
        double currentPTS=0;
        bool result = false;
        double pts = pkt->pts;
        double dts = pkt->dts;
        
        if(pkt->pts == DVD_NOPTS_VALUE)
        {
            pts = pkt->dts;
        }
        
        if(pts != DVD_NOPTS_VALUE)
        {
            currentPTS = pts;
        }
        
        ofLog(OF_LOG_VERBOSE, "decode dts:%.0f pts:%.0f cur:%.0f, size:%d", pkt->dts, pkt->pts, currentPTS, pkt->size);
        decode(pkt->data, pkt->size, dts, pts);
        
        return result;
    }
    bool decode(uint8_t* demuxer_content, int iSize, double dts, double pts)
    {
        OMX_ERRORTYPE error;
        
        unsigned int demuxer_bytes = (unsigned int)iSize;
        
        if (demuxer_content && demuxer_bytes > 0)
        {
            while(demuxer_bytes)
            {
                
                OMX_BUFFERHEADERTYPE* omxBuffer = NULL;
                if(!decoderInputBuffersAvailable.empty())
                {
                    omxBuffer = decoderInputBuffersAvailable.front();
                    decoderInputBuffersAvailable.pop();
                }
                ofLogVerbose() << "decoderInputBuffersAvailable: " << decoderInputBuffersAvailable.size();
                omxBuffer->nFlags = 0;
                omxBuffer->nOffset = 0;
                
                if(doSetStartTime)
                {
                    omxBuffer->nFlags |= OMX_BUFFERFLAG_STARTTIME;
                    ofLog(OF_LOG_VERBOSE, "VideoDecoderDirect::Decode VDec : setStartTime %f\n", (pts == DVD_NOPTS_VALUE ? 0.0 : pts) / DVD_TIME_BASE);
                    doSetStartTime = false;
                }
                if (pts == DVD_NOPTS_VALUE)
                {
                    omxBuffer->nFlags |= OMX_BUFFERFLAG_TIME_UNKNOWN;
                }
                
                omxBuffer->nTimeStamp = ToOMXTime((uint64_t)(pts == DVD_NOPTS_VALUE) ? 0 : pts);
                if(demuxer_bytes > omxBuffer->nAllocLen)
                {
                    ofLog() << "USING nAllocLen: "<< omxBuffer->nAllocLen;
                    omxBuffer->nFilledLen = omxBuffer->nAllocLen;
                }else
                {
                    ofLog() << "USING demuxer_bytes: " << demuxer_bytes;

                    omxBuffer->nFilledLen = demuxer_bytes;
                }
                ofLog() << "PRE COPY";
                
                ofLog() << "demuxer_bytes: " << demuxer_bytes;
                ofLog() << "demuxer_content: " << demuxer_content;
                ofLog() << "omxBuffer->nFilledLen: " << omxBuffer->nFilledLen;
                ofLog() << "omxBuffer->nTimeStamp: " << FromOMXTime(omxBuffer->nTimeStamp);

                memcpy(omxBuffer->pBuffer, demuxer_content, omxBuffer->nFilledLen);
                ofLog() << "POST COPY";

                demuxer_bytes -= omxBuffer->nFilledLen;
                demuxer_content += omxBuffer->nFilledLen;
                


                if(demuxer_bytes == 0)
                {
                    //ofLogVerbose(__func__) << "OMX_BUFFERFLAG_ENDOFFRAME";
                    omxBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
                }
                error = OMX_EmptyThisBuffer(decoder, omxBuffer);
                OMX_TRACE(error);
                
            }
            
            return true;
        }
        
        return false;
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
        Decoder* myDecoder = (Decoder*)pAppData;
        myDecoder->decoderInputBuffersAvailable.push(pBuffer);
        myDecoder->decodeNext();
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
