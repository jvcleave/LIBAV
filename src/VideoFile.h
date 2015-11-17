#pragma once

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
        forceSoftwareDecoding = false;
        isAVI = false;
        isMKV = false;
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
    void*        codecExtraData; // extra data for codec to use
    unsigned int codecExtraSize; // size of extra data
    unsigned int codecTag; // extra identifier hints for decoding
    
    /* ac3/dts indof */
    unsigned int framesize;
    uint32_t     syncword;
    
    bool isAVI;
    bool isMKV;
    
    void setup(AVStream *avStream)
    {
        codec               = avStream->codec->codec_id;
        codecExtraData      = avStream->codec->extradata;
        codecExtraSize      = avStream->codec->extradata_size;
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
    
    bool setup(AVStream *avStream_, int id_)
    {
        avStream = avStream_;
        id          = id_;
        bool isValid =false;
        switch (avStream->codec->codec_type)
        {
            case AVMEDIA_TYPE_AUDIO:
            {
                isValid = true;
                hasVideo = true;
                break;
            }
            case AVMEDIA_TYPE_VIDEO:
            {
                isValid = true;
                hasAudio = true;                
                break;
            }
        }
        
        if(isValid)
        {
           streamInfo.setup(avStream); 
        }
        return isValid;
    }
    
    AVStream    *avStream;
    int         id;
    StreamInfo streamInfo;
    bool hasAudio;
    bool hasVideo;
};

class VideoFile
{
public:
    VideoFile();
    void setup(string videoPath);
    
    vector<Stream> streams;
    
private:
    int audioStreamCounter;
    int videoStreamCounter;

};
