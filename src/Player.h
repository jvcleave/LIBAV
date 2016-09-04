#pragma once

#include "Decoder.h"

class Player
{
public:
    
    Clock clock;
    VideoFile* videoFile;
    Decoder decoder;
    
    Player()
    {
        videoFile = NULL;
    };
    
    void setup(VideoFile* videoFile_)
    {
        videoFile = videoFile_;
        clock.setup(false);
        decoder.setup(&clock, videoFile);
    }
 
};
