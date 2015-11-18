#pragma once

#include "Decoder.h"

class Player
{
public:
    Player()
    {
        videoFile = NULL;
        clock = NULL;
    };
    
    void setup(Clock* clock_, VideoFile* videoFile_)
    {
        clock = clock_;
        videoFile = videoFile_;
        decoder.setup(clock, videoFile->getBestVideoStream());
    }
    Clock* clock;
    VideoFile* videoFile;
    Decoder decoder;
};
