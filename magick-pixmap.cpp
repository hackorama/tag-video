#include "pixmap.h"

Pixmap::Pixmap()
{
	image = NULL;
	width = 0;
	height = 0;
	ok = false;
	init();
}

Pixmap::Pixmap(string filename)
{
	ImageInfo *image_info = CloneImageInfo((ImageInfo *) NULL);
	GetExceptionInfo(&exception);
	strcpy(image_info->filename, filename.c_str());
	image = ReadImage(image_info,&exception);
	DestroyImageInfo(image_info);
	if (image == (Image *) NULL){
		width = 0;
		height = 0;
		ok = false;
	}else{
		width = image->columns;
		height =  image->rows;
		ok = true;
	}
	debug = false;
	init();
}

Pixmap::~Pixmap()
{
	DestroyImage(image);
	DestroyExceptionInfo(&exception);
}

void
Pixmap::init()
{
	IMGTYPE = "png";
	serial = 0 ;
	pen_r = MAXRGB;
	pen_g = 0;
	pen_b = 0;
	savePen();
}

void
Pixmap::resizePixmap(int _w, int _h)
{
	if(width == _w && height == _h) return;

	Image *resized_image = ScaleImage(image, _w, _h, &exception);
	DestroyImage(image);
	image = resized_image;
	width = image->columns;
	height =  image->rows;
	if(debug) cout << "pixmap resized to :" << width << ", " << height << endl;
}

Image*
Pixmap::getImage()
{
	return image;
}

void 
Pixmap::debugImage(string name, int i)
{
	writeImage(name, i);
}

void 
Pixmap::debugImage(string name)
{
	writeImage(name);
}

void 
Pixmap::writeImage(string name, int i)
{
	stringstream filename;
	string localserial = "" ;
	if( i < 10 )		    localserial = "00";
	else if( i < 100 && i > 9 ) localserial = "0";
	filename << name << "-" << localserial  << i ;
	writeImage(filename.str());
}
void 
Pixmap::writeImage(string name)
{
	serial++;

	stringstream filename;
	string prefix = "" ;
	if( serial < 10 )		      prefix = "00";
	else if( serial < 100 && serial > 9 ) prefix = "0";
	filename << prefix << serial << "-" << name << "." << IMGTYPE ;

	ImageInfo *image_info=CloneImageInfo((ImageInfo *) NULL);
	(void) strcpy(image->filename, (filename.str()).c_str());
	(void) strcpy(image_info->filename, (filename.str()).c_str());
	WriteImage(image_info,image);
	DestroyImageInfo(image_info);
}

void
Pixmap::setDebug(bool flag)
{
	debug = flag;
}

int
Pixmap::getWidth()
{
	if( width < 0 ) width = 0;
	return width;
}

int
Pixmap::getHeight()
{
	if( height < 0 ) height = 0;
	return height;
}

bool
Pixmap::isValid()
{
	if( image == NULL ) ok = false;
	return ok;
}

int
Pixmap::maxRGB()
{
	return MAXRGB;
}

bool
Pixmap::inRange(int x, int y)
{
	if( ! isValid() )  return false;
	if( ( x >= 0 &&  x < width ) && ( y >= 0 &&  y < height ) ) {
		return true;
	}
	return false;
}

void
Pixmap::switchColor( int r, int g, int b, int new_r, int new_g, int new_b )
{
	for(int i=0; i<width; i++){
		for(int j=0; j<height; j++){
			switchPixel( i, j, r, g, b, new_r, new_g, new_b);
		}
	}
}

bool
Pixmap::switchPixel( int x, int y, int r, int g, int b, 
					int new_r, int new_g, int new_b )
{
	if( ! inRange( x, y ) ) return true;
	PixelPacket pixel = GetOnePixel( image, x, y);
	if( pixel.red == r &&  pixel.green == g &&  pixel.blue == b ){
		return setPixel( x, y, new_r, new_g, new_b );
	}
	return false;
}

int
Pixmap::getPixel( int x, int y ) 
{
	//if( ! inRange( x, y ) ) return 0;
	PixelPacket pixel = GetOnePixel( image, x, y);
	return ( pixel.red + pixel.green + pixel.blue );
}

void
Pixmap::setPen( int r, int g, int b ) 
{
	pen_r = convertRGB(r);
	pen_g = convertRGB(g);
	pen_b = convertRGB(b);
}

void
Pixmap::savePen() 
{
	saved_pen_r = pen_r;
	saved_pen_g = pen_g;
	saved_pen_b = pen_b;
}

void
Pixmap::restorePen() 
{
	pen_r = saved_pen_r;
	pen_g = saved_pen_g;
	pen_b = saved_pen_g;
}

int
Pixmap::convertRGB( int x ) 
{
	if( x <= 0 || x > 256  ) return x;
	int factor = MAXRGB / 256;
	return x*factor;
}

bool
Pixmap::colorPixel( int x, int y ) // not black
{
	if( ! inRange( x, y ) ) return true;
	PixelPacket pixel = GetOnePixel( image, x, y);
	int p = pixel.red + pixel.green + pixel.blue ;
	if( p > 0 && p < (3 * MAXRGB) ) return true;
	return false;
}

bool
Pixmap::darkPixel( int x, int y ) // not black
{
	if( ! inRange( x, y ) ) return true;
	PixelPacket pixel = GetOnePixel( image, x, y);
	int p = pixel.red + pixel.green + pixel.blue ;
	if( p >= MAXRGB ) return true;
	return false;
}

bool
Pixmap::lightPixel( int x, int y ) // not black
{
	if( ! inRange( x, y ) ) return true;
	PixelPacket pixel = GetOnePixel( image, x, y);
	int p = pixel.red + pixel.green + pixel.blue ;
	if( p < MAXRGB ) return true;
	return false;
}

bool
Pixmap::redPixel( int x, int y ) 
{
	if( ! inRange( x, y ) ) return false;
	PixelPacket pixel = GetOnePixel( image, x, y);
	int p = pixel.red + pixel.green + pixel.blue ;
	if( pixel.red == MAXRGB && p == MAXRGB ) return true;
	return false;
}

bool
Pixmap::greenPixel( int x, int y ) 
{
	if( ! inRange( x, y ) ) return false;
	PixelPacket pixel = GetOnePixel( image, x, y);
	int p = pixel.red + pixel.green + pixel.blue ;
	if( pixel.green == MAXRGB && p == MAXRGB ) return true;
	return false;
}

bool
Pixmap::bluePixel( int x, int y ) 
{
	if( ! inRange( x, y ) ) return false;
	PixelPacket pixel = GetOnePixel( image, x, y);
	int p = pixel.red + pixel.green + pixel.blue ;
	if( pixel.blue == MAXRGB && p == MAXRGB ) return true;
	return false;
}

bool
Pixmap::whitePixel( int x, int y ) // not black
{
	if( ! inRange( x, y ) ) return true;
	PixelPacket pixel = GetOnePixel( image, x, y);
	int p = pixel.red + pixel.green + pixel.blue ;
	if( p == (3 * MAXRGB) ) return true;
	return false;
}

bool
Pixmap::blackPixel( int x, int y ) // black 
{
	if( ! inRange( x, y ) ) return false;
	PixelPacket pixel = GetOnePixel( image, x, y);
	int p = pixel.red + pixel.green + pixel.blue ;
	if( p ==  0 ) return true;
	return false;
}

bool
Pixmap::markPixels(int x, int y, int r, int g, int b, int border)
{

	setPixel(x-border, y-border, r, g, b);
	setPixel(x,   y-border, r, g, b);
	setPixel(x+border, y-border, r, g, b);

	setPixel(x-border, y, r, g, b);
	setPixel(x,   y, r, g, b);
	setPixel(x+border, y, r, g, b);

	setPixel(x-border, y+border, r, g, b);
	setPixel(x,   y+border, r, g, b);
	setPixel(x+border, y+border, r, g, b);

	return true;
}

bool
Pixmap::setPixel(int x, int y)
{
	if( ! inRange( x, y ) ) return false;
	PixelPacket *pixel = SetImagePixels(image, x, y, 1, 1);
	pixel->red = pen_r;
	pixel->green = pen_g;
	pixel->blue = pen_b;
	if (!SyncImagePixels(image)) { 
		fprintf( stdout, "Sync Error\n" );
		return false;
	}
	return true;
}

bool
Pixmap::setPixel(int x, int y, int width)
{
	if( ! inRange( x, y ) ) return false;
	bool result = true;
	for(int i = 0; i < width*2; i++){
		result &= setPixel((x-width)+i, y);
	}
	for(int i = 0; i < width*2; i++){
		result &= setPixel(x, (y-width)+i);
	}
	return result;
}

bool
Pixmap::setPixel256RGB(int x, int y, int r, int g, int b)
{
	int factor = MAXRGB / 256 ;
	return setPixel(x, y, r*factor, g*factor, b*factor);
}

bool
Pixmap::setPixel(int x, int y, int r, int g, int b)
{
	if( ! inRange( x, y ) ) return false;
	PixelPacket *pixel = SetImagePixels(image, x, y, 1, 1);
	pixel->red = r;
	pixel->green = g;
	pixel->blue = b;
	if (!SyncImagePixels(image)) { 
		fprintf( stdout, "Sync Error\n" );
		return false;
	}
	return true;
}

void
Pixmap::clearPixmap(int r, int g, int b)
{
	for(int i=0; i<width; i++){
		for(int j=0; j<height; j++){
			setPixel( i, j, r, g, b);
		}
	}
}

void
Pixmap::clearPixmap()
{
	clearPixmap(maxRGB(), maxRGB(), maxRGB());
}

void
Pixmap::clearPixmapWhite()
{
	clearPixmap(0, 0, 0);
}

void
Pixmap::markPoint(int x, int y, int radius)
{
	for( int i = 0; i < (2*radius); i++)
		for( int j = 0; j < (2*radius); j++)
			setPixel( (x-radius) + i, (y-radius) + j );
}

void
Pixmap::markPoint(int x, int y)
{
	setPixel(x,y);

	setPixel(x++,y);
	setPixel(x--,y);
	setPixel(x,y++);
	setPixel(x,y--);
	setPixel(x++,y++);
	setPixel(x++,y--);
	setPixel(x--,y++);
	setPixel(x--,y--);
}

void
Pixmap::markVLine(int x)
{
	markVLine(1, height-1, x);
}

void
Pixmap::markHLine(int y)
{
	markHLine(1, width-1, y);
}

void
Pixmap::markHLine(int _x1, int _x2, int y)
{
	int x1 = 0,  x2 = 0;
	if( _x1 < _x2 ) { 
		x1 = _x1 ; x2 = _x2 ;
	}else{
		x1 = _x2 ; x2 = _x1 ;
	}
	for(int x = x1; x <= x2; x++){
		setPixel(x, y);
	}
}

void
Pixmap::markVLine(int _y1, int _y2, int x)
{
	int y1 = 0,  y2 = 0;
	if( _y1 < _y2 ) { 
		y1 = _y1 ; y2 = _y2 ;
	}else{
		y1 = _y2 ; y2 = _y1 ;
	}
	for(int y = y1; y <= y2; y++){
		setPixel(x, y);
	}
}

