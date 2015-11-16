#pragma once

#include "ofMain.h"
#include <sys/stat.h>


extern "C" 
{
    
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
    
#include <libswresample/swresample.h>
    
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
    
#include <libavcodec/avcodec.h>
    
#include <libavutil/avutil.h>
#include <libavutil/crc.h>
#include <libavutil/fifo.h>
#include <libavutil/opt.h>
#include <libavutil/mem.h>
#include <libavutil/file.h>

#include <libavformat/avformat.h>

}

class ofApp : public ofBaseApp{
	public:
		void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
};
