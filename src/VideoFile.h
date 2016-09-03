#pragma once
#pragma GCC diagnostic ignored "-Wswitch"

#include "ofMain.h"
#include <sys/stat.h>

extern "C" 
{
    
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
    
#include <libswresample/swresample.h>
    
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
    
#include <libavcodec/avcodec.h>
    
#include <libavutil/avutil.h>
#include <libavutil/crc.h>
#include <libavutil/fifo.h>
#include <libavutil/opt.h>
#include <libavutil/mem.h>
#include <libavutil/file.h>
    
#include <libavformat/avformat.h>
    
}

#include <IL/OMX_Video.h>

//define STALL ofSleepMillis(2000)
#define STALL
#define MAX_OMX_STREAMS        100

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

#define DVD_TIME_TO_SEC(x)  ((int)((double)(x) / DVD_TIME_BASE))
#define DVD_TIME_TO_MSEC(x) ((int)((double)(x) * 1000 / DVD_TIME_BASE))
#define DVD_SEC_TO_TIME(x)  ((double)(x) * DVD_TIME_BASE)
#define DVD_MSEC_TO_TIME(x) ((double)(x) * DVD_TIME_BASE / 1000)

#define DVD_PLAYSPEED_PAUSE       0       // frame stepping
#define DVD_PLAYSPEED_NORMAL      1000



class Stream
{
public:
    
    
    
    AVStream*   avStream;
    bool        hasAudio;
    bool        hasVideo;
    bool isValid;
    
    AVCodecID codecID;
    AVMediaType mediaType;
    bool forceSoftwareDecoding;  //force software decoding
    OMX_VIDEO_CODINGTYPE omxCodingType;

    // VIDEO
    int fpsScale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
    int fpsRate;
    int height; 
    int width; 
    float aspect; 
    bool isVariableFrameRate; 
    bool hasStills; 
    int level; // encoder level of the stream reported by the decoder. used to qualify hw decoders.
    int profile; // encoder profile of the stream reported by the decoder. used to qualify hw decoders.
    bool isPTSValid;  // pts cannot be trusted (avi's).
    int numFrames;
    int gopSize;
    int64_t duration;
    int fps;
    
    // AUDIO
    int audioChannels;
    int audioSampleRate;
    int audioBitrate;
    int audioBlockAlign;
    int audioBitsPerSample;
    
    // CODEC EXTRADATA
    unsigned char*        codecExtraData; // extra data for codec to use
    unsigned int codecExtraSize; // size of extra data
    unsigned int codecTag; // extra identifier hints for decoding
    
    /* ac3/dts indof */
    unsigned int framesize;
    uint32_t     syncword;
    
    bool isAVI;
    bool isMKV;
    bool doFormatting;
    string mediaTypeString;
    Stream()
    {
        avStream = NULL;
        hasAudio = false;
        hasVideo = false;
        isValid = false;
        forceSoftwareDecoding = false;
        isAVI = false;
        isMKV = false;
        doFormatting =false;
        fpsScale = 0;
        height = 0;
        width = 0;
        aspect = 0;
        isVariableFrameRate = false;
        hasStills = false;
        level = 0;
        profile = 0;
        isPTSValid = false;
        numFrames = 0;
        gopSize = 0;
        duration = 0;
        fps = 0;
        audioChannels = 0;
        audioChannels = 0;
        audioSampleRate = 0;
        audioBitrate = 0;
        audioBlockAlign = 0;
        audioBitsPerSample = 0;
        
        codecExtraData = NULL;
        codecExtraSize = 0;
        codecTag = 0;
        codecID = AV_CODEC_ID_NONE;

        framesize = 0;
        syncword = 0;
        isAVI = false;
        isMKV = false;
        doFormatting = false;
        mediaTypeString = "";
        mediaType = AVMEDIA_TYPE_UNKNOWN;
        omxCodingType   = OMX_VIDEO_CodingUnused;
    }
    
    bool NaluFormatStartCodes(unsigned char *in_extradata, int in_extrasize)
    {
        switch(codecID)
        {
            case AV_CODEC_ID_H264:
            {
                if (in_extrasize < 7 || in_extradata == NULL)
                {
                    return true;
                }
                // valid avcC atom data always starts with the value 1 (version), otherwise annexb
                else if ( *in_extradata != 1 )
                {
                    return true;
                }
            }
            default:
                break;
        }
        return false;
    }
    
    bool setup(AVStream* avStream_)
    {
        avStream = avStream_;
        codecID = avStream->codec->codec_id;
        mediaType = avStream->codec->codec_type;
        switch (mediaType)
        {
            case AVMEDIA_TYPE_AUDIO:
            {
                mediaTypeString = "AVMEDIA_TYPE_AUDIO";
                isValid = true;
                hasAudio = true;
                break;
            }
            case AVMEDIA_TYPE_VIDEO:
            {
                mediaTypeString = "AVMEDIA_TYPE_VIDEO";
                isValid = true;
                hasVideo = true;                
                break;
            }
            case AVMEDIA_TYPE_UNKNOWN:
            {
                mediaTypeString = "AVMEDIA_TYPE_UNKNOWN";            
                break;
            }    
            case AVMEDIA_TYPE_DATA:
            {
                mediaTypeString = "AVMEDIA_TYPE_DATA";            
                break;
            } 
            case AVMEDIA_TYPE_SUBTITLE:
            {
                mediaTypeString = "AVMEDIA_TYPE_SUBTITLE";            
                break;
            }
            case AVMEDIA_TYPE_ATTACHMENT:
            {
                mediaTypeString = "AVMEDIA_TYPE_ATTACHMENT";            
                break;
            }
            case AVMEDIA_TYPE_NB:
            {
                mediaTypeString = "AVMEDIA_TYPE_NB";            
                break;
            }
            default:
            {
                mediaTypeString = "DEFAULT THROWN";
                break;
            }
        }
        if(isValid)
        {
            processCodec();
            //codecExtraData      = avStream->codec->extradata;
            //codecExtraSize      = avStream->codec->extradata_size;
            audioChannels       = avStream->codec->channels;
            audioSampleRate     = avStream->codec->sample_rate;
            audioBlockAlign     = avStream->codec->block_align;
            audioBitrate        = avStream->codec->bit_rate;
            audioBitsPerSample  = avStream->codec->bits_per_coded_sample;
            gopSize             = avStream->codec->gop_size;
            
            if(audioBitsPerSample == 0)
            {
                audioBitsPerSample = 16;
            }
            
            width         = avStream->codec->width;
            height        = avStream->codec->height;
            profile       = avStream->codec->profile;
            
            
            if(avStream->codec->extradata_size > 0 && avStream->codec->extradata != NULL)
            {
                codecExtraSize = avStream->codec->extradata_size;
                codecExtraData = (unsigned char *)malloc(codecExtraSize);
                memcpy(codecExtraData, avStream->codec->extradata, avStream->codec->extradata_size);
                ofLogVerbose(__func__) << "codecExtraSize: " << codecExtraSize;
            }
            doFormatting = NaluFormatStartCodes(codecExtraData, codecExtraSize);
            ofLogVerbose(__func__) << "doFormatting: " << doFormatting;
            
            if(avStream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                //CUSTOM
                duration    = avStream->duration;
                numFrames   = avStream->nb_frames;
                
                
                fpsRate  = avStream->r_frame_rate.num;
                fpsScale = avStream->r_frame_rate.den;
                
                if(isMKV && avStream->avg_frame_rate.den && avStream->avg_frame_rate.num)
                {
                    fpsRate     = avStream->avg_frame_rate.num;
                    fpsScale    = avStream->avg_frame_rate.den;
                }
                else 
                {
                    if(avStream->r_frame_rate.num && avStream->r_frame_rate.den)
                    {
                        fpsRate  = avStream->r_frame_rate.num;
                        fpsScale = avStream->r_frame_rate.den;
                    }else
                    {
                        fpsRate  = 0;
                        fpsScale = 0; 
                    }
                }
                
                if (avStream->sample_aspect_ratio.num != 0)
                {
                    aspect = av_q2d(avStream->sample_aspect_ratio) * avStream->codec->width / avStream->codec->height;
                }
                else
                {
                    if (avStream->codec->sample_aspect_ratio.num != 0)
                    {
                        aspect = av_q2d(avStream->codec->sample_aspect_ratio) * avStream->codec->width / avStream->codec->height;
                    }else
                    {
                        aspect = 0.0f;
                    }
                }
                if (isAVI && avStream->codec->codec_id == AV_CODEC_ID_H264)
                {
                    isPTSValid = false;
                }
            }

        }
        return isValid;
    }
    void processCodec()
    {
        switch (codecID)
        {
            case AV_CODEC_ID_H264:
            {
                switch(avStream->codec->profile)
                {
                    case FF_PROFILE_H264_BASELINE:
                        // (role name) video_decoder.avc
                        // H.264 Baseline profile
                        omxCodingType = OMX_VIDEO_CodingAVC;
                        break;
                    case FF_PROFILE_H264_MAIN:
                        // (role name) video_decoder.avc
                        // H.264 Main profile
                        omxCodingType = OMX_VIDEO_CodingAVC;
                        break;
                    case FF_PROFILE_H264_HIGH:
                        // (role name) video_decoder.avc
                        // H.264 Main profile
                        omxCodingType = OMX_VIDEO_CodingAVC;
                        break;
                    case FF_PROFILE_UNKNOWN:
                        omxCodingType = OMX_VIDEO_CodingAVC;
                        break;
                    default:
                        omxCodingType = OMX_VIDEO_CodingAVC;
                        break;
                }
            }
                break;
            case AV_CODEC_ID_MPEG4:
                // (role name) video_decoder.mpeg4
                // MPEG-4, DivX 4/5 and Xvid compatible
                omxCodingType = OMX_VIDEO_CodingMPEG4;
                break;
            case AV_CODEC_ID_MPEG1VIDEO:
            case AV_CODEC_ID_MPEG2VIDEO:
                // (role name) video_decoder.mpeg2
                // MPEG-2
                omxCodingType = OMX_VIDEO_CodingMPEG2;
                break;
            case AV_CODEC_ID_H263:
                // (role name) video_decoder.mpeg4
                // MPEG-4, DivX 4/5 and Xvid compatible
                omxCodingType = OMX_VIDEO_CodingMPEG4;
                break;
            case AV_CODEC_ID_VP6:
            case AV_CODEC_ID_VP6F:
            case AV_CODEC_ID_VP6A:
                // (role name) video_decoder.vp6
                // VP6
                omxCodingType = OMX_VIDEO_CodingVP6;
                break;
            case AV_CODEC_ID_VP8:
                // (role name) video_decoder.vp8
                // VP8
                omxCodingType = OMX_VIDEO_CodingVP8;
                break;
            case AV_CODEC_ID_THEORA:
                // (role name) video_decoder.theora
                // theora
                omxCodingType = OMX_VIDEO_CodingTheora;
                break;
            case AV_CODEC_ID_MJPEG:
            case AV_CODEC_ID_MJPEGB:
                // (role name) video_decoder.mjpg
                // mjpg
                omxCodingType = OMX_VIDEO_CodingMJPEG;
                break;
            case AV_CODEC_ID_VC1:
            case AV_CODEC_ID_WMV3:
                // (role name) video_decoder.vc1
                // VC-1, WMV9
                omxCodingType = OMX_VIDEO_CodingWMV;
                break;
            default:
                omxCodingType = OMX_VIDEO_CodingUnused;
                ofLog(OF_LOG_VERBOSE, "Video codec id unknown: %x\n", codecID);
                break;
        }
    }

    string toString()
    {
        stringstream info;
        info << "mediaTypeString: "     <<	mediaTypeString         << endl;
        if(hasVideo)
        {
            info << "width: "				<<	width					<< endl;
            info << "height: "				<<	height					<< endl;
            info << "profile: "				<<	profile					<< endl;
            info << "numFrames: "			<<	numFrames				<< endl;
            info << "framesize: "			<<	framesize				<< endl;
            info << "fpsScale: "			<<	fpsScale				<< endl;
            info << "fpsRate: "				<<	fpsRate					<< endl;
            info << "gopSize: "             <<	gopSize                 << endl;
            info << "fps: "                 <<	fps                     << endl;
        }

        info << "duration: "			<<	duration				<< endl;
        info << "aspect: "				<<	aspect					<< endl;
        info << "level: "				<<	level					<< endl;
        info << "isPTSValid: "			<<	isPTSValid				<< endl;
        info << "codecTag: "			<<	codecTag				<< endl;
        info << "codecExtraSize: "		<<	codecExtraSize			<< endl;

        if(hasAudio)
        {
            info << "audioChannels: "		<<	audioChannels			<< endl;
            info << "audioSampleRate: "		<<	audioSampleRate			<< endl;
            info << "audioBlockAlign: "		<<	audioBlockAlign			<< endl;
            info << "audioBitrate: "		<<	audioBitrate			<< endl;
            info << "audioBitsPerSample: "	<<	audioBitsPerSample		<< endl;
        }
        
        return info.str();
    }
    
};
class OMXPacket
{
public:
    
    double    pts; // pts in DVD_TIME_BASE
    double    dts; // dts in DVD_TIME_BASE
    double    now; // dts in DVD_TIME_BASE
    double    duration; // duration in DVD_TIME_BASE if available
    int       size;
    uint8_t*  data;
    int       stream_index;
    AVMediaType codec_type;
    Stream* stream;
    
    OMXPacket()
    {
        size = 0;
        stream_index = 0;
        dts  = DVD_NOPTS_VALUE;
        pts  = DVD_NOPTS_VALUE;
        now  = DVD_NOPTS_VALUE;
        duration = DVD_NOPTS_VALUE;
        codec_type = AVMEDIA_TYPE_UNKNOWN;
        stream = NULL;
    }
    
    double ConvertTimestamp(AVFormatContext* avFormatContext, int64_t pts, int den, int num)
    {
        
        if (pts == (int64_t)AV_NOPTS_VALUE)
        {
            return DVD_NOPTS_VALUE;
        }
        
        // do calculations in floats as they can easily overflow otherwise
        // we don't care for having a completly exact timestamp anyway
        double timestamp = (double)pts * num  / den;
        double starttime = 0.0f;
        
        if (avFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
        {
            starttime = (double)avFormatContext->start_time / AV_TIME_BASE;
        }
        
        if(timestamp > starttime)
        {
            timestamp -= starttime;
        }
        else if( timestamp + 0.1f > starttime )
        {
            timestamp = 0;
        }
        
        return timestamp*DVD_TIME_BASE;
    }
    
    
    
    bool setup(AVPacket* avPacket, AVFormatContext* avFormatContext, Stream* stream_)
    {
        
        if(!avPacket)
        {
            
            ofLogVerbose() << "PACKET IS NULL";
            return false;
        }

        size = avPacket->size;
        //ofLogVerbose() << "FF_INPUT_BUFFER_PADDING_SIZE: " << FF_INPUT_BUFFER_PADDING_SIZE;
        if(avPacket->data)
        {
            data = new uint8_t[size + FF_INPUT_BUFFER_PADDING_SIZE];
            memcpy(data, avPacket->data, size);
            ofLogVerbose() << "DATA COPIED: " << size;
        }else
        {
            data = NULL;
        }
        
        
        stream = stream_;
        
        
        stream_index = avPacket->stream_index;
        if(!stream)
        {
            ofLogError() << "NO STREAM!";
            STALL;
            return false;
        }
        AVStream* avStream = stream->avStream;
        
        if(!avStream)
        {
            ofLogError() << "NO avStream!";
            STALL;
            return false;
        }
        codec_type = avStream->codec->codec_type;
        
        
        dts = ConvertTimestamp(avFormatContext, avPacket->dts, avStream->time_base.den, avStream->time_base.num);
        pts = ConvertTimestamp(avFormatContext, avPacket->pts, avStream->time_base.den, avStream->time_base.num);
        duration = DVD_SEC_TO_TIME((double)avPacket->duration * avStream->time_base.num / avStream->time_base.den);
        
        
        if(avPacket->dts != (int64_t)AV_NOPTS_VALUE)
        {
            int64_t duration;
            duration = avPacket->dts;
            if(avStream->start_time != (int64_t)AV_NOPTS_VALUE)
            {
                duration -= avStream->start_time;
            }
            
            if(duration > avStream->duration)
            {
                avStream->duration = duration;
                duration = av_rescale_rnd(avStream->duration,
                                          (int64_t)avStream->time_base.num * AV_TIME_BASE, 
                                          avStream->time_base.den,
                                          AV_ROUND_NEAR_INF);
                if ((avFormatContext->duration == (int64_t)AV_NOPTS_VALUE) ||  (avFormatContext->duration != (int64_t)AV_NOPTS_VALUE && duration > avFormatContext->duration))
                {
                    avFormatContext->duration = duration;
                }
            }
        }        
        return true;
        
    }
    string toString()
    {
        stringstream info;
        info << "pts: " << pts << endl;
        info << "dts: " << dts << endl;
        info << "duration: " << duration << endl;
        info << "size: " << size << endl;
        info << "stream_index: " << stream_index << endl;
        if(stream)
        {
            info << "stream: " << stream->toString() << endl;
        }
        
        return info.str();
    }
    ~OMXPacket()
    {
        if(data)
        {
            delete data;
            data = NULL;
        }
        
    }
};
class VideoFile
{
public:
    
    
    struct BufferData {
        uint8_t *ptr = NULL;
        size_t size = 0; ///< size left in the buffer
    };
    
    
    vector<Stream*> audioStreams;
    vector<Stream*> videoStreams;
    vector<Stream*> otherStreams;
    Stream* videoStream;
    ofFile file;
    AVFormatContext* avFormatContext;
    
    AVIOContext* avIOContext;
    AVInputFormat* avInputFormat;
    AVDictionary* avDictionary;
    unsigned char* avIOContextBuffer;
    vector<OMXPacket*> omxPackets;
    
    BufferData bufferData;
    
    
    static const bool doAbort = false; 
    int numStreams;
    map<int, Stream*> streamMap;
    
    
    
    
        
    
    VideoFile()
    {
        avFormatContext = NULL;
        avIOContext = NULL;
        avDictionary = NULL;
        avIOContextBuffer=NULL;
        videoStream = NULL;
        numStreams = 0;
    }
    
    ~VideoFile()
    {
        if(avDictionary)
        {
            av_dict_free(&avDictionary); 
            avDictionary = NULL;
        }
    }
    
    bool hasAudio()
    {
        return !audioStreams.empty();
    };
    
    bool hasVideo()
    {
        return !videoStreams.empty();
    };
    
    
    
    
    
    static int onInterrupt(void*)
    {
        if(VideoFile::doAbort)
        {
            return 1;
        }
        ofLogVerbose() << "onInterrupt";
        return 0;
    }
    
    static int onWritePacket(void* bufferData_, uint8_t* buffer, int bufferSize)
    {
        ofLogVerbose() << "onWritePacket";
        
        if(onInterrupt(NULL))
        {
            return -1;
        }
        
        BufferData* bufferData = (BufferData*)bufferData_;
        ofLog(OF_LOG_VERBOSE, "onWritePacket ptr:%p size:%zu\n", bufferData->ptr, bufferData->size);
        return bufferSize;
    }
    
    
    static int onReadPacket(void* bufferData_, uint8_t* buffer, int bufferSize)
    {
        ofLogVerbose() << "onReadPacket";
        
        if(onInterrupt(NULL))
        {
            return -1;
        }
        BufferData* bufferData = (BufferData*)bufferData_;
        bufferSize = FFMIN(bufferSize, bufferData->size);
        
        ofLog(OF_LOG_VERBOSE, "onReadPacket ptr:%p size:%zu\n", bufferData->ptr, bufferData->size);
        
        memcpy(buffer, bufferData->ptr, bufferSize);
        bufferData->ptr  += bufferSize;
        bufferData->size -= bufferSize;
        return bufferSize;
    }
    
    static int64_t onSeekPacket(void* bufferData_, int64_t pos, int whence)
    {
        ofLogVerbose() << "onSeekPacket";
        if(onInterrupt(NULL))
        {
            return -1;
        }
        BufferData* bufferData = (BufferData*)bufferData_;    
        ofLogVerbose() << "onSeekPacket, bufferData->size: " << bufferData->size;
        ofLogVerbose() << "onSeekPacket, pos: " << pos;
        return 0;
        
    }
    
    string getIOContextInfo(AVIOContext* ioContext)
    {
        stringstream info;
        info << "ioContext->seekable: " << ioContext->seekable << endl;
        info << "ioContext->max_packet_size: " << ioContext->max_packet_size << endl;
        info << "ioContext->buffer_size: " << ioContext->buffer_size << endl;
        info << "ioContext->maxsize: " << ioContext->maxsize << endl;
        info << "ioContext->direct: " << ioContext->direct << endl;
        info << "ioContext->bytes_read: " << ioContext->bytes_read << endl;
        info << "ioContext->seek_count: " << ioContext->seek_count << endl;
        //info << "ioContext->orig_buffer_size: " << ioContext->orig_buffer_size << endl;
        info << "ioContext->eof_reached: " << ioContext->eof_reached << endl;
        info << "ioContext->must_flush: " << ioContext->must_flush << endl;
        info << "ioContext->pos: " << ioContext->pos << endl;
        info << "ioContext->buf_ptr: " << ioContext->buf_ptr << endl;
        info << "ioContext->buf_end: " << ioContext->buf_end << endl;
        info << "ioContext->checksum: " << ioContext->checksum << endl;
        info << "ioContext->read_packet IS NULL: " << (ioContext->read_packet == NULL) << endl;
        info << "ioContext->write_packet IS NULL: " << (ioContext->write_packet == NULL) << endl;
        info << "ioContext->seek IS NULL: " << (ioContext->seek == NULL) << endl;
        
        return info.str();
    }
    
    string getAvPacketInfo(AVPacket&  pkt)
    {
        stringstream info;
        info << "pts: " << pkt.pts << endl;
        info << "dts: " << pkt.dts << endl;
        info << "size: " << pkt.size << endl;
        info << "stream_index: " << pkt.stream_index << endl;
        info << "isKeyFrame: " << (pkt.flags & AV_PKT_FLAG_KEY) << endl;
        info << "isCorrupt: " << (pkt.flags & AV_PKT_FLAG_CORRUPT) << endl;
        info << "hasSideData: " << !(pkt.side_data == NULL) << endl;
        info << "side_data_elems: " << pkt.side_data_elems << endl;
        info << "duration: " << pkt.duration << endl;
        info << "pos: " << pkt.pos << endl;
        info << "convergence_duration: " << pkt.convergence_duration << endl;
        
        return info.str();
    }
    
    bool read()
    {
        bool result = true;
        AVPacket  pkt;
        pkt.size = 0;
        pkt.data = NULL;
        pkt.stream_index = avFormatContext->nb_streams;
        
        if(avIOContext)
        {
            avIOContext->eof_reached = 0;
            //result = false;
        }
        
        
        int frameCode = av_read_frame(avFormatContext, &pkt);
        ofLogVerbose() << "frameCode: " << frameCode;
        if (frameCode < 0)
        {
            ofLogVerbose(__func__) << "END OF FILE";
            result = false;
            STALL;
        }else
        {
            
            ofLogVerbose() << "omxPackets.size(): " << omxPackets.size();
            
            ofLogVerbose() << "AvPacketInfo: " << endl << getAvPacketInfo(pkt);

            Stream* stream = streamMap[pkt.stream_index];
            if(stream && stream->hasVideo)
            {
                OMXPacket* omxPacket = new OMXPacket();
                if(omxPacket->setup(&pkt, avFormatContext, stream))
                {
                    omxPackets.push_back(omxPacket);
                    //ofLogVerbose() << "omxPacket: " << omxPacket->toString(); 
                }else
                {
                    delete omxPacket;
                }
                
            }
            
            

            ofLogVerbose() << getIOContextInfo(avFormatContext->pb);
            STALL;
            
            if (pkt.size < 0 || pkt.stream_index >= numStreams)
            {
                //in some cases ffmpeg returns a negative packet size
                if(avFormatContext->pb && !avFormatContext->pb->eof_reached)
                {
                    ofLogError(__func__) << "negative packet size" << pkt.size;
                }
            }            
        }
        av_free_packet(&pkt);
        return result;
    }
    
    void setup(string videoPath)
    {
        file = ofFile(videoPath);
        if(file.exists())
        {
            ofLogVerbose(__func__) << "OPENING " << file.getAbsolutePath();
        }
        
        
        
        avFormatContext = avformat_alloc_context();
        avFormatContext->flags |= AVFMT_FLAG_NONBLOCK;
        
        AVIOInterruptCB interruptCallback;
        
        interruptCallback.callback= VideoFile::onInterrupt;
        interruptCallback.opaque = (void*)this;
        
        avFormatContext->interrupt_callback = interruptCallback;
        
        
        
        int result = 0;
        
        
        
        bool isStream = false;
        
        if(isStream)
        {
            result = avformat_open_input(&avFormatContext, videoPath.c_str(), avInputFormat, &avDictionary);
        }
        else
        {
            uint8_t* buffer = NULL;
            size_t bufferSize = 0;
            
            int mapResult = av_file_map(videoPath.c_str(), &buffer, &bufferSize, 0, NULL);
            
            ofLogVerbose() << "mapResult: " << mapResult << " bufferSize: " << bufferSize;
            STALL;
            
            bufferData.ptr  = buffer;
            bufferData.size  = bufferSize;
            
            int write_flag = 0;
            size_t avIOContextBufferSize = 4096;
            avIOContextBuffer = (unsigned char*)av_malloc(avIOContextBufferSize);
            
            avIOContext = avio_alloc_context(avIOContextBuffer, 
                                             avIOContextBufferSize,
                                             write_flag, 
                                             (void*)&bufferData, 
                                             &onReadPacket, 
                                             &onWritePacket, 
                                             &onSeekPacket);
            
            
            
            unsigned int byteStreamOffset = 0;
            unsigned int maxProbeSize = 0;
            void* logContext = NULL;
            
            int probeResult = av_probe_input_buffer(avIOContext, 
                                                    &avInputFormat, 
                                                    videoPath.c_str(), 
                                                    logContext, 
                                                    byteStreamOffset, 
                                                    maxProbeSize);
            ofLogVerbose() << "probeResult: " << probeResult;
            STALL;
            
            
            int openInputResult = avformat_open_input(&avFormatContext, videoPath.c_str(), avInputFormat, &avDictionary);
            ofLogVerbose() << "openInputResult: " << openInputResult;
            if(openInputResult == AVERROR_INVALIDDATA)
            {
                ofLogVerbose() << "AVERROR_INVALIDDATA";
                STALL;
            }
            
            avFormatContext->pb = avIOContext;
            
            int infoResult = avformat_find_stream_info(avFormatContext, NULL);
            ofLogVerbose() << "infoResult: " << infoResult;
            
            int numPrograms = avFormatContext->nb_programs;
            ofLogVerbose() << "numPrograms: " << numPrograms;
            
            ofLogVerbose() << "avIOContext getIOContextInfo: " << getIOContextInfo(avIOContext);
            
            
            numStreams = avFormatContext->nb_streams;
            
            
            for (size_t i = 0; i < numStreams; i++)
            {
                Stream* stream = new Stream();
                streamMap[i] = stream;
                
                bool isValidStream = stream->setup(avFormatContext->streams[i]);
                if(isValidStream)
                {
                    if(stream->hasVideo)
                    {
                        videoStreams.push_back(stream);
                    }else
                    {
                        if(stream->hasAudio)
                        {
                            audioStreams.push_back(stream);
                        }
                        
                    }
                    ofLogVerbose() << stream->toString();
                }else
                {
                    otherStreams.push_back(stream);
                }
                
                ofLogVerbose() << "STREAM INFO " << i << stream->toString();
                
            }

            ofLogVerbose() << "audioStreams: " << audioStreams.size();
            ofLogVerbose() << "videoStreams: " << videoStreams.size();
            ofLogVerbose() << "otherStreams: " << otherStreams.size();
            STALL;
            
            
            bool isOutput = false;
            int index = 0;
            av_dump_format(avFormatContext, index, videoPath.c_str(), isOutput);
            
            av_file_unmap(buffer, bufferSize);
            
            
        }
        
        int counter = 0;
        while(read())
        {
            counter++;
            ofLog() << "counter: " << counter;
        }
        ofLogVerbose() << "result: " << result;
        
    }
    
    Stream* getBestVideoStream()
    {
        Stream* result = NULL;
        
        if(hasVideo())
        {
            if(!videoStreams.empty())
            {
                result =  videoStreams.front();
            }
            
        }else
        {
            ofLogError(__func__) << "NO VIDEOS";
        }
        return result;
    };
    
    
};
