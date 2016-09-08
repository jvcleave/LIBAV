#pragma once

#include "VideoFile.h"
#include "ClockComponent.h"
#include "DecoderComponent.h"


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


class Decoder: public ComponentListener
{
public:
    
    VideoFile* videoFile;
    Stream* stream;
    
    ClockComponent clockComponent;
    DecoderComponent decoderComponent;
    RenderComponent renderComponent;
    SchedulerComponent schedulerComponent;

    int EOFCounter;
    

    int packetCounter = 0;
    bool doSetStartTime;

    CriticalSection  m_critSection;
    double currentPTS;
    bool PortSettingsChanged;
    bool useScheduler;
    bool isDecoding;
   
    std::mutex decoderMutex;
    Decoder()
    {
        videoFile = NULL;
        stream = NULL;
        packetCounter = 0;
        currentPTS = 0;
        doSetStartTime = true;

        EOFCounter = 0;
        PortSettingsChanged = false;
        useScheduler = false;
        isDecoding = false;

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
        
    void setup(VideoFile* videoFile_)
    {
        
        OMX_Init();
        
        videoFile = videoFile_;
        stream = videoFile->getBestVideoStream();
        
        clockComponent.setup();
        bool useAudioRef = false;

        clockComponent.setReference(useAudioRef);
        
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        
        
        decoderComponent.setup(this);

        if(renderComponent.setup())
        {
            ofLogVerbose(__func__) << "renderComponent PASS";
        }
        
        
        if(schedulerComponent.setup())
        {
            ofLogVerbose(__func__) << "schedulerComponent PASS";
        }
        
        
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
        error = OMX_SetParameter(decoderComponent.handle, OMX_IndexParamVideoPortFormat, &formatType);
        OMX_TRACE(error);
        
        
        
        OMX_PARAM_PORTDEFINITIONTYPE portParam;
        OMX_INIT_STRUCTURE(portParam);
        portParam.nPortIndex = VIDEO_DECODER_INPUT_PORT;
        
        error = OMX_GetParameter(decoderComponent.handle, OMX_IndexParamPortDefinition, &portParam);
        OMX_TRACE(error);
        
        portParam.nBufferCountActual = 20;
        portParam.format.video.nFrameWidth  = stream->width;
        portParam.format.video.nFrameHeight = stream->height;
        error = OMX_SetParameter(decoderComponent.handle, OMX_IndexParamPortDefinition, &portParam);
        OMX_TRACE(error);
        
        OMX_CONFIG_REQUESTCALLBACKTYPE notifications;
        OMX_INIT_STRUCTURE(notifications);
        notifications.nPortIndex = VIDEO_DECODER_OUTPUT_PORT;
        notifications.nIndex = OMX_IndexParamBrcmPixelAspectRatio;
        notifications.bEnable = OMX_TRUE;
        error = OMX_SetParameter(decoderComponent.handle, OMX_IndexConfigRequestCallback, &notifications);
        OMX_TRACE(error);

        
        
        OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
        OMX_INIT_STRUCTURE(concanParam);
        concanParam.bStartWithValidFrame = OMX_FALSE;
        
        error = OMX_SetParameter(decoderComponent.handle, OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam);
        OMX_TRACE(error);
        
        //LOGGER << "stream->doFormatting: " << stream->doFormatting;
        
        //
        if(stream->doFormatting)
        {
            OMX_NALSTREAMFORMATTYPE nalStreamFormat;
            OMX_INIT_STRUCTURE(nalStreamFormat);
            nalStreamFormat.nPortIndex = VIDEO_DECODER_INPUT_PORT;
            nalStreamFormat.eNaluFormat = OMX_NaluFormatStartCodes;
            error = OMX_SetParameter(decoderComponent.handle, (OMX_INDEXTYPE)OMX_IndexParamNalStreamFormatSelect, &nalStreamFormat);
            OMX_TRACE(error);
        }
        
        OMX_CONFIG_BOOLEANTYPE timeStampMode;
        OMX_INIT_STRUCTURE(timeStampMode);
        timeStampMode.bEnabled = OMX_TRUE;
        error = OMX_SetParameter(decoderComponent.handle, OMX_IndexParamBrcmVideoTimestampFifo, &timeStampMode);
        OMX_TRACE(error);
        
        error = OMX_SendCommand(decoderComponent.handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        OMX_TRACE(error);
        
        error = OMX_SendCommand(decoderComponent.handle, OMX_CommandPortEnable, VIDEO_DECODER_INPUT_PORT, NULL);
        OMX_TRACE(error);
        
        OMX_PARAM_PORTDEFINITIONTYPE portFormat;
        OMX_INIT_STRUCTURE(portFormat);
        portFormat.nPortIndex = VIDEO_DECODER_INPUT_PORT;
        
        error = OMX_GetParameter(decoderComponent.handle, OMX_IndexParamPortDefinition, &portFormat);
        OMX_TRACE(error);
        
        //LOGGER << "decoderComponent.handle portFormat.bEnabled: " << portFormat.bEnabled;
        //LOGGER << "decoderComponent.handle portFormat.nBufferCountActual: " << portFormat.nBufferCountActual;
        //LOGGER << "decoderComponent.handle portFormat.nBufferSize: " << portFormat.nBufferSize;
        
        for (size_t i = 0; i < portFormat.nBufferCountActual; i++)
        {
            OMX_BUFFERHEADERTYPE *buffer = NULL;
            //OMX_U8* data = NULL;
            
            error = OMX_AllocateBuffer(decoderComponent.handle, &buffer, VIDEO_DECODER_INPUT_PORT, NULL, portFormat.nBufferSize);
            OMX_TRACE(error);
            
            buffer->nInputPortIndex = VIDEO_DECODER_INPUT_PORT;
            buffer->nFilledLen      = 0;
            buffer->nOffset         = 0;
            buffer->pAppPrivate     = (void*)i;
            decoderComponent.inputBuffers.push_back(buffer);
            decoderComponent.inputBuffersAvailable.push(buffer);
        }
 
        //start decoderComponent.handle
        error = OMX_SendCommand(decoderComponent.handle, OMX_CommandStateSet, OMX_StateExecuting, 0);
        OMX_TRACE(error);
        
        
        SendDecoderConfig();
         
        
    }
    
    void SendDecoderConfig()
    {
       // SingleLock lock (m_critSection);
        if(stream->codecExtraSize > 0 && stream->codecExtraData != NULL)
        {
            int extraSize = stream->codecExtraSize;
            uint8_t* extraData = (uint8_t *)malloc(extraSize);
            memcpy(extraData, stream->codecExtraData, extraSize);
            
            OMX_BUFFERHEADERTYPE* omxBuffer = decoderComponent.getInputBuffer(500);
            if(omxBuffer)
            {
                omxBuffer->nOffset = 0;
                omxBuffer->nFilledLen = extraSize;
                memset((unsigned char *)omxBuffer->pBuffer, 0x0, omxBuffer->nAllocLen);
                memcpy((unsigned char *)omxBuffer->pBuffer, extraData, omxBuffer->nFilledLen);
                omxBuffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;
               
                OMX_ERRORTYPE error = OMX_EmptyThisBuffer(decoderComponent.handle, omxBuffer); 
                OMX_TRACE(error);
                
                while(!PortSettingsChanged)
                {
                    decodeNext();    
                }
                
            }else
            {
                ofLogError() << "NO INPUT BUFFER";
                STALL(2);
                
            }
            
        } 
    }
    void onPortSettingsChanged()
    {
        PortSettingsChanged = true;
        ofLog() << "PortSettingsChanged: " << PortSettingsChanged << " isDecoding: " << isDecoding;
        OMX_ERRORTYPE error;

        ofLogVerbose(__func__) << "START";
        
       
        
        //clockComponent.schedulerComponent.handle
        error = createTunnel(clockComponent.handle, OMX_CLOCK_OUTPUT_PORT0, schedulerComponent.handle, VIDEO_SCHEDULER_CLOCK_PORT);
        OMX_TRACE(error);
        if(error == OMX_ErrorNone)
        {
            LOGGER << "clockComponent.schedulerComponent.handle SUCCESS";
        }
        
        
        
        
        
        if(useScheduler)
        {
            error = createTunnel(decoderComponent.handle, VIDEO_DECODER_OUTPUT_PORT, schedulerComponent.handle, VIDEO_SCHEDULER_INPUT_PORT);
            OMX_TRACE(error);
            
            if(error == OMX_ErrorNone)
            {
                //LOGGER << "decoderComponent.handle->schedulerComponent.handle SUCCESS";
            }
            STALL(3);
            
            error = createTunnel(schedulerComponent.handle, VIDEO_SCHEDULER_OUTPUT_PORT, renderComponent.handle, VIDEO_RENDER_INPUT_PORT);
            OMX_TRACE(error);
            if(error == OMX_ErrorNone)
            {
                //LOGGER << "schedulerComponent.handle->renderComponent.handle SUCCESS";
            }
            STALL(3);
            
        }else
        {
            error = createTunnel(decoderComponent.handle, VIDEO_DECODER_OUTPUT_PORT, renderComponent.handle, VIDEO_RENDER_INPUT_PORT);
            OMX_TRACE(error);
        }
        
        
        clockComponent.start(0.0);
        

        
        if(useScheduler)
        {
            //start schedulerComponent.handle
            error = OMX_SendCommand(schedulerComponent.handle, OMX_CommandStateSet, OMX_StateExecuting, 0);
            OMX_TRACE(error);
            STALL(3);
        }
        
        //start renderComponent.handle
        error = OMX_SendCommand(renderComponent.handle, OMX_CommandStateSet, OMX_StateExecuting, 0);
        OMX_TRACE(error);
        
        
        
        OMX_CONFIG_DISPLAYREGIONTYPE displayConfig;
        OMX_INIT_STRUCTURE(displayConfig);
        displayConfig.nPortIndex = VIDEO_RENDER_INPUT_PORT;
        displayConfig.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_MODE);
        displayConfig.fullscreen  = OMX_TRUE; 
        displayConfig.mode  = OMX_DISPLAY_MODE_FILL; 
        error = OMX_SetConfig(renderComponent.handle, OMX_IndexConfigDisplayRegion, &displayConfig);
        OMX_TRACE(error);
        
        
        OMX_STATETYPE state;
        error = OMX_GetState(decoderComponent.handle, &state);
        OMX_TRACE(error);
        if(state == OMX_StateIdle)
        {
            error = OMX_SendCommand(decoderComponent.handle, OMX_CommandStateSet, OMX_StateExecuting, 0);
            OMX_TRACE(error);
        }
        
        
        PortSettingsChanged = true;
        decodeNext();

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
    
    
    void decodeNext()
    {
        if(isDecoding)
        {
            return;
        }
        decoderMutex.lock();
        isDecoding = true;
        if(!videoFile)
        {
            ofLogError(__func__) << "NO VIDEO FILE";
            
        }else
        {
            if(packetCounter < videoFile->omxPackets.size())
            {
                if(videoFile->omxPackets[packetCounter])
                {
                    if(decodeOMXPacket(videoFile->omxPackets[packetCounter]))
                    {
                        packetCounter++;
                    }
                }
                
            }
        }
        isDecoding = false;
        decoderMutex.unlock();

    }
    bool decodeOMXPacket(OMXPacket* pkt)
    {        
        bool result = false;
        if(!pkt)
        {
            ofLogError(__func__) << "pkt IS NULL";
            return false;
        }
        
        
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
        info << "getMediaTime: " << clockComponent.getMediaTime();

        //LOGGER << info.str();
        
        //ofLog(OF_LOG_VERBOSE, "decode dts:%.0f pts:%.0f cur:%.0f, size:%d", pkt->dts, pkt->pts, currentPTS, pkt->size);
        result = decode(pkt->data, pkt->size, dts, pts);
        //ofLogVerbose(__func__) << "result: " << result;
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
                if(!decoderComponent.inputBuffersAvailable.empty())
                {
                    omxBuffer = decoderComponent.inputBuffersAvailable.front();
                    decoderComponent.inputBuffersAvailable.pop();
                }
                omxBuffer->nFlags = 0;
                omxBuffer->nOffset = 0;
                
                if(doSetStartTime)
                {
                    omxBuffer->nFlags |= OMX_BUFFERFLAG_STARTTIME;
                    doSetStartTime = false;
                }
                if (pts == DVD_NOPTS_VALUE)
                {
                    omxBuffer->nFlags |= OMX_BUFFERFLAG_TIME_UNKNOWN;
                }
                
                omxBuffer->nTimeStamp = ToOMXTime((uint64_t)(pts == DVD_NOPTS_VALUE) ? 0 : pts);
                if(demuxer_bytes > omxBuffer->nAllocLen)
                {
                    omxBuffer->nFilledLen = omxBuffer->nAllocLen;
                }else
                {
                     omxBuffer->nFilledLen = demuxer_bytes;
                }                

                memcpy(omxBuffer->pBuffer, demuxer_content, omxBuffer->nFilledLen);

                demuxer_bytes -= omxBuffer->nFilledLen;
                demuxer_content += omxBuffer->nFilledLen;


                if(demuxer_bytes == 0)
                {
                    EOFCounter++;
                    omxBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
                    ofLog() <<  "EOFCounter: " << EOFCounter << " timeStamp: " << FromOMXTime(omxBuffer->nTimeStamp);
                }
                
                error = OMX_EmptyThisBuffer(decoderComponent.handle, omxBuffer);
                OMX_TRACE(error);
            }
            
            return true;
        }
        
        return false;
    }

    void onFillBuffer()
    {
    
    }
    
    void onEmptyBuffer()
    {
        if(PortSettingsChanged)
        {
            decodeNext();
        }
        
    }
    
    void onEvent(OMX_EVENTTYPE eEvent)
    {
        
        stringstream info;
        info << "EVENT TYPE: " << OMX_Maps::getInstance().eventTypes[eEvent] << endl;
        
        ofLogVerbose(__func__) << info.str();
        
        
        if(eEvent == OMX_EventPortSettingsChanged)
        {
            onPortSettingsChanged();
        }
    }
 

};
