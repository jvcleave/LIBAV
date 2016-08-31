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
        clock.setup(videoFile->hasAudio());
        decoder.setup(&clock, videoFile->getBestVideoStream());
    }
 
};
