#pragma once

#include "Clock.h"
#include "VideoFile.h"

class CriticalSection
{
public:
    inline CriticalSection()
    {
        pthread_mutexattr_t mta;
        pthread_mutexattr_init(&mta);
        pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_lock, &mta);
    }
    inline ~CriticalSection()
    {
        pthread_mutex_destroy(&m_lock);
    }
    inline void lock()
    {
        pthread_mutex_lock(&m_lock);
    }
    inline void unlock()
    {
        pthread_mutex_unlock(&m_lock);
    }
    
protected:
    pthread_mutex_t m_lock;
};


class SingleLock
{
public:
    inline SingleLock(CriticalSection& cs)
    {
        section = cs;
        section.lock();
    }
    inline ~SingleLock()
    {
        section.unlock();
    }
    
protected:
    CriticalSection section;
};
//#define STALL(x) ofSleepMillis(x*1000);
#define STALL(x) 

#define LOGGER ofLogVerbose(__func__)


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
    
    int EOFCounter;
    
    vector<OMX_BUFFERHEADERTYPE*> decoderInputBuffers;
    queue<OMX_BUFFERHEADERTYPE*> decoderInputBuffersAvailable;
    int packetCounter = 0;
    bool doSetStartTime;
    pthread_cond_t  m_input_buffer_cond;
    pthread_mutex_t   m_omx_input_mutex;
    CriticalSection  m_critSection;
    double currentPTS;
    bool isDecoderReady;
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
    static void add_timespecs(struct timespec& time, long millisecs)
    {
        time.tv_sec  += millisecs / 1000;
        time.tv_nsec += (millisecs % 1000) * 1000000;
        if (time.tv_nsec > 1000000000)
        {
            time.tv_sec  += 1;
            time.tv_nsec -= 1000000000;
        }
    }

    
    Decoder()
    {
        clock = NULL;
        videoFile = NULL;
        stream = NULL;
        packetCounter = 0;
        currentPTS = 0;
        doSetStartTime = true;
        pthread_mutex_init(&m_omx_input_mutex, NULL);
        pthread_cond_init(&m_input_buffer_cond, NULL);
        EOFCounter = 0;
        isDecoderReady = false;

    }
    
    OMX_ERRORTYPE createTunnel(OMX_HANDLETYPE source, int sourcePort, 
                               OMX_HANDLETYPE destination, int destinationPort)
    {
        OMX_ERRORTYPE error = OMX_ErrorNone;
        OMX_STATETYPE state;
        OMX_GetState(source, &state);
        
        if(state != OMX_StateIdle)
        {
            error = OMX_SendCommand(source, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if(error != OMX_ErrorNone)
            {
                ofLogError() << "Could not set source to Idle";
                return error;
            }
        }


        error = OMX_SendCommand(source, OMX_CommandPortDisable, sourcePort, NULL);
        if(error != OMX_ErrorNone)
        {
            ofLogError() << "Could not disable source port " << sourcePort;
            return error;
        }
        
        OMX_GetState(destination, &state);
        
        if(state != OMX_StateIdle)
        {
            error = OMX_SendCommand(destination, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if(error != OMX_ErrorNone)
            {
                ofLogError() << "Could not set destination to Idle";
                return error;
            }
        }
        
        
        
        error = OMX_SendCommand(destination, OMX_CommandPortDisable, destinationPort, NULL);
        if(error != OMX_ErrorNone)
        {
            ofLogError() << "Could not disable destinationPort " << destinationPort;
            return error;
        }
        
        error = OMX_SetupTunnel(source, sourcePort, destination, destinationPort);
        if(error != OMX_ErrorNone)
        {
            ofLogError() << "Could not set up Tunnel from sourcePort, destinationPort" << sourcePort << " -> " << destinationPort;
            return error;
        }

        
        error = OMX_SendCommand(source, OMX_CommandPortEnable, sourcePort, NULL);
        if(error != OMX_ErrorNone)
        {
            ofLogError() << "Could not enable sourcePort " << sourcePort;
            return error;
        }
        
        
        
        error = OMX_SendCommand(destination, OMX_CommandPortEnable, destinationPort, NULL);
        if(error != OMX_ErrorNone)
        {
            ofLogError() << "Could not enable destinationPort " << destinationPort;
            return error;
        }
        return error;
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
        
        //LOGGER << "decoder DISABLE START";
        error = disableAllPortsForComponent(decoder);
        OMX_TRACE(error);
        //LOGGER << "decoder DISABLE END";
        
        
        OMX_CALLBACKTYPE  rendererCallbacks;
        rendererCallbacks.EventHandler    = &Decoder::onRenderEvent;
        rendererCallbacks.EmptyBufferDone = &Decoder::onRenderEmptyBufferDone;
        rendererCallbacks.FillBufferDone  = &Decoder::onRenderFillBufferDone;
        
        error = OMX_GetHandle(&renderer, OMX_VIDEO_RENDER, this, &rendererCallbacks);
        OMX_TRACE(error);
        
        //LOGGER << "renderer DISABLE START";
        error = disableAllPortsForComponent(renderer);
        OMX_TRACE(error);
        //LOGGER << "renderer DISABLE END";
        
        
        OMX_CALLBACKTYPE  schedulerCallbacks;
        schedulerCallbacks.EventHandler    = &Decoder::onSchedulerEvent;
        schedulerCallbacks.EmptyBufferDone = &Decoder::onSchedulerEmptyBufferDone;
        schedulerCallbacks.FillBufferDone  = &Decoder::onSchedulerFillBufferDone;
        
        error = OMX_GetHandle(&scheduler, OMX_VIDEO_SCHEDULER, this, &schedulerCallbacks);
        OMX_TRACE(error);
        
        //LOGGER << "scheduler DISABLE START";
        error = disableAllPortsForComponent(scheduler);
        OMX_TRACE(error);
        //LOGGER << "scheduler DISABLE END";
        
        
        //clock->scheduler
        error = createTunnel(clock->handle, OMX_CLOCK_OUTPUT_PORT0, scheduler, VIDEO_SCHEDULER_CLOCK_PORT);
        OMX_TRACE(error);
        if(error == OMX_ErrorNone)
        {
            //LOGGER << "clock->scheduler SUCCESS";
        }
        STALL(3);
        
        
        OMX_VIDEO_PARAM_PORTFORMATTYPE formatType;
        OMX_INIT_STRUCTURE(formatType);
        formatType.nPortIndex = VIDEO_DECODER_INPUT_PORT;
        formatType.eCompressionFormat = stream->omxCodingType;
        
        if (stream->fpsScale > 0 && stream->fpsRate > 0)
        {
            formatType.xFramerate = (long long)(1<<16) * stream->fpsRate / stream->fpsScale;
        }
        else
        {
            formatType.xFramerate = 25 * (1<<16);
        }
        //LOGGER << "formatType.xFramerate: " << formatType.xFramerate;
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
        
        //LOGGER << "stream->doFormatting: " << stream->doFormatting;
        
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
        
        error = OMX_SendCommand(decoder, OMX_CommandPortEnable, VIDEO_DECODER_INPUT_PORT, NULL);
        OMX_TRACE(error);
        
        OMX_PARAM_PORTDEFINITIONTYPE portFormat;
        OMX_INIT_STRUCTURE(portFormat);
        portFormat.nPortIndex = VIDEO_DECODER_INPUT_PORT;
        
        error = OMX_GetParameter(decoder, OMX_IndexParamPortDefinition, &portFormat);
        OMX_TRACE(error);
        
        //LOGGER << "decoder portFormat.bEnabled: " << portFormat.bEnabled;
        //LOGGER << "decoder portFormat.nBufferCountActual: " << portFormat.nBufferCountActual;
        //LOGGER << "decoder portFormat.nBufferSize: " << portFormat.nBufferSize;
        
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
        
        bool useScheduler = true;

        if(useScheduler)
        {
            error = createTunnel(decoder, VIDEO_DECODER_OUTPUT_PORT, scheduler, VIDEO_SCHEDULER_INPUT_PORT);
            OMX_TRACE(error);
            
            if(error == OMX_ErrorNone)
            {
                //LOGGER << "decoder->scheduler SUCCESS";
            }
            STALL(3);
            
            error = createTunnel(scheduler, VIDEO_SCHEDULER_OUTPUT_PORT, renderer, VIDEO_RENDER_INPUT_PORT);
            OMX_TRACE(error);
            if(error == OMX_ErrorNone)
            {
                //LOGGER << "scheduler->renderer SUCCESS";
            }
            STALL(3);
            
        }else
        {
            error = createTunnel(decoder, VIDEO_DECODER_OUTPUT_PORT, renderer, VIDEO_RENDER_INPUT_PORT);
            OMX_TRACE(error);
        }
       
            
       
#if 0   
        //allocate render input buffers (unused with direct)
        portFormat.nPortIndex = VIDEO_RENDER_INPUT_PORT;
        error = OMX_GetParameter(renderer, OMX_IndexParamPortDefinition, &portFormat);
        OMX_TRACE(error);
        
        //LOGGER << "renderer portFormat.bEnabled: " << portFormat.bEnabled;
        //LOGGER << "renderer portFormat.nBufferCountActual: " << portFormat.nBufferCountActual;
        //LOGGER << "renderer portFormat.nBufferSize: " << portFormat.nBufferSize;
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
#endif       
        clock->start(0.0);
        
        //start decoder
        error = OMX_SendCommand(decoder, OMX_CommandStateSet, OMX_StateExecuting, 0);
        OMX_TRACE(error);
        
        if(useScheduler)
        {
            //start scheduler
            error = OMX_SendCommand(scheduler, OMX_CommandStateSet, OMX_StateExecuting, 0);
            OMX_TRACE(error);
            STALL(3);
        }
        
        //start renderer
        error = OMX_SendCommand(renderer, OMX_CommandStateSet, OMX_StateExecuting, 0);
        OMX_TRACE(error);
        
        
        
        OMX_CONFIG_DISPLAYREGIONTYPE displayConfig;
        OMX_INIT_STRUCTURE(displayConfig);
        displayConfig.nPortIndex = VIDEO_RENDER_INPUT_PORT;
        displayConfig.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_MODE);
        displayConfig.fullscreen  = OMX_TRUE; 
        displayConfig.mode  = OMX_DISPLAY_MODE_FILL; 
        error = OMX_SetConfig(renderer, OMX_IndexConfigDisplayRegion, &displayConfig);
        OMX_TRACE(error);
       
        
        if(stream->codecExtraSize > 0 && stream->codecExtraData != NULL)
        {
            int extraSize = stream->codecExtraSize;
            uint8_t* extraData = (uint8_t *)malloc(extraSize);
            memcpy(extraData, stream->codecExtraData, extraSize);
            
            OMX_BUFFERHEADERTYPE* omxBuffer = getInputBuffer(500);
            if(omxBuffer)
            {
                omxBuffer->nOffset = 0;
                omxBuffer->nFilledLen = extraSize;
                memset((unsigned char *)omxBuffer->pBuffer, 0x0, omxBuffer->nAllocLen);
                memcpy((unsigned char *)omxBuffer->pBuffer, extraData, omxBuffer->nFilledLen);
                omxBuffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;
                error = OMX_EmptyThisBuffer(decoder, omxBuffer); 
            }else
            {
                ofLogError() << "NO INPUT BUFFER";
                STALL(2);

            }
            
        }  
        
    }
    
    
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
    
    
    
    
    
    OMX_BUFFERHEADERTYPE* getInputBuffer(long timeout)
    {
        if(!decoder) 
        {
            ofLogError(__func__) << " NO decoder";
        }
        
        OMX_BUFFERHEADERTYPE* inputBuffer = NULL;
        
        
        pthread_mutex_lock(&m_omx_input_mutex);
        struct timespec endtime;
        clock_gettime(CLOCK_REALTIME, &endtime);
        add_timespecs(endtime, timeout);
        while (1)
        {
            if(!decoderInputBuffersAvailable.empty())
            {
                inputBuffer = decoderInputBuffersAvailable.front();
                decoderInputBuffersAvailable.pop();
                break;
            }
            
            int retcode = pthread_cond_timedwait(&m_input_buffer_cond, &m_omx_input_mutex, &endtime);
            if (retcode != 0)
            {
                ofLogError(__func__) << "DECODER TIMEOUT AT: " << timeout;
                break;
            }
        }
        pthread_mutex_unlock(&m_omx_input_mutex);
        return inputBuffer;
    }

    void decodeNext()
    {
        packetCounter++;
        if(packetCounter<videoFile->omxPackets.size())
        {
            decodeOMXPacket(videoFile->omxPackets[packetCounter]);
        }else
        {
            //LOGGER << "END OF PACKETS";
            STALL(5);
            ofExit();
        }
        
    }
    bool decodeOMXPacket(OMXPacket* pkt)
    {
        
        if(!pkt)
        {
            return false;
        }
        
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
        
        stringstream info;
        info << endl;
        info << "packetCounter: " << packetCounter << endl;
        info << "pts: " << pts << endl;
        info << "pkt->pts: " << pkt->pts << endl;
        info << "dts: " << dts << endl;
        info << "pkt->dts: " << pkt->dts << endl;
        info << "currentPTS: " << currentPTS << endl;
        info << "getMediaTime: " << clock->getMediaTime();

        //LOGGER << info.str();
        
        //ofLog(OF_LOG_VERBOSE, "decode dts:%.0f pts:%.0f cur:%.0f, size:%d", pkt->dts, pkt->pts, currentPTS, pkt->size);
        result = decode(pkt->data, pkt->size, dts, pts);
        
        return result;
    }
    bool decode(uint8_t* demuxer_content, int iSize, double dts, double pts)
    {
        
        //SingleLock lock (m_critSection);
        
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
                //LOGGER << "decoderInputBuffersAvailable: " << decoderInputBuffersAvailable.size();
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
                    //LOGGER << "USING nAllocLen: "<< omxBuffer->nAllocLen;
                    omxBuffer->nFilledLen = omxBuffer->nAllocLen;
                }else
                {
                    //LOGGER << "USING demuxer_bytes: " << demuxer_bytes;

                    omxBuffer->nFilledLen = demuxer_bytes;
                }                
                //LOGGER << "demuxer_bytes: " << demuxer_bytes;
                //LOGGER << "omxBuffer->nFilledLen: " << omxBuffer->nFilledLen;
                //LOGGER << "omxBuffer->nTimeStamp: " << FromOMXTime(omxBuffer->nTimeStamp);

                memcpy(omxBuffer->pBuffer, demuxer_content, omxBuffer->nFilledLen);

                demuxer_bytes -= omxBuffer->nFilledLen;
                demuxer_content += omxBuffer->nFilledLen;


                if(demuxer_bytes == 0)
                {
                    //LOGGER << "OMX_BUFFERFLAG_ENDOFFRAME";
                    EOFCounter++;
                    omxBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
                    ofLog() <<  "EOFCounter: " << EOFCounter;
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
        //LOGGER << "";
        return OMX_ErrorNone;
    }
    
    static OMX_ERRORTYPE onRenderEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
    {	
        //LOGGER << "";
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
        LOGGER << "EVENT TYPE: " << OMX_Maps::getInstance().eventTypes[eEvent];
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        switch (eEvent) 
        {
            case OMX_EventCmdComplete:
            {
                break;
            }
            case OMX_EventParamOrConfigChanged:
            {
                
                LOGGER << "OMX_EventParamOrConfigChanged";
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
        //LOGGER << "";
        return OMX_ErrorNone;
    }
    
    static OMX_ERRORTYPE onSchedulerEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
    {	
        //LOGGER << "";
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
        LOGGER << "EVENT TYPE: " << OMX_Maps::getInstance().eventTypes[eEvent];
        
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
                LOGGER << "OMX_EventParamOrConfigChanged";
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
        //LOGGER << "";
        return OMX_ErrorNone;
    }
    
    static OMX_ERRORTYPE onDecoderEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
    {	
        //LOGGER << "";
        Decoder* myDecoder = (Decoder*)pAppData;
        
        
        
        pthread_mutex_lock(&myDecoder->m_omx_input_mutex);
        myDecoder->decoderInputBuffersAvailable.push(pBuffer);
        
        // this allows (all) blocked tasks to be awoken
        pthread_cond_broadcast(&myDecoder->m_input_buffer_cond);
        
        pthread_mutex_unlock(&myDecoder->m_omx_input_mutex);
        
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
        LOGGER << "EVENT TYPE: " << OMX_Maps::getInstance().eventTypes[eEvent];
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        
        OMX_STATETYPE state;
        error = OMX_GetState(hComponent, &state);
        OMX_TRACE(error);
        //LOGGER << "state: " << OMX_Maps::getInstance().omxStateNames[state];
        
        switch (eEvent) 
        {
            case OMX_EventCmdComplete:
            {
                break;
            }
            case OMX_EventParamOrConfigChanged:
            {
                
                //return decoder->onCameraEventParamOrConfigChanged();
                LOGGER << "OMX_EventParamOrConfigChanged";
                Decoder *decoder = static_cast<Decoder*>(pAppData);
                decoder->isDecoderReady = true;
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
