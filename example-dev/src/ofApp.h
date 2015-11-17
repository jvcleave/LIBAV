#pragma once

#include "ofMain.h"
#include "VideoFile.h"
#include "Clock.h"


class ofApp : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    
    VideoFile videoFile;
    Clock clock;

};
