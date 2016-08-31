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

class StreamInfo
{
public:
    StreamInfo()
    {
        codecExtraData = NULL;
        codecExtraSize = 0;
        forceSoftwareDecoding = false;
        isAVI = false;
        isMKV = false;
        doFormatting =false;
    }
    
    AVCodecID codec;
    bool forceSoftwareDecoding;  //force software decoding
    
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
    
    bool NaluFormatStartCodes(enum AVCodecID codec, unsigned char *in_extradata, int in_extrasize)
    {
        switch(codec)
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

    void setup(AVStream *avStream)
    {
        codec               = avStream->codec->codec_id;
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
        doFormatting = NaluFormatStartCodes(codec, codecExtraData, codecExtraSize);
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
    string toString()
    {
        stringstream info;
        info << "width: "				<<	width					<< "\n";
        info << "height: "				<<	height					<< "\n";
        info << "profile: "				<<	profile					<< "\n";
        info << "numFrames: "			<<	numFrames				<< "\n";
        info << "duration: "			<<	duration				<< "\n";
        info << "framesize: "			<<	framesize				<< "\n";
        info << "fpsScale: "			<<	fpsScale				<< "\n";
        info << "fpsRate: "				<<	fpsRate					<< "\n";
        info << "aspect: "				<<	aspect					<< "\n";
        info << "level: "				<<	level					<< "\n";
        info << "isPTSValid: "			<<	isPTSValid				<< "\n";
        info << "codecTag: "			<<	codecTag				<< "\n";
        info << "codecExtraSize: "		<<	codecExtraSize			<< "\n";
        info << "gopSize: "             <<	gopSize                 << "\n";
        info << "fps: "                 <<	fps                     << "\n";
        info << "audioChannels: "		<<	audioChannels			<< "\n";
        info << "audioSampleRate: "		<<	audioSampleRate			<< "\n";
        info << "audioBlockAlign: "		<<	audioBlockAlign			<< "\n";
        info << "audioBitrate: "		<<	audioBitrate			<< "\n";
        info << "audioBitsPerSample: "	<<	audioBitsPerSample		<< "\n";
        
        return info.str();
    }
};


class Stream
{
public:
    Stream()
    {
        avStream = NULL;
        hasAudio = false;
        hasVideo = false;
    }
    
    bool setup(AVStream *avStream_)
    {
        avStream = avStream_;
        bool isValid =false;
        switch (avStream->codec->codec_type)
        {
            case AVMEDIA_TYPE_AUDIO:
            {
                ofLogVerbose() << "AVMEDIA_TYPE_AUDIO";
                isValid = true;
                hasAudio = true;
                break;
            }
            case AVMEDIA_TYPE_VIDEO:
            {
                ofLogVerbose() << "AVMEDIA_TYPE_VIDEO";
                isValid = true;
                hasVideo = true;                
                break;
            }
            default:
            {
                ofLogVerbose() << "DEFAULT THROWN";
                break;
            }
        }
        
        if(isValid)
        {
           streamInfo.setup(avStream); 
        }
        return isValid;
    }
    
    AVStream*   avStream;
    StreamInfo  streamInfo;
    bool        hasAudio;
    bool        hasVideo;
};

class VideoFile
{
public:
    vector<Stream> audioStreams;
    vector<Stream> videoStreams;
    Stream* videoStream;
    
    static const bool doAbort = false; 
    VideoFile()
    {
        videoStream = NULL;
    }
    bool hasAudio()
    {
        return !audioStreams.empty();
    };
    
    bool hasVideo()
    {
        return !videoStreams.empty();
    };
    
    
    
    struct BufferData {
        uint8_t *ptr = NULL;
        size_t size = 0; ///< size left in the buffer
    };
    
    static int interrupt_cb(void*)
    {
        if(VideoFile::doAbort)
        {
            return 1;
        }
        ofLogVerbose() << "interrupt_cb";
        return 0;
    }
    
    static int onReadPacket(void* bufferData_, uint8_t* buffer, int bufferSize)
    {
        if(interrupt_cb(NULL))
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
        if(interrupt_cb(NULL))
        {
            return -1;
        }
        BufferData* bufferData = (BufferData*)bufferData_;    
        ofLogVerbose() << "onSeekPacket, bufferData->size: " << bufferData->size;
        ofLogVerbose() << "onSeekPacket, pos: " << pos;
        return 0;
        
    }
    
    
    
    void setup(string videoPath)
    {
        unsigned char* buffer = NULL;
        size_t bufferSize;
        unsigned char* avIOContextBuffer = NULL;
        
        size_t avIOContextBufferSize = 4096;
        
        AVFormatContext* avFormatContext = avformat_alloc_context();
        avFormatContext->flags |= AVFMT_FLAG_NONBLOCK;
        const AVIOInterruptCB interruptCallback = { interrupt_cb, NULL };
        avFormatContext->interrupt_callback = interruptCallback;
        
        AVIOContext* avIOContext = NULL;
        AVInputFormat* avInputFormat  = NULL;
        AVDictionary* avDictionary = NULL;
        
        int result = 0;
        bool isStream = false;
        
        if(isStream)
        {
            result = avformat_open_input(&avFormatContext, videoPath.c_str(), avInputFormat, &avDictionary);
            av_dict_free(&avDictionary);
        }
        else
        {
            int mapResult = av_file_map(videoPath.c_str(), &buffer, &bufferSize, 0, NULL);
            ofLogVerbose() << "mapResult: " << mapResult;
            
            BufferData bufferData;
            int write_flag = 0;
            
            avIOContextBuffer = (unsigned char*)av_malloc(avIOContextBufferSize);
            if(avIOContextBuffer)
            {
                ofLogVerbose() << "avIOContextBuffer PASS";
            }
            
            avIOContext = avio_alloc_context(avIOContextBuffer, 
                                             avIOContextBufferSize,
                                             write_flag, 
                                             (void*)&bufferData, 
                                             &onReadPacket, 
                                             NULL, 
                                             &onSeekPacket);
            
            if(avIOContext)
            {
                ofLogVerbose() << "avIOContext PASS";
            }
            
            if(avIOContext->max_packet_size)
            {
                ofLogVerbose() << "avIOContext->max_packet_size: " << avIOContext->max_packet_size;
            }
            
            
            
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
            
            if(avInputFormat)
            {
                ofLogVerbose() << "avInputFormat: PASS";
                
            }else
            {
                ofLogVerbose() << "avInputFormat: FAIL";
                
            }
            
            
            //TODO: causing crash/error here
            //avFormatContext->pb = avIOContext;
            
            
            
            int openInputResult = avformat_open_input(&avFormatContext, videoPath.c_str(), avInputFormat, &avDictionary);
            ofLogVerbose() << "openInputResult: " << openInputResult;
            if(openInputResult == AVERROR_INVALIDDATA)
            {
                ofLogVerbose() << "AVERROR_INVALIDDATA";
                
            }
            
            
            int infoResult = avformat_find_stream_info(avFormatContext, NULL);
            ofLogVerbose() << "infoResult: " << infoResult;
            
            int numPrograms = avFormatContext->nb_programs;
            ofLogVerbose() << "numPrograms: " << numPrograms;
            
            
            if(avFormatContext->pb)
            {
                AVIOContext* ioContext = avFormatContext->pb;
                
                ofLogVerbose() << "ioContext->seekable: " << ioContext->seekable;
                ofLogVerbose() << "ioContext->max_packet_size: " << ioContext->max_packet_size;
                ofLogVerbose() << "ioContext->buffer_size: " << ioContext->buffer_size;
                ofLogVerbose() << "ioContext->maxsize: " << ioContext->maxsize;
                ofLogVerbose() << "ioContext->direct: " << ioContext->direct;
                ofLogVerbose() << "ioContext->bytes_read: " << ioContext->bytes_read;
                ofLogVerbose() << "ioContext->seek_count: " << ioContext->seek_count;
                //ofLogVerbose() << "ioContext->orig_buffer_size: " << ioContext->orig_buffer_size;
                ofLogVerbose() << "ioContext->eof_reached: " << ioContext->eof_reached;
                ofLogVerbose() << "ioContext->must_flush: " << ioContext->must_flush;
                ofLogVerbose() << "ioContext->pos: " << ioContext->pos;
                ofLogVerbose() << "ioContext->buf_ptr: " << ioContext->buf_ptr;
                ofLogVerbose() << "ioContext->buf_end: " << ioContext->buf_end;
                ofLogVerbose() << "ioContext->checksum: " << ioContext->checksum;
                
                
                
                
                
                ofLogVerbose() << "ioContext->read_packet IS NULL: " << (ioContext->read_packet == NULL);
                ofLogVerbose() << "ioContext->write_packet IS NULL: " << (ioContext->write_packet == NULL);
                ofLogVerbose() << "ioContext->seek IS NULL: " << (ioContext->seek == NULL);
                
                
                
                
                ofLogVerbose() << "ioContext->opaque IS NULL: " << (ioContext->opaque == NULL);
                if(ioContext->opaque)
                {
                    ofLogVerbose() << "IS SAME FILE" << (ioContext->opaque == avIOContext->opaque);
                }
                
                
            }else
            {
                ofLogVerbose() << "avFormatContext->pb IS NULL";
            }
            
            
            
            ofLogVerbose() << "avFormatContext->nb_streams: " << avFormatContext->nb_streams;
            
            for (size_t i = 0; i < avFormatContext->nb_streams; i++)
            {
                Stream stream;
                bool isValidStream = stream.setup(avFormatContext->streams[i]);
                if(isValidStream)
                {
                    
                    if(stream.hasAudio)
                    {
                        //audioStreams.push_back(stream);
                    }
                    if(stream.hasVideo)
                    {
                        videoStreams.push_back(stream);
                    }
                    ofLogVerbose() << stream.streamInfo.toString();
                }
            }
            ofLogVerbose() << "hasAudio(): " << hasAudio();
            ofLogVerbose() << "hasVideo(): " << hasVideo();
            
            
            
            bool isOutput = false;
            int index = 0;
            av_dump_format(avFormatContext, index, videoPath.c_str(), isOutput);
            
        }
        
        ofLogVerbose() << "result: " << result;
        
    }
    
    Stream* getBestVideoStream()
    {
        Stream* result = NULL;
        
        if(hasVideo())
        {
            result =  &videoStreams.front();
        }else
        {
            ofLogError(__func__) << "NO VIDEOS";
        }
        return result;
    };

    
};
