#include "decoder.h"

Decoder::Decoder(string _filename)
{
	init(NULL);
	config->TAG_IMAGE_FILE = _filename;
	config->ARGS_OK = true;
}

Decoder::Decoder(int argc,char **argv)
{
	init(NULL);
	config->checkArgs(argc, argv);
}

Decoder::Decoder(Tagimage* _tagimage)
{
	init(NULL);
	tagimage = _tagimage;
	if(tagimage != NULL) config->ARGS_OK = true;
}

Decoder::Decoder(Config *_config)
{
	init(_config);
}

Decoder::Decoder()
{
	init(NULL);
}

Decoder::~Decoder()
{
	if(tagimage != NULL) delete tagimage;
	if(local_config) { //'locally created'  Vs 'passed in reference'
		if(config != NULL)   delete config;
	}
}

void
Decoder::init(Config *_config)
{
	config = _config;
	tagimage = NULL;
	local_config = false;
	if( config == NULL ) { 
		local_config = true; //'locally created'  Vs 'passed in reference'
		config = new Config();
	}
	fuzzy = config->FUZZY;
	for(int i=0; i<12; i++) tag[i] = -1;
	last_mode = 1;
}

Config*
Decoder::getConfig()
{
	return config;
}

void
Decoder::copyTag(int* _tag)
{
	for(int i=0; i<12; i++) _tag[i] = tag[i];
}

//called once for single image processing
bool
Decoder::processTag()
{
	if(!config->ARGS_OK ) return false;
	if(tagimage == NULL) tagimage = new Tagimage(config);
	if(!tagimage->isValid()) { 
		delete tagimage; tagimage = NULL;
		return false;
	}
	if(config->VISUAL_DEBUG) config->setDebugPixmap(new Pixmap(config->TAG_IMAGE_FILE));
	Threshold* threshold = new Threshold(config, tagimage);
	threshold->computeEdgemap();
	delete tagimage; tagimage = NULL;
	delete threshold;
	Shape *shapes = new Shape[config->MAX_SHAPES];
	Shape *anchor = new Shape(config);
	Border* border = new Border(config, shapes, anchor);
	int nshapes = border->findShapes();
	delete border;
	if( nshapes >= 12  ){
		Pattern* pattern = new Pattern(config, shapes, nshapes, anchor);
		pattern->findCodeInternal(tag);
		delete pattern;
	}
	delete anchor;
	delete [] shapes;
	return true;
}

//called more than once in video frame processing
bool
Decoder::processFrame(unsigned char *data, int width, int height)
{
	assert(data != NULL) ;
	assert( (data+(width*height)) != NULL) ;

	//1. Skip frame 
	if(fuzzy->selectFrame(config->V_SKIP_FRAMES) == false) {
		if(fuzzy->tagIsActive()) markCode(data, width, height, 0);
		return false;
	}

	//2.Check Motion 
	if(fuzzy->motionDetect(data, width, height) == false) { 
		if(fuzzy->tagIsActive()) markCode(data, width, height, 0);
		return false;
	}

	//3. Process Tag
	config->V_GRID_WIDTH = width;
	config->V_GRID_HEIGHT = height;
	//config->setDebugPixmap(new Pixmap("debug.jpg"));
	for(int i=0; i<12; i++) tag[i] = -1;
	Threshold* threshold = new Threshold(config, data, width, height);
	threshold->computeEdgemap();
	delete threshold;
	resetVMap(width*height);
	Shape *shapes = new Shape[config->MAX_SHAPES];
	Shape *anchor = new Shape(config);
	Border* border = new Border(config, shapes, anchor);
	int nshapes = border->findShapes();
	delete border;
	if( nshapes >= 12  ){
		Pattern* pattern = new Pattern(config, shapes, nshapes, anchor);
		pattern->findCodeExternal(tag);
		delete pattern;
	}
		
	delete anchor;
	delete [] shapes;
	bool tagvalidity = fuzzy->validateTag(tag);
	if(tagvalidity) markCode(data, width, height, 1);
	return tagvalidity;
}

/*
void
Decoder::frameOverlayOnly(unsigned char *data, int width, int height)
{
	if(fuzzy->tagIsActive()) markCode(data, width, height, 0);
}
*/

void
Decoder::resetVMap(int size)
{
	if(config->V_MAP == NULL ) config->V_MAP = new bool[size];
	for(int i=0; i < size; i++) config->V_MAP[i] = false;
}

void
Decoder::markCode(unsigned char *data, int width, int height, int mode)
{
	if(mode == 0) mode = last_mode;

	int block_radius = width < height ?  width/50 : height/50 ;
	if(block_radius<1) block_radius = 2;

	assert(config->V_MAP != NULL );
	RGBBYTE *thispixel;

	/*float xscale =  (float)width  / (float)config->GRID_WIDTH ;  
	float yscale =  (float)height / (float)config->GRID_HEIGHT ;  
	if(xscale == 0) xscale = 1;
	if(yscale == 0) yscale = 1;*/

	for(int y = block_radius; y < height-block_radius; y++){
		for(int x = block_radius; x < width-block_radius; x++){
			if(config->V_MAP[(y*width) + x]) {
				if(mode == 1){
					for(int i=(block_radius*-1); i < block_radius; i++){
						for(int j=(block_radius*-1); j < block_radius; j++){
							thispixel = (RGBBYTE*) data + ((y+j)*width) + (x+i) ;
			    			thispixel->R = 0;
							thispixel->G = 255;
   		     				thispixel->B = 0;
						}
					}
				}else if(mode == 2){
					for(int i=(block_radius*-1); i < block_radius; i++){
						for(int j=(block_radius*-1); j < block_radius; j++){
							thispixel = (RGBBYTE*) data + ((y+j)*width) + (x+i) ;
			    			thispixel->R = 0;
							thispixel->G = 255;
   		     				thispixel->B = 255;
						}
					}
				}else if(mode == 3){
					for(int i=(block_radius*-1); i < block_radius; i++){
						for(int j=(block_radius*-1); j < block_radius; j++){
							thispixel = (RGBBYTE*) data + ((y+j)*width) + (x+i) ;
			    			thispixel->R = 255;
							thispixel->G = 0;
   		     				thispixel->B = 0;
						}
					}
				}
			}
		}
	}
	if(mode != 0) last_mode = mode;
}

int
Decoder::tagAge()
{
	if(fuzzy->tagIsBrandNew())  return 0;
	if(fuzzy->tagIsNew())  		return 1;
	if(fuzzy->tagIsOld())  		return 2;
	if(fuzzy->tagIsVeryOld()) 	return 3;
	return 3;
}

int
Decoder::validFor()
{
	return fuzzy->valid_for;
}
