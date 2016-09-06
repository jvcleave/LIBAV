#pragma once
#include "Component.h"


class DecoderComponent: public Component
{
public:
    
 

    DecoderComponent()
    {
        name = OMX_VIDEO_DECODER;
    }

};


class RenderComponent: public Component
{
public:
    
    
    RenderComponent()
    {
        name = OMX_VIDEO_RENDER;
    }

};

class SchedulerComponent: public Component
{
public:
    
    
    SchedulerComponent()
    {
        name = OMX_VIDEO_SCHEDULER;
    }

};