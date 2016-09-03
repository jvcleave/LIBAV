#include "ofApp.h"


void ofApp::setup()
{

    av_register_all();
    avformat_network_init();
    
    //string videoPath = ofToDataPath("Timecoded_Big_bunny_1.mov", true);
    //string videoPath = ofToDataPath("test.h264", true);

    string videoPath = ofToDataPath("/home/pi/videos/current/KALI UCHIS - NEVER BE YOURS-e9aqYvzqrnI.mp4", true);
    ofLogVerbose() << "videoPath: " << videoPath;
    videoFile.setup(videoPath);
    
    player.setup(&videoFile);
    
    

}

void ofApp::update(){

}

void ofApp::draw(){

}

void ofApp::keyPressed(int key){

}

