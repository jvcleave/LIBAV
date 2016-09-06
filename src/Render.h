#pragma once
#include "Component.h"


class DecoderComponent
{
public:
    
    Component component;
    OMX_HANDLETYPE decoder;

    DecoderComponent()
    {
    
    }
    OMX_HANDLETYPE setup()
    {
        decoder = component.setup(OMX_VIDEO_DECODER);
        return decoder;
    }
};