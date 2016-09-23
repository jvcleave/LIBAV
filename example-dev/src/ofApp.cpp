#include "ofApp.h"


void ofApp::setup()
{

    av_register_all();
    avformat_network_init();
    
    
    ofDirectory videos(ofToDataPath("/home/pi/videos/current", true));
    videos.sort();
    
    string videoPath = videos.getFiles()[0].path();
    //string videoPath = ofToDataPath("test.h264", true);

    //string videoPath = ofToDataPath("/home/pi/videos/current/KALI UCHIS - NEVER BE YOURS-e9aqYvzqrnI.mp4", true);
    ofLogVerbose() << "videoPath: " << videoPath;
    videoFile.setup(videoPath);
    int numPackets = videoFile.omxPackets.size();
    ofLogVerbose() << "numPackets: " << numPackets;
    player.setup(&videoFile);
    
    

}

void ofApp::update(){

}

void ofApp::draw(){

}

void ofApp::keyPressed(int key){

}

