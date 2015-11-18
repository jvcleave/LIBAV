#pragma once

#include "OMX_Maps.h"



class Clock
{
public:
    Clock();
    void setup(bool useAudioAsReference);
    OMX_HANDLETYPE handle;
};