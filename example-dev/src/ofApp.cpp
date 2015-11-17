#include "ofApp.h"


void ofApp::setup()
{

    string videoPath = ofToDataPath("Timecoded_Big_bunny_1.mov", true);
    ofLogVerbose() << "videoPath: " << videoPath;
    videoFile.setup(videoPath);

}

void ofApp::update(){

}

void ofApp::draw(){

}

void ofApp::keyPressed(int key){

}

