#include "ofApp.h"


void ofApp::setup()
{

    av_register_all();
    avformat_network_init();
    
    string videoPath = ofToDataPath("Timecoded_Big_bunny_1.mov", true);
    ofLogVerbose() << "videoPath: " << videoPath;
    videoFile.setup(videoPath);
    
    clock.setup(videoFile.hasAudio());
    //player.setup(&clock, &videoFile);
    
    

}

void ofApp::update(){

}

void ofApp::draw(){

}

void ofApp::keyPressed(int key){

}

