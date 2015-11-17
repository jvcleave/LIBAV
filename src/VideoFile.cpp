//
//  VideoFile.cpp
//
//  Created by jason van cleave on 11/17/15.
//

#include "VideoFile.h"


bool doAbort = false;


struct BufferData {
    uint8_t *ptr = NULL;
    size_t size = 0; ///< size left in the buffer
};



static int interrupt_cb(void*)
{
    if(doAbort)
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

VideoFile::VideoFile()
{
    av_register_all();
    avformat_network_init();
    
    audioStreamCounter = 0;
    videoStreamCounter = 0;
    

}

void VideoFile::setup(string videoPath)
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
            bool isValidStream = stream.setup(avFormatContext->streams[i], i);
            if(isValidStream)
            {
                streams.push_back(stream);
            }
           
           
            
        }
        ofLogVerbose() << "streams.size(): " << streams.size();
        for (size_t i = 0; i < avFormatContext->nb_streams; i++)
        {
            ofLogVerbose() << streams[i].streamInfo.toString();
        }
        
        
        
        bool isOutput = false;
        int index = 0;
        av_dump_format(avFormatContext, index, videoPath.c_str(), isOutput);
        
    }
    
    ofLogVerbose() << "result: " << result;

}