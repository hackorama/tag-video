#ifndef _COMMON_H_INCLUDED
#define _COMMON_H_INCLUDED

//#define USEJPG
#define _WINDOWS

//#define NDEBUG //Remove asserts
#include <assert.h>
#include <iostream>

typedef struct _RGBBYTE { //same as RGBTRIPLE Windows GDI
    unsigned char R;
    unsigned char G;
    unsigned char B;
} RGBBYTE;

#endif /* _COMMON_H_INCLUDED */
