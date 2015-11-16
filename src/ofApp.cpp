#include "ofApp.h"

bool doAbort = false;

FILE* filePointer;
static int interrupt_cb(void* unused)
{
    if(doAbort)
    {
        return 1;
    }
    return 0;
}

static int onReadPacket(void* filePointer_, uint8_t* buffer, int bufferSize)
{
    if(interrupt_cb(NULL))
    {
        return -1;
    }
    FILE* myFile = (FILE*)filePointer_;
    return fread(buffer, 1, bufferSize, myFile);
}

static int64_t onSeekPacket(void* filePointer, int64_t pos, int whence)
{
    if(interrupt_cb(NULL))
    {
        return -1;
    }
    return 0;

}



//--------------------------------------------------------------
void ofApp::setup()
{
    filePointer = NULL;
    av_register_all();
    avformat_network_init();
    
    AVFormatContext* avFormatContext = avformat_alloc_context();
    avFormatContext->flags |= AVFMT_FLAG_NONBLOCK;
    const AVIOInterruptCB interruptCallback = { interrupt_cb, NULL };
    avFormatContext->interrupt_callback = interruptCallback;
    
    AVInputFormat* avInputFormat  = NULL;
    AVDictionary* avDictionary = NULL;

    string videoPath = ofToDataPath("Timecoded_Big_bunny_1.mov", true);

    //videoPath = "/opt/vc/src/hello_pi/hello_video/test.h264";
    int result = 0;
    bool isStream = false;
    ofLogVerbose() << "videoPath: " << videoPath;
    if(isStream)
    {
        result = avformat_open_input(&avFormatContext, videoPath.c_str(), avInputFormat, &avDictionary);
        av_dict_free(&avDictionary);
    }
    else
    {
        int FFMPEG_FILE_BUFFER_SIZE=32768;
        unsigned char* buffer = NULL;
        buffer = (unsigned char*)av_malloc(FFMPEG_FILE_BUFFER_SIZE);
        int write_flag = 0;
        filePointer = fopen64(videoPath.c_str(), "r");
        fseeko64(filePointer, 0, SEEK_END);
        off64_t fileLength = ftello64(filePointer);
        fseeko64(filePointer, 0, SEEK_SET);
        ofLogVerbose() << "fileLength: " << fileLength;
        
        AVIOContext* avIOContext = NULL;
        avIOContext = avio_alloc_context(buffer, 
                                         FFMPEG_FILE_BUFFER_SIZE, 
                                         write_flag, 
                                         (void*)(filePointer), 
                                         onReadPacket, 
                                         NULL,
                                         onSeekPacket);
        avIOContext->max_packet_size = 6144;
        
        if(avIOContext->max_packet_size)
        {
            avIOContext->max_packet_size *= FFMPEG_FILE_BUFFER_SIZE / avIOContext->max_packet_size;
            ofLogVerbose() << "avIOContext->max_packet_size: " << avIOContext->max_packet_size;
        }
        //unsigned int  flags     = READ_TRUNCATED | READ_BITRATE | READ_CHUNKED;

        struct stat st;
        if (fstat(fileno(filePointer), &st) == 0)
        {
            int isSeekable =  !S_ISFIFO(st.st_mode);
            ofLogVerbose() << "isSeekable: " << isSeekable;
            avIOContext->seekable = isSeekable;

        }
        
        
        unsigned int byteStreamOffset = 0;
        unsigned int maxProbeSize = 0;
        void* logContext = NULL;
        
        int probeResult = av_probe_input_buffer(avIOContext, &avInputFormat, videoPath.c_str(), logContext, byteStreamOffset, maxProbeSize);
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
 
        bool isOutput = false;
        int index = 0;
        av_dump_format(avFormatContext, index, videoPath.c_str(), isOutput);
        
        
        uint8_t *abuffer = NULL;
        size_t buffer_size = 4096;
        
        int mapResult = av_file_map(videoPath.c_str(), &abuffer, &buffer_size, 0, NULL);
        ofLogVerbose() << "mapResult: " << mapResult;
        
    }
    
    ofLogVerbose() << "result: " << result;
    
    
    

    
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}