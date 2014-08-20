#ifndef _DECODER_H_INCLUDED
#define _DECODER_H_INCLUDED

#include <string>

#include "threshold.h"
#include "tagimage.h"
#include "pixmap.h"
#include "border.h"
#include "pattern.h"
#include "common.h"
#include "config.h"


using namespace std;

class Decoder 						//I am the tag decoder engine
{

public:
	Decoder(); 						//No image to work with (working on video frames ?)
	Decoder(Config *config); 						//No image to work with (working on video frames ?)
	Decoder(string filename); 		//Give me the image file name
	Decoder(int argc, char **argv); //Or give me the image file and other command line options
	Decoder(Tagimage* tagimage);	//Or give the image object you have created already 
	~Decoder();						//  **BE WARNED** To save memory I am told to delete the image 
									//  as soon I finish processing, so pass me a copy of you want to keep it. 
									//  *DONT DELETE** the image agian yourself afterwards
									//  I feel bad to do this, but memory is tight on cellphones 
	
	Config* getConfig();			//Get my configuration control, and customize my behaviour
	bool    processTag();			//Ask me to proces the tag for you (I assign all my work to others here)
	bool    processFrame(unsigned char *data, int width, int height);
	//void    frameOverlayOnly(unsigned char *data, int width, int height);
									//Ask me to proces a video frame, if I am on Win32 DirecShow 
									//(I assign all my work to others here also)
	void    copyTag(int *tag);		//Copy (not a reference) the result back to you
	int     tagAge();				//for frame processing cosmetics, age of last valid tag found
	void 	markCode(unsigned char *data, int width, int height, int mode);
	int     validFor();
	
private:							//These are my internal stuff, not of interest to outside world
	void init(Config *config);
	void resetVMap(int size);

	Config*   config;		//Where I store all my options (ask the Config class for details)
	Tagimage* tagimage;		//The image I am working on(either a reference or one I created)
	Fuzzy* 	  fuzzy;		//my fuzzy logic helper, to decide video frame chanegs and code correctness ( I get a ref from config->FUZZY )
	int tag[12];			//I store the result here
	bool local_config; 		//Lets me track if I created a Config myslef or I just got a reference passed to me
	int last_mode;

};

#endif /* _DECODER_H_INCLUDED */

