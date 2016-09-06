#pragma once

#include "Decoder.h"

class Player
{
public:
    
    VideoFile* videoFile;
    Decoder decoder;
    
    Player()
    {
        videoFile = NULL;
    };
    
    void setup(VideoFile* videoFile_)
    {
        OMX_Init();

        videoFile = videoFile_;
        decoder.setup(videoFile);
    }
 
};
